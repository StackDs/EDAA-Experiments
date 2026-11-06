#!/usr/bin/env python3
"""Grafica muestras reales seleccionando explícitamente ejes, series y facetas."""

import argparse
import re
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from plot_utils import groups, load_samples, summarize
from progress_utils import Progress


BASE = Path(__file__).resolve().parents[1]
MARKERS = ("o", "s", "^", "D", "v", "P", "X")


def label(columns, values):
    return ", ".join(f"{column}={value}" for column, value in zip(columns, values))


def slug(value):
    return re.sub(r"[^a-zA-Z0-9_-]+", "_", value).strip("_") or "value"


def render(summary, args):
    plt.rcParams.update({
        "font.family": "serif",
        "font.size": 11,
        "axes.grid": True,
        "grid.alpha": 0.3,
        "grid.linestyle": "--",
        "figure.figsize": (8, 5.2),
        "savefig.dpi": 200,
        "savefig.bbox": "tight",
    })
    styles = {
        key: (f"C{index % 10}", MARKERS[index % len(MARKERS)])
        for index, (key, _) in enumerate(groups(summary, args.group_by))
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    panels = list(groups(summary, args.facet_by))
    progress = Progress(len(panels) + 1, "Graficador generico")
    for number, (facet, panel) in enumerate(panels, start=1):
        figure, axis = plt.subplots()
        try:
            for key, series in groups(panel, args.group_by):
                series = series.sort_values(args.x)
                median = series["median"].to_numpy(dtype=float)
                color, marker = styles[key]
                axis.errorbar(
                    series[args.x].to_numpy(dtype=float),
                    median,
                    yerr=[median - series["q1"].to_numpy(dtype=float),
                          series["q3"].to_numpy(dtype=float) - median],
                    label=label(args.group_by, key),
                    color=color,
                    marker=marker,
                    capsize=3,
                    linewidth=1.6,
                )
            title = args.title or f"{args.y} frente a {args.x}"
            if facet:
                title += "\n" + label(args.facet_by, facet)
            axis.set_title(title)
            axis.set_xlabel(args.xlabel or args.x)
            axis.set_ylabel(args.ylabel or f"{args.y}: mediana e intervalo [Q1, Q3]")
            axis.set_xscale(args.xscale)
            axis.set_yscale(args.yscale)
            if args.yscale == "linear":
                axis.set_ylim(bottom=0)
            axis.legend(loc="best")
            filename = f"fig_{slug(args.y)}_vs_{slug(args.x)}_{number:03d}"
            destination = args.output_dir / f"{filename}.png"
            progress.show("Guardando " + destination.name)
            figure.savefig(destination)
        finally:
            plt.close(figure)
        progress.advance(str(destination))
    summary.to_csv(args.output_dir / "summary.csv", index=False)
    progress.advance("summary.csv guardado")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", type=Path, help="CSV de muestras crudas o directorios")
    parser.add_argument("--x", required=True, help="Columna numérica del eje X")
    parser.add_argument("--y", required=True, help="Columna numérica medida, con unidades explícitas")
    parser.add_argument("--group-by", nargs="+", default=["method"], help="Columnas que distinguen series")
    parser.add_argument("--facet-by", nargs="+", default=[], help="Columnas que separan figuras")
    parser.add_argument("--xscale", choices=("linear", "log"), default="linear")
    parser.add_argument("--yscale", choices=("linear", "log"), default="linear")
    parser.add_argument("--xlabel")
    parser.add_argument("--ylabel")
    parser.add_argument("--title")
    parser.add_argument("--output-dir", type=Path, default=BASE / "plots" / "generated")
    args = parser.parse_args(argv)
    try:
        progress = Progress(2, "Lectura y resumen")
        samples = load_samples(args.inputs, args.x, args.y, args.group_by, args.facet_by)
        progress.advance("Muestras leidas")
        for column, scale in ((args.x, args.xscale), (args.y, args.yscale)):
            if scale == "log" and (samples[column] <= 0).any():
                raise ValueError(f"La escala logarítmica requiere {column} > 0")
        summary = summarize(samples, args.x, args.y, args.group_by, args.facet_by)
        progress.advance("Resumen calculado")
        render(summary, args)
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
