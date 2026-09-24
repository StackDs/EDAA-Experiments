#!/usr/bin/env bash
# Script de ejecucion de pruebas sobre algoritmos de busqueda enfocado a la posicion del elemento buscado
# Genera CSVs en data/search_pos/ para los 6 algoritmos de busqueda.

set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$BASE_DIR/bin/bench_search_pos"
DATA_DIR="$BASE_DIR/data/search_pos"

mkdir -p "$DATA_DIR"

if [ ! -f "$BIN" ]; then
    echo "Error: Binario no encontrado en $BIN. Ejecuta 'make' primero."
    exit 1
fi

RUNS=128
N_SIZE=10000001
LOWER_K=0
UPPER_K=10000000
STEP_K=500000   # Saltos aditivos exactos de 5% sobre 10^7 elementos (21 puntos de 0% a 100%)

ALGORITHMS=("seq" "seq_stl" "bin" "bin_stl" "gal" "gal_stl")

echo -e "\033[1;34m================================================================"
echo " INICIANDO EXPERIMENTO 2: Efecto de la Posicion (k) en Busqueda"
echo " Tamano fijo n = $N_SIZE, Rango k in [$LOWER_K, $UPPER_K], Paso aditivo = $STEP_K"
echo " Repeticiones: $RUNS, Destino: $DATA_DIR/"
echo -e "================================================================\033[0m\n"

for algo in "${ALGORITHMS[@]}"; do
    OUT_CSV="$DATA_DIR/${algo}.csv"
    echo -e "\033[1;33m>>> Ejecutando algoritmo: [$algo] -> $OUT_CSV\033[0m"
    
    #Usamos taskset para amarrar la ejecucion al nucleo de maximo rendiminento y asi evitar cambios de contexto que puedan afectar a la mediacion
    taskset -c 2 "$BIN" "$algo" "$OUT_CSV" "$RUNS" "$N_SIZE" "$LOWER_K" "$UPPER_K" "$STEP_K"
    
    echo -e "\033[0;32m[OK] $algo finalizado exitosamente.\033[0m"
    echo "Pausa termica de enfriamiento (2s)..."
    sleep 2
    echo ""
done

echo -e "\033[1;32m================================================================"
echo " EXPERIMENTO 2 COMPLETADO: Todos los CSVs generados en data/search_pos/"
echo -e "================================================================\033[0m"
