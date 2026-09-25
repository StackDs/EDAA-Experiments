#!/usr/bin/env bash


set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCH_DIR="$BASE_DIR/benchmarks"

START_TIME=$(date +%s)

echo -e "\033[1;35m================================================================"
echo " BATERIA EXPERIMENTAL COMPLETA: BOLETIN 01 (EDAA)"
echo " Fecha de inicio: $(date)"
echo " CPU: $(lscpu | grep "Model name:" | sed -e 's/Model name:[[:space:]]*//')"
echo -e "================================================================\033[0m\n"

# 1. Compilar todo con optimizacion agresiva
echo -e "\033[1;36m[Fase 1/4] Compilando binarios optimizados (-O3 -march=native)...\033[0m"
make -C "$BASE_DIR" all

# 2. Experimento de Busqueda: size
echo -e "\n\033[1;36m[Fase 2/4] Ejecutando Experimento 1 (Busqueda por size n)...\033[0m"
bash "$BENCH_DIR/run_search_size.sh"

# 3. Experimento de Busqueda: Posicion
echo -e "\n\033[1;36m[Fase 3/4] Ejecutando Experimento 2 (Busqueda por Posicion k)...\033[0m"
bash "$BENCH_DIR/run_search_pos.sh"

# 4. Experimento de Colas de Prioridad (Heaps)
echo -e "\n\033[1;36m[Fase 4/4] Ejecutando Experimento 3 (Colas de Prioridad: Heaps)...\033[0m"
bash "$BENCH_DIR/run_heaps.sh"

# 5. Limpieza de ejecutables compilados
echo -e "\n\033[1;36m[Limpieza] Eliminando binarios ejecutables compilados...\033[0m"
make -C "$BASE_DIR" clean

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

echo -e "\n\033[1;32m================================================================"
echo " TODAS LAS BATERIAS EXPERIMENTALES FINALIZARON EXITOSAMENTE"
echo " Tiempo total transcurrido: $((ELAPSED / 60)) min $((ELAPSED % 60)) s"
echo " Archivos generados en:"
echo "   - $BASE_DIR/data/search_size/ (6 CSVs)"
echo "   - $BASE_DIR/data/search_pos/  (6 CSVs)"
echo "   - $BASE_DIR/data/heaps/       (10 CSVs)"
echo -e "================================================================\033[0m"
