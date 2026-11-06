CXX = g++
SHELL := /bin/bash
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -Wpedantic -Iinclude
PYTHON ?= $(if $(wildcard ../.venv/bin/python3),$(abspath ../.venv/bin/python3),python3)
TECTONIC ?= tectonic
CPU_CORE ?= 2
PILOT ?=
SESSION ?=
BENCH_ARGS ?=
PLOT_ARGS ?=
HEADERS := $(wildcard include/utils/*.hpp include/range_queries/*.hpp)
export CXX CXXFLAGS
export PYTHONDONTWRITEBYTECODE = 1

.NOTPARALLEL:
.PHONY: all build help check pilot run_all plot plot_generic plot_session report clean FORCE

all:
	+@case "$${MAKEFLAGS%% *}" in *n*) exit 0 ;; esac; \
	set -e; \
	source benchmarks/progress.sh; \
	FLOW_START_CS="$$(uptime_cs)"; FLOW_DONE=0; \
	mkdir -p data; \
	exec 9>data/.benchmark.lock; \
	flock -n 9 || { echo 'Ya hay otra preparacion o medicion activa'; exit 1; }; \
	cleanup() { \
		local status=$$?; \
		display_progress "$$FLOW_DONE" 3 "$$FLOW_START_CS" 'Flujo completo | Limpieza de compilacion'; \
		if $(MAKE) --no-print-directory clean; then \
			if [[ $$status -eq 0 ]]; then \
				display_progress 3 3 "$$FLOW_START_CS" 'Flujo completo | Resultados y graficos PNG listos'; \
			else echo "Flujo interrumpido con codigo $$status" >&2; fi; \
		else \
			if [[ $$status -eq 0 ]]; then status=1; fi; \
		fi; \
		exit "$$status"; \
	}; \
	trap cleanup EXIT; \
	display_progress 0 3 "$$FLOW_START_CS" 'Flujo completo | Compilando ejecutables'; \
	$(MAKE) --no-print-directory build; \
	FLOW_DONE=1; \
	display_progress 1 3 "$$FLOW_START_CS" 'Flujo completo | Piloto bateria memoria y graficos'; \
	BENCHMARK_LOCK_FD=9 PILOT="$(PILOT)" PYTHON="$(PYTHON)" \
		bash benchmarks/run_all.sh --cpu-core "$(CPU_CORE)" $(BENCH_ARGS); \
	FLOW_DONE=2

help:
	@echo 'make all                    Compila ejecuta piloto bateria graficos PNG y limpia'
	@echo 'make build                  Compila sin ejecutar experimentos'
	@echo 'make check                  Verifica en temporales y los elimina'
	@echo 'make pilot                  Estima la matriz con cuatro repeticiones'
	@echo 'make run_all PILOT=ruta      Ejecuta 32 repeticiones tras validar el piloto'
	@echo 'make plot_session SESSION=ruta'
	@echo 'make report'

build: bin/bench_rmq bin/bench_memory

bin:
	mkdir -p $@

bin/build-config.json: FORCE scripts/build_config.py | bin
	$(PYTHON) scripts/build_config.py --cxx="$(CXX)" --flags="$(CXXFLAGS)" --output $@

bin/bench_rmq: src/bench_rmq.cpp $(HEADERS) bin/build-config.json | bin
	$(CXX) $(CXXFLAGS) -MMD -MP $< -o $@

bin/bench_memory: src/bench_memory.cpp src/memory_tracking.cpp $(HEADERS) bin/build-config.json | bin
	$(CXX) $(CXXFLAGS) src/bench_memory.cpp src/memory_tracking.cpp -o $@

check: build
	$(PYTHON) scripts/check.py

pilot: build
	PYTHON="$(PYTHON)" bash benchmarks/run_pilot.sh --cpu-core "$(CPU_CORE)" $(BENCH_ARGS)

run_all: build
	@test -n "$(PILOT)" || { echo 'Indique PILOT con la ruta de un piloto completo'; exit 1; }
	PYTHON="$(PYTHON)" bash benchmarks/run_all.sh --cpu-core "$(CPU_CORE)" --pilot "$(PILOT)" $(BENCH_ARGS)

plot:
	$(PYTHON) plots/plot_boletin04.py $(PLOT_ARGS)

plot_generic: plot

plot_session:
	$(PYTHON) plots/plot_experiments.py --session "$(SESSION)" --publish-root . $(PLOT_ARGS)

report:
	cd report && $(TECTONIC) Boletin04.tex

clean:
	rm -f bin/bench_rmq bin/bench_memory bin/build-config.json bin/bench_rmq.d
	@rmdir bin 2>/dev/null || true

-include bin/bench_rmq.d
