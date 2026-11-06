CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -pedantic -Iinclude
DEPFLAGS = -MMD -MP
PYTHON ?= $(if $(wildcard ../.venv/bin/python3),$(abspath ../.venv/bin/python3),python3)
TECTONIC ?= tectonic
PLOT_ARGS ?=
SESSION ?= $(if $(DATA_ROOT),$(DATA_ROOT),data)
HEADERS := $(wildcard include/string_matching/*.hpp include/utils/*.hpp)
BENCHES := bin/bench_text_size bin/bench_pattern_size bin/bench_occurrences
export CXX CXXFLAGS

.NOTPARALLEL:
.PHONY: all build check prepare run_text_size run_pattern_size run_occurrences run_all plot plot_generic report clean FORCE

all: run_all

build: bin/prepare_patterns $(BENCHES)

bin:
	mkdir -p $@

bin/build-config.json: FORCE scripts/build_config.py | bin
	@$(PYTHON) scripts/build_config.py --cxx="$(CXX)" --flags="$(CXXFLAGS)" --output $@

bin/prepare_patterns: src/prepare_patterns.cpp $(HEADERS) bin/build-config.json | bin
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $< -o $@

bin/memory_tracking.o: src/memory_tracking.cpp $(HEADERS) bin/build-config.json | bin
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

bin/bench_%: src/bench_%.cpp bin/memory_tracking.o $(HEADERS) bin/build-config.json | bin
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $< bin/memory_tracking.o -o $@

check:
	@for header in include/string_matching/*.hpp; do \
		printf '#include "%s"\n' "$${header#include/}" | \
		$(CXX) $(CXXFLAGS) -x c++ -fsyntax-only - || exit 1; \
	done

prepare: build
	PYTHON="$(PYTHON)" bash benchmarks/common.sh prepare

run_text_size: build
	PYTHON="$(PYTHON)" bash benchmarks/run_text_size.sh

run_pattern_size: build
	PYTHON="$(PYTHON)" bash benchmarks/run_pattern_size.sh

run_occurrences: build
	PYTHON="$(PYTHON)" bash benchmarks/run_occurrences.sh

# Evitamos efectos reales con -n, incluso al usar Make recursivamente.
run_all:
	@case "$${MAKEFLAGS%% *}" in *n*) exit 0 ;; esac; \
	exec 9>texts/.benchmark.lock; \
	flock -n 9 || { echo "Ya hay otra preparacion o medicion activa" >&2; exit 1; }; \
	trap '$(MAKE) --no-print-directory clean' EXIT; \
	$(MAKE) --no-print-directory build && \
	BENCHMARK_LOCK_FD=9 PYTHON="$(PYTHON)" bash benchmarks/run_all.sh

plot:
	$(PYTHON) plots/plot_experiments.py --session "$(SESSION)" $(PLOT_ARGS)

plot_generic:
	$(PYTHON) plots/plot_boletin03.py $(PLOT_ARGS)

report:
	cd report && $(TECTONIC) Boletin03.tex

clean:
	rm -f $(BENCHES) bin/prepare_patterns bin/memory_tracking.o bin/build-config.json bin/*.d
	rm -f plots/__pycache__/*.py[co] scripts/__pycache__/*.py[co]
	@rmdir bin plots/__pycache__ scripts/__pycache__ 2>/dev/null || true

-include $(wildcard bin/*.d)
