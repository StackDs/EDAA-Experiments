#!/usr/bin/env bash
# Script de ejecucion de pruebas sobre algoritmos de busqueda enfocado al size del arreglo
# Genera CSVs en data/search_size/ para los 6 algoritmos de busqueda.

set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$BASE_DIR/bin/bench_search_size"
DATA_DIR="$BASE_DIR/data/search_size"

mkdir -p "$DATA_DIR"

if [ ! -f "$BIN" ]; then
    echo "Error: Binario no encontrado en $BIN. Ejecuta 'make' primero."
    exit 1
fi

RUNS=128
LOWER=1000
UPPER=10000000
STEP=2

ALGORITHMS=("seq" "seq_stl" "bin" "bin_stl" "gal" "gal_stl")

echo -e "\033[1;34m================================================================"
echo " INICIANDO EXPERIMENTO 1: Efecto del Tamano (n) en Busqueda"
echo " Rango: n in [$LOWER, $UPPER], Factor: x$STEP, Repeticiones: $RUNS"
echo " Destino: $DATA_DIR/"
echo -e "================================================================\033[0m\n"

for algo in "${ALGORITHMS[@]}"; do
    OUT_CSV="$DATA_DIR/${algo}.csv"
    echo -e "\033[1;33m>>> Ejecutando algoritmo: [$algo] -> $OUT_CSV\033[0m"
    
    #Usamos taskset para amarrar la ejecucion al nucleo de maximo rendiminento y asi evitar cambios de contexto que puedan afectar a la mediacion
    taskset -c 2 "$BIN" "$algo" "$OUT_CSV" "$RUNS" "$LOWER" "$UPPER" "$STEP"
    
    echo -e "\033[0;32m[OK] $algo finalizado exitosamente.\033[0m"
    echo "Pausa termica de enfriamiento (2s)..."
    sleep 2
    echo ""
done

echo -e "\033[1;32m================================================================"
echo " EXPERIMENTO 1 COMPLETADO: Todos los CSVs generados en data/search_size/"
echo -e "================================================================\033[0m"
