PYTHON ?= $(if $(wildcard ../.venv/bin/python3),$(abspath ../.venv/bin/python3),python3)
TECTONIC ?= tectonic
PLOT_ARGS ?=

.PHONY: all help plot plot_generic report

all: help

help:
	@echo 'make report                         Compila report/Boletin04.pdf'
	@echo "make plot PLOT_ARGS='--help'         Muestra las opciones del graficador"
	@echo 'Las implementaciones y los benchmarks están pendientes de definir.'

plot:
	$(PYTHON) plots/plot_boletin04.py $(PLOT_ARGS)

plot_generic: plot

report:
	cd report && $(TECTONIC) Boletin04.tex
