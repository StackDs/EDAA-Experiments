#!/usr/bin/env bash
# Adaptado de la orquestacion del boletin 03
set -euo pipefail
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$BASE_DIR/benchmarks/progress.sh"
PYTHON="${PYTHON:-python3}"
export PYTHONDONTWRITEBYTECODE=1

usage() {
    echo 'Uso: common.sh all|pilot|official|smoke [opciones]'
    echo 'Opciones: --sizes --seeds --cycles --dynamic-queries --static-queries'
    echo '         --target-ns --max-batch --cpu-core --data-root --pilot --timeout-seconds'
}

configure() {
    SIZES=1000,4000,16000,64000,256000,1000000
    SEEDS=1729,2718,31415
    CYCLES=10 DYNAMIC_QUERIES=1000 STATIC_QUERIES=10000
    TARGET_NS=5000000 MAX_BATCH=1000000 TIMEOUT_SECONDS=3600
    CPU_CORE="${CPU_CORE:-2}" DATA_ROOT="$BASE_DIR/data"
    PILOT="${PILOT:-}" SESSION_PATH_FILE=""
    while [[ $# -gt 0 ]]; do
        if [[ "$1" == --help || "$1" == -h ]]; then usage; exit 0; fi
        if [[ $# -lt 2 ]]; then echo "Falta el valor de $1" >&2; return 1; fi
        case "$1" in
            --sizes) SIZES="$2" ;;
            --seeds) SEEDS="$2" ;;
            --cycles) CYCLES="$2" ;;
            --dynamic-queries) DYNAMIC_QUERIES="$2" ;;
            --static-queries) STATIC_QUERIES="$2" ;;
            --target-ns) TARGET_NS="$2" ;;
            --max-batch) MAX_BATCH="$2" ;;
            --cpu-core) CPU_CORE="$2" ;;
            --data-root) DATA_ROOT="$2" ;;
            --pilot) PILOT="$2" ;;
            --session-path-file) SESSION_PATH_FILE="$2" ;;
            --timeout-seconds) TIMEOUT_SECONDS="$2" ;;
            *) echo "Opcion desconocida: $1" >&2; return 1 ;;
        esac
        shift 2
    done
    if [[ "$DATA_ROOT" == '~' ]]; then DATA_ROOT="$HOME";
    elif [[ "$DATA_ROOT" == '~/'* ]]; then DATA_ROOT="$HOME/${DATA_ROOT:2}"; fi
}

acquire_lock() {
    mkdir -p "$BASE_DIR/data"
    if [[ -n "${BENCHMARK_LOCK_FD:-}" ]]; then
        if [[ ! "$BENCHMARK_LOCK_FD" =~ ^[0-9]+$ ]] ||
           [[ ! "/proc/$$/fd/$BENCHMARK_LOCK_FD" -ef "$BASE_DIR/data/.benchmark.lock" ]]; then
            echo 'El bloqueo heredado no corresponde a esta bateria' >&2
            return 1
        fi
        exec 9>&"$BENCHMARK_LOCK_FD"
    else
        exec 9>"$BASE_DIR/data/.benchmark.lock"
    fi
    flock -n 9 || { echo 'Ya hay otra preparacion o medicion activa' >&2; return 1; }
    export BENCHMARK_LOCK_FD=9
}

