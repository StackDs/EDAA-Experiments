#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
PROFILE="${PROFILE:-official}"
DATA_ROOT="${DATA_ROOT:-$BASE_DIR/data}"
CPU_CORE="${CPU_CORE:-2}"
PAUSE_SECONDS="${PAUSE_SECONDS:-2}"
RUNS="${RUNS:-64}"
TEXT_FILE="${TEXT_FILE:-$BASE_DIR/texts/english.bin}"
PATTERN_DIR="${PATTERN_DIR:-$BASE_DIR/texts/patterns-$PROFILE}"

if [[ "$PROFILE" != official ]]; then
    echo "Solo se admite el perfil oficial del informe" >&2
    exit 1
fi
SIZE=50000000
SIZES=1000000,5000000,10000000,20000000,50000000
COUNT=10
TARGET_NS="${TARGET_NS:-5000000}"
MAX_BATCH="${MAX_BATCH:-1000000}"
ARGS=(--text "$TEXT_FILE" --patterns "$PATTERN_DIR" --profile "$PROFILE"
      --size "$SIZE" --sizes "$SIZES" --count "$COUNT" --runs "$RUNS"
      --target-ns "$TARGET_NS" --max-batch "$MAX_BATCH" --pause-seconds "$PAUSE_SECONDS")

prepare_inputs() {
    # Incorporamos el corpus y verificamos los patrones fuera del reloj
    if [[ "$TEXT_FILE" == "$BASE_DIR/texts/english.bin" ]]; then
        SOURCE_ARGS=()
        if [[ -n "${CORPUS_ARCHIVE:-}" ]]; then SOURCE_ARGS=(--source "$CORPUS_ARCHIVE"); fi
        "$PYTHON" "$BASE_DIR/scripts/prepare_text.py" --directory "$BASE_DIR/texts" "${SOURCE_ARGS[@]}"
    fi
    "$BASE_DIR/bin/prepare_patterns" "${ARGS[@]}"
}

run_experiments() {
    mkdir -p "$DATA_ROOT"
    # Serializamos las mediciones incluso entre procesos de Make distintos
    exec 9>"$BASE_DIR/texts/.benchmark.lock"
    flock -n 9 || { echo "Ya hay otra preparacion o medicion activa" >&2; exit 1; }
    taskset -c "$CPU_CORE" true || { echo "Nucleo no disponible seleccione CPU_CORE" >&2; exit 1; }
    prepare_inputs
    SESSION_DIR="$(mktemp -d "$DATA_ROOT/$PROFILE-$(date +%Y%m%d-%H%M%S)-XXXXXX")"
    export SESSION_DIR CPU_CORE PROFILE RUNS PAUSE_SECONDS TARGET_NS MAX_BATCH BASE_DIR
    "$PYTHON" - <<'PY'
import json
import hashlib
import os
import platform
from pathlib import Path

metadata = {key.lower(): os.environ[key] for key in
            ("CPU_CORE", "PROFILE", "RUNS", "PAUSE_SECONDS", "TARGET_NS", "MAX_BATCH")}
metadata["platform"] = platform.platform()
metadata.update(json.loads(Path(os.environ["BASE_DIR"], "bin/build-config.json").read_text()))
metadata["binary_sha256"] = {}
for binary in Path(os.environ["BASE_DIR"]).glob("bin/bench_*"):
    with binary.open("rb") as source:
        metadata["binary_sha256"][binary.name] = hashlib.file_digest(source, "sha256").hexdigest()
Path(os.environ["SESSION_DIR"], "session.json").write_text(json.dumps(metadata, indent=2) + "\n")
PY
    for experiment in "$@"; do
        # Ejecutamos cada experimento con afinidad y salida propia
        taskset -c "$CPU_CORE" "$BASE_DIR/bin/bench_$experiment" "${ARGS[@]}" \
            --output "$SESSION_DIR/$experiment"
    done
    echo "Sesion: $SESSION_DIR"
    echo "CSV: $SESSION_DIR"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    if [[ "${1:-}" != prepare ]]; then echo "Use prepare" >&2; exit 1; fi
    exec 9>"$BASE_DIR/texts/.benchmark.lock"
    flock -n 9 || { echo "Ya hay otra preparacion o medicion activa" >&2; exit 1; }
    prepare_inputs
fi
