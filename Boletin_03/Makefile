CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -pedantic -Iinclude
PYTHON ?= $(if $(wildcard ../.venv/bin/python3),$(abspath ../.venv/bin/python3),python3)
TECTONIC ?= tectonic
PLOT_ARGS ?=

.PHONY: all check plot report

all: check

check:
	@printf '%s\n' \
		'#include <cstdint>' \
		'#include <string_view>' \
		'#include "string_matching/suffix_array.hpp"' \
		'#include "string_matching/brute_force.hpp"' \
		'#include "string_matching/kmp.hpp"' \
		'#include "string_matching/rabin_karp.hpp"' | \
		$(CXX) $(CXXFLAGS) -x c++ -fsyntax-only -

plot:
	$(PYTHON) plots/plot_boletin03.py $(PLOT_ARGS)

report:
	cd report && $(TECTONIC) Boletin03.tex
