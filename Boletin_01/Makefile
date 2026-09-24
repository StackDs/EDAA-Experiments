
CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -Iinclude
BIN_DIR = bin
SRC_DIR = src
BENCH_DIR = benchmarks

TARGETS = $(BIN_DIR)/bench_search_size $(BIN_DIR)/bench_search_pos $(BIN_DIR)/bench_heaps

.PHONY: all clean clean_data clean_plots run_search_size run_search_pos run_heaps run_all directories plot plot_exp1 plot_exp2 plot_exp3 plot_search plot_heaps

all: directories $(TARGETS)

directories:
	@mkdir -p $(BIN_DIR) data/search_size data/search_pos data/heaps

$(BIN_DIR)/bench_search_size: $(SRC_DIR)/bench_search_size.cpp include/generator.hpp include/search/search.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/bench_search_pos: $(SRC_DIR)/bench_search_pos.cpp include/generator.hpp include/search/search.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/bench_heaps: $(SRC_DIR)/bench_heaps.cpp include/heaps/heaps.hpp include/heaps/binary_heap.cpp include/heaps/binomial_heap.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run_search_size: all
	@bash $(BENCH_DIR)/run_search_size.sh
	@$(MAKE) clean
	@echo "\033[0;32m[Limpieza] Binarios eliminados correctamente tras la ejecucion.\033[0m"

run_search_pos: all
	@bash $(BENCH_DIR)/run_search_pos.sh
	@$(MAKE) clean
	@echo "\033[0;32m[Limpieza] Binarios eliminados correctamente tras la ejecucion.\033[0m"

run_heaps: all
	@bash $(BENCH_DIR)/run_heaps.sh
	@$(MAKE) clean
	@echo "\033[0;32m[Limpieza] Binarios eliminados correctamente tras la ejecucion.\033[0m"

run_all: all
	@bash $(BENCH_DIR)/run_all.sh
	@$(MAKE) clean
	@echo "\033[0;32m[Limpieza] Binarios eliminados correctamente tras la ejecucion completa.\033[0m"

PYTHON = /home/stack/schl/EDAA-Experiments/.venv/bin/python3

plot:
	@$(PYTHON) plots/plot_boletin01.py --all

plot_exp1:
	@$(PYTHON) plots/plot_boletin01.py --exp1

plot_exp2:
	@$(PYTHON) plots/plot_boletin01.py --exp2

plot_exp3:
	@$(PYTHON) plots/plot_boletin01.py --exp3

plot_search:
	@$(PYTHON) plots/plot_boletin01.py --search

plot_heaps:
	@$(PYTHON) plots/plot_boletin01.py --heaps

clean:
	rm -rf $(BIN_DIR)

clean_data:
	rm -f data/search_size/*.csv data/search_pos/*.csv data/heaps/*.csv

clean_plots:
	rm -rf plots/*.png plots/*.pdf plots/experimento_* plots/Experimento_*
