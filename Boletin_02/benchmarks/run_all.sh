#!/usr/bin/env bash

set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCH_DIR="$BASE_DIR/benchmarks"
DATA_ROOT="${DATA_ROOT:-$BASE_DIR/data}"
export DATA_ROOT

START_TIME=$(date +%s)

echo "================================================================"
echo " BATERIA EXPERIMENTAL COMPLETA: BOLETIN 02"
echo " Inicio: $(date)"
echo "================================================================"

echo "[Fase 1/3] Preparando el binario con g++..."
make -C "$BASE_DIR" all

echo "[Fase 2/3] Midiendo insercion y eliminacion directa..."
bash "$BENCH_DIR/run_basic_ops.sh"

echo "[Fase 3/3] Midiendo busquedas y eliminacion tras consultas..."
bash "$BENCH_DIR/run_distributions.sh"

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
echo "================================================================"
echo " BATERIA COMPLETADA EN $((ELAPSED / 60)) min $((ELAPSED % 60)) s"
echo " CSV: $DATA_ROOT/basic_ops/ (6 archivos)"
echo " CSV: $DATA_ROOT/splay_distributions/ (8 archivos)"
echo "================================================================"
