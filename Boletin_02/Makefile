CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -Werror -pedantic -Iinclude
PYTHON ?= $(if $(wildcard ../.venv/bin/python3),$(abspath ../.venv/bin/python3),python3)
PLOT_ARGS ?=
TECTONIC ?= tectonic

BIN_DIR = bin
BENCH_DIR = benchmarks
TARGET = $(BIN_DIR)/bench_trees
HEADERS = $(wildcard include/trees/*.h include/utils/*.h)

.PHONY: all run_basic_ops run_distributions run_all plot report clean

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $@

$(TARGET): src/bench_trees.cpp $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

run_basic_ops: all
	bash $(BENCH_DIR)/run_basic_ops.sh

run_distributions: all
	bash $(BENCH_DIR)/run_distributions.sh

run_all: all
	bash $(BENCH_DIR)/run_all.sh

plot:
	$(PYTHON) plots/plot_boletin02.py $(PLOT_ARGS)

report:
	cd report && $(TECTONIC) Boletin02.tex

clean:
	rm -f $(TARGET)
