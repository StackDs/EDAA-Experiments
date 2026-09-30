#!/usr/bin/env bash

set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$BASE_DIR/bin/bench_trees"
DATA_DIR="${DATA_ROOT:-$BASE_DIR/data}/basic_ops"

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
OPERATIONS=(insert erase)

echo "================================================================"
echo " BOLETIN 02: INSERCION Y VACIADO COMPLETO DEL ARBOL (D0)"
echo " n: [$LOWER, $UPPER], factor: x$STEP, repeticiones: $RUNS"
echo " Nucleo: $CPU_CORE, pausa entre series: ${PAUSE_SECONDS}s"
echo " CSV: $DATA_DIR/"
echo "================================================================"

for tree in "${TREES[@]}"; do
    for op in "${OPERATIONS[@]}"; do
        OUT_CSV="$DATA_DIR/${tree}_${op}.csv"
        echo ">>> $tree / $op -> $OUT_CSV"
        taskset -c "$CPU_CORE" "$BIN" "$tree" "$op" none "$OUT_CSV" "$RUNS" "$LOWER" "$UPPER" "$STEP"
        echo "[OK] $tree / $op"
        sleep "$PAUSE_SECONDS"
    done
done

echo "Experimento de operaciones basicas completado."
