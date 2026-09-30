#!/usr/bin/env bash

set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$BASE_DIR/bin/bench_trees"
DATA_DIR="${DATA_ROOT:-$BASE_DIR/data}/splay_distributions"

RUNS="${RUNS:-64}"
LOWER="${LOWER:-1000}"
UPPER="${UPPER:-128000}"
STEP="${STEP:-2}"
PAUSE_SECONDS="${PAUSE_SECONDS:-2}"
CPU_CORE="${CPU_CORE:-2}"

if [[ ! -x "$BIN" ]]; then
    echo "Error: no se encontro $BIN. Ejecuta benchmarks/run_all.sh primero." >&2
    exit 1
fi
if ! command -v taskset >/dev/null 2>&1; then
    echo "Error: CPU_CORE requiere taskset." >&2
    exit 1
fi

mkdir -p "$DATA_DIR"

TREES=(avl red_black splay)
DISTRIBUTIONS=(uniform negative_binomial)

echo "================================================================"
echo " BOLETIN 02: CONSULTAS Y ELIMINACION TRAS CONSULTAS (D1)"
echo " n: [$LOWER, $UPPER], factor: x$STEP, repeticiones: $RUNS"
echo " Nucleo: $CPU_CORE, pausa entre series: ${PAUSE_SECONDS}s"
echo " Distribuciones: ${DISTRIBUTIONS[*]}"
echo " CSV: $DATA_DIR/"
echo "================================================================"

for distribution in "${DISTRIBUTIONS[@]}"; do
    for tree in "${TREES[@]}"; do
        OUT_CSV="$DATA_DIR/${tree}_search_${distribution}.csv"
        echo ">>> $tree / search / $distribution -> $OUT_CSV"
        taskset -c "$CPU_CORE" "$BIN" "$tree" search "$distribution" "$OUT_CSV" "$RUNS" "$LOWER" "$UPPER" "$STEP"
        echo "[OK] $tree / search / $distribution"
        sleep "$PAUSE_SECONDS"
    done

    OUT_CSV="$DATA_DIR/splay_erase_after_search_${distribution}.csv"
    echo ">>> splay / erase_after_search / $distribution -> $OUT_CSV"
    taskset -c "$CPU_CORE" "$BIN" splay erase_after_search "$distribution" "$OUT_CSV" "$RUNS" "$LOWER" "$UPPER" "$STEP"
    echo "[OK] splay / erase_after_search / $distribution"
    sleep "$PAUSE_SECONDS"
done

echo "Experimento de distribuciones completado."
