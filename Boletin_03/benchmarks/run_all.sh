#!/usr/bin/env bash
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"
run_experiments text_size pattern_size occurrences

# Generamos los graficos desde las carpetas fijas de cada experimento
echo "Generando graficos"
"$PYTHON" "$BASE_DIR/plots/plot_experiments.py" --session "$SESSION_DIR"