check_cpu() {
    if [[ ! "$CPU_CORE" =~ ^[0-9]{1,6}$ ]]; then echo 'CPU_CORE invalido' >&2; return 1; fi
    CPU_CORE=$((10#$CPU_CORE))
    local allowed part first last
    allowed="$(taskset -pc "$$")"
    allowed="${allowed##*: }"
    local -a parts
    IFS=, read -ra parts <<< "$allowed"
    for part in "${parts[@]}"; do
        first="${part%-*}" last="${part#*-}"
        if (( CPU_CORE >= first && CPU_CORE <= last )); then
            taskset -c "$CPU_CORE" true
            return
        fi
    done
    echo 'CPU_CORE no pertenece a la afinidad permitida' >&2
    return 1
}

temporary_root() {
    if [[ -d /tmp/opencode ]]; then printf '/tmp/opencode\n'; else printf '%s\n' "${TMPDIR:-/tmp}"; fi
}

run_session() (
    PROFILE="$1" SESSION_PATH_FILE="${2:-}"
    case "$PROFILE" in
        official)
            RUNS=32
            if [[ -z "$PILOT" ]]; then echo 'Primero ejecute make pilot y seleccione PILOT con su ruta' >&2; exit 1; fi ;;
        pilot) RUNS=4 ;;
        smoke)
            RUNS=4 SIZES=17,33 SEEDS=1729 CYCLES=2
            DYNAMIC_QUERIES=60 STATIC_QUERIES=64 TARGET_NS=1000 MAX_BATCH=128 ;;
        *) echo 'Perfil invalido' >&2; exit 1 ;;
    esac
    check_cpu
    scratch="$(mktemp -d "$(temporary_root)/boletin04-session-XXXXXXXX")"
    SESSION_DIR="" START_CS=0 stage=preparacion
    local progress_start
    progress_start="$(uptime_cs)"
    display_progress 0 4 "$progress_start" "$PROFILE | Validando configuracion y preparando sesion"
    cleanup() {
        local status=$?
        if [[ $status -ne 0 && -n "$SESSION_DIR" ]]; then
            ELAPSED_CS=$(( $(uptime_cs) - START_CS ))
            ERROR_MESSAGE="Fallo en $stage con codigo $status"
            export ELAPSED_CS ERROR_MESSAGE
            printf '%s\n' "$ERROR_MESSAGE" >> "$SESSION_DIR/execution.log"
            "$PYTHON" "$BASE_DIR/scripts/session_metadata.py" fail || true
        fi
        if [[ $status -ne 0 ]]; then echo "$PROFILE | Interrumpido en $stage con codigo $status" >&2; fi
        rm -rf -- "$scratch"
        exit "$status"
    }
    trap cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    CONTEXT_FILE="$scratch/context.json"
    export PROFILE RUNS SIZES SEEDS CYCLES DYNAMIC_QUERIES STATIC_QUERIES
    export TARGET_NS MAX_BATCH CPU_CORE TIMEOUT_SECONDS PILOT SESSION_PATH_FILE CONTEXT_FILE
    prepared="$("$PYTHON" "$BASE_DIR/scripts/session_metadata.py" prepare)"
    read -r CONFIG_ID EFFECTIVE_TIMEOUT SIZES SEEDS CYCLES DYNAMIC_QUERIES STATIC_QUERIES TARGET_NS MAX_BATCH <<< "$prepared"
    mkdir -p "$DATA_ROOT"
    DATA_ROOT="$(cd "$DATA_ROOT" && pwd -P)"
    SESSION_DIR="$(mktemp -d "$DATA_ROOT/$PROFILE-$(date +%Y%m%d-%H%M%S)-XXXXXXXX")"
    export SESSION_DIR
    START_CS="$(uptime_cs)"
    "$PYTHON" "$BASE_DIR/scripts/session_metadata.py" begin
    echo "Sesion: $SESSION_DIR"

    run_timed() {
        local remaining seconds
        remaining=$(( EFFECTIVE_TIMEOUT * 100 - $(uptime_cs) + START_CS ))
        if (( remaining <= 0 )); then echo 'Presupuesto de tiempo agotado' >&2; return 124; fi
        printf -v seconds '%d.%02d' "$((remaining / 100))" "$((remaining % 100))"
        printf '\nEtapa %s\n' "$stage" >> "$SESSION_DIR/execution.log"
        timeout --signal=KILL "${seconds}s" "$@" 2>&1 | tee -a "$SESSION_DIR/execution.log"
    }

    ARGS=(--profile "$PROFILE" --output "$SESSION_DIR" --runs "$RUNS"
          --session-id "${SESSION_DIR##*/}" --config-id "$CONFIG_ID"
          --sizes "$SIZES" --seeds "$SEEDS" --cycles "$CYCLES"
          --dynamic-queries "$DYNAMIC_QUERIES" --static-queries "$STATIC_QUERIES"
          --target-ns "$TARGET_NS" --max-batch "$MAX_BATCH")
    stage=mediciones
    display_progress 1 4 "$progress_start" "$PROFILE | Ejecutando los cuatro experimentos"
    run_timed taskset -c "$CPU_CORE" "$BASE_DIR/bin/bench_rmq" "${ARGS[@]}"
    stage=memoria
    display_progress 2 4 "$progress_start" "$PROFILE | Analizando memoria en una pasada separada"
    run_timed taskset -c "$CPU_CORE" "$BASE_DIR/bin/bench_memory" "${ARGS[@]}"
    stage=graficos
    display_progress 3 4 "$progress_start" "$PROFILE | Validando resultados y generando graficos"
    local -a plot_args=(--session "$SESSION_DIR" --staged)
    if [[ "$PROFILE" == official ]]; then
        if [[ "$DATA_ROOT" == "$BASE_DIR/data" ]]; then
            plot_args+=(--publish-root "$BASE_DIR")
        else
            plot_args+=(--publish-root "$DATA_ROOT/results")
        fi
    fi
    run_timed env MPLCONFIGDIR="$scratch/mpl" "$PYTHON" "$BASE_DIR/plots/plot_experiments.py" "${plot_args[@]}"
    ELAPSED_CS=$(( $(uptime_cs) - START_CS ))
    export ELAPSED_CS
    stage=cierre
    "$PYTHON" "$BASE_DIR/scripts/session_metadata.py" finish
    display_progress 4 4 "$progress_start" "$PROFILE | Resultados y graficos completos"
    echo "Resultados completos: $SESSION_DIR"
)

run_experiments() {
    local profile="$1"
    shift
    configure "$@"
    acquire_lock
    run_session "$profile" "$SESSION_PATH_FILE"
}

run_all() {
    configure "$@"
    acquire_lock
    local scratch
    scratch="$(mktemp -d "$(temporary_root)/boletin04-all-XXXXXXXX")"
    trap "$(printf 'rm -f -- %q; rmdir -- %q' "$scratch/pilot-path" "$scratch")" EXIT
    if [[ -z "$PILOT" ]]; then
        run_session pilot "$scratch/pilot-path"
        IFS= read -r PILOT < "$scratch/pilot-path"
    fi
    run_session official "$SESSION_PATH_FILE"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    profile="${1:-}"
    if [[ $# -gt 0 ]]; then shift; fi
    if [[ "$profile" == --help || "$profile" == -h ]]; then usage; exit 0; fi
    if [[ "$profile" == all ]]; then run_all "$@"; else run_experiments "$profile" "$@"; fi
fi
