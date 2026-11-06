#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
PROFILE="${PROFILE:-official}"
DATA_ROOT="${DATA_ROOT:-$BASE_DIR/data}"
CPU_CORE="${CPU_CORE:-2}"
PAUSE_SECONDS="${PAUSE_SECONDS:-2}"
RUNS="${RUNS:-32}"
TEXT_FILE="${TEXT_FILE:-$BASE_DIR/texts/english.bin}"
PATTERN_DIR="${PATTERN_DIR:-$BASE_DIR/texts/patterns-$PROFILE}"

if [[ "$PROFILE" != official ]]; then
    echo "Solo se admite el perfil oficial del informe" >&2
    exit 1
fi
SIZE=10000000
SIZES=1000000,2000000,4000000,8000000,10000000
COUNT=5
TARGET_NS="${TARGET_NS:-5000000}"
MAX_BATCH="${MAX_BATCH:-1000000}"
ARGS=(--text "$TEXT_FILE" --patterns "$PATTERN_DIR" --profile "$PROFILE"
      --size "$SIZE" --sizes "$SIZES" --count "$COUNT" --runs "$RUNS"
      --target-ns "$TARGET_NS" --max-batch "$MAX_BATCH" --pause-seconds "$PAUSE_SECONDS")

prepare_inputs() {
    echo "Preparando corpus y patrones"
    # Incorporamos el corpus y verificamos los patrones fuera del reloj
    if [[ "$TEXT_FILE" == "$BASE_DIR/texts/english.bin" ]]; then
        SOURCE_ARGS=()
        if [[ -n "${CORPUS_ARCHIVE:-}" ]]; then SOURCE_ARGS=(--source "$CORPUS_ARCHIVE"); fi
        "$PYTHON" "$BASE_DIR/scripts/prepare_text.py" --directory "$BASE_DIR/texts" "${SOURCE_ARGS[@]}"
    fi
    "$BASE_DIR/bin/prepare_patterns" "${ARGS[@]}"
}

run_experiments() {
    if [[ $# -eq 0 ]]; then echo "Seleccione al menos un experimento" >&2; return 1; fi
    local experiment
    for experiment in "$@"; do experiment_directory "$experiment" >/dev/null || return 1; done
    # Serializamos las mediciones incluso entre procesos de Make distintos
    # Make mantiene el mismo bloqueo desde la compilacion hasta la limpieza.
    if [[ "${BENCHMARK_LOCK_FD:-}" != 9 ]]; then
        exec 9>"$BASE_DIR/texts/.benchmark.lock"
    fi
    flock -n 9 || { echo "Ya hay otra preparacion o medicion activa" >&2; exit 1; }
    taskset -c "$CPU_CORE" true || { echo "Nucleo no disponible seleccione CPU_CORE" >&2; exit 1; }
    prepare_inputs
    mkdir -p "$DATA_ROOT"
    SESSION_DIR="$(cd "$DATA_ROOT" && pwd)"
    SESSION_ID="$("$PYTHON" -c 'import uuid; print(uuid.uuid4().hex)')"
    export SESSION_DIR SESSION_ID CPU_CORE PROFILE RUNS PAUSE_SECONDS TARGET_NS MAX_BATCH BASE_DIR
    SESSION_METADATA="$("$PYTHON" - <<'PY'
import json
import hashlib
import os
import platform
from pathlib import Path

metadata = {key.lower(): os.environ[key] for key in
            ("SESSION_ID", "CPU_CORE", "PROFILE", "RUNS", "PAUSE_SECONDS", "TARGET_NS", "MAX_BATCH")}
metadata["platform"] = platform.platform()
metadata.update(json.loads(Path(os.environ["BASE_DIR"], "bin/build-config.json").read_text()))
metadata["binary_sha256"] = {}
for binary in Path(os.environ["BASE_DIR"]).glob("bin/bench_*"):
    if not binary.is_file() or binary.suffix:
        continue
    with binary.open("rb") as source:
        metadata["binary_sha256"][binary.name] = hashlib.file_digest(source, "sha256").hexdigest()
print(json.dumps(metadata, indent=2))
PY
    )"
    export SESSION_METADATA
    local current=0 total=$# label status
    echo "Resultados: $SESSION_DIR"
    for experiment in "$@"; do
        current=$((current + 1))
        case "$experiment" in
            text_size) label="Largo del texto" ;;
            pattern_size) label="Largo del patron" ;;
            occurrences) label="Numero de ocurrencias" ;;
        esac
        printf '\n[%s/%s] Experimento: %s\n' "$current" "$total" "$label"
        EXPERIMENT_DIR="$SESSION_DIR/$(experiment_directory "$experiment")"
        export EXPERIMENT_DIR
        # Solo sustituimos el experimento seleccionado, no las otras mediciones.
        "$PYTHON" - <<'PY'
import os
import shutil
from pathlib import Path

directory = Path(os.environ["EXPERIMENT_DIR"])
if directory.is_symlink() or directory.is_file():
    raise ValueError("La salida del experimento debe ser un directorio sin enlaces simbolicos")
if directory.exists():
    shutil.rmtree(directory)
PY
        # Ejecutamos cada experimento con afinidad y salida propia
        status=0
        taskset -c "$CPU_CORE" "$BASE_DIR/bin/bench_$experiment" "${ARGS[@]}" \
            --output "$EXPERIMENT_DIR" --session-id "$SESSION_ID" || status=$?
        # Conservamos tambien los metadatos de una medicion que quedo incompleta.
        "$PYTHON" - <<'PY'
import os
from pathlib import Path

directory = Path(os.environ["EXPERIMENT_DIR"])
if directory.is_dir():
    (directory / "session.json").write_text(os.environ["SESSION_METADATA"] + "\n")
PY
        if [[ $status -ne 0 ]]; then return "$status"; fi
    done
    echo "CSV: $SESSION_DIR"
}

experiment_directory() {
    case "$1" in
        text_size) echo experimento_1_largo_texto ;;
        pattern_size) echo experimento_2_largo_patron ;;
        occurrences) echo experimento_3_numero_ocurrencias ;;
        *) echo "Experimento desconocido: $1" >&2; return 1 ;;
    esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    if [[ "${1:-}" != prepare ]]; then echo "Use prepare" >&2; exit 1; fi
    exec 9>"$BASE_DIR/texts/.benchmark.lock"
    flock -n 9 || { echo "Ya hay otra preparacion o medicion activa" >&2; exit 1; }
    prepare_inputs
fi
