#!/usr/bin/env bash
# Script de ejecucion de pruebas sobre algoritmos de busqueda enfocado a las 
# build, insert, extract_min, top, meld.
# Genera CSVs en data/heaps/ para las 5 operaciones fundamentales:

set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$BASE_DIR/bin/bench_heaps"
DATA_DIR="$BASE_DIR/data/heaps"

mkdir -p "$DATA_DIR"

if [ ! -f "$BIN" ]; then
    echo "Error: Binario no encontrado en $BIN. Ejecuta 'make' primero."
    exit 1
fi

RUNS=128
LOWER=1000
UPPER=1000000
STEP=2

HEAP_TYPES=("binary" "binomial")
OPERATIONS=("build" "insert" "extract_min" "top" "meld")

echo -e "\033[1;34m================================================================"
echo " INICIANDO EXPERIMENTO HEAPS: Binary Heap vs Binomial Heap"
echo " Rango: n in [$LOWER, $UPPER], Factor: x$STEP, Repeticiones: $RUNS"
echo " Destino: $DATA_DIR/"
echo -e "================================================================\033[0m\n"

for heap in "${HEAP_TYPES[@]}"; do
    for op in "${OPERATIONS[@]}"; do
        OUT_CSV="$DATA_DIR/${heap}_${op}.csv"
        echo -e "\033[1;33m>>> Ejecutando: [$heap - $op] -> $OUT_CSV\033[0m"
        
        #Usamos taskset para amarrar la ejecucion al nucleo de maximo rendimiento y asi evitar cambios de contexto que puedan afectar a la mediacion
        taskset -c 2 "$BIN" "$heap" "$op" "$OUT_CSV" "$RUNS" "$LOWER" "$UPPER" "$STEP"
        
        echo -e "\033[0;32m[OK] $heap - $op finalizado exitosamente.\033[0m"
        echo "Pausa termica de enfriamiento (2s)..."
        sleep 2
        echo ""
    done
done

echo -e "\033[1;32m================================================================"
echo " EXPERIMENTO HEAPS COMPLETADO: Todos los CSVs generados en data/heaps/"
echo -e "================================================================\033[0m"
