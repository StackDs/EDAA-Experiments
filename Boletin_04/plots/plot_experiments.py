#!/usr/bin/env python3
"""Valida sesiones RMQ y genera tablas y figuras desde muestras reales."""

import argparse
import json
from pathlib import Path
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from progress_utils import Progress

EXPERIMENTS = ["construction", "static_queries", "maintenance", "dynamic_queries"]
METHODS = ["sparse_table", "segment_tree"]
KEY = ["method", "n", "k", "range", "seed", "run", "cycle"]


def ranges(n):
    result = []
    for name, k in [("1", 1), ("8", 8), ("63", 63), ("511", 511), ("half", max(1, n // 2)), ("full", n)]:
        if k <= n and k not in [old[1] for old in result]:
            result.append((name, k))
    return result


def require(condition, message):
    if not condition:
        raise ValueError(message)


def identity(frame, metadata):
    require((frame.session_id == metadata["session_id"]).all(), "Sesiones mezcladas")
    require((frame.config_id == metadata["config_id"]).all(), "Configuraciones mezcladas")


def load_session(path, staged=False, progress=False):
    bar = Progress(len(EXPERIMENTS) + 2, "Validacion CSV") if progress else None
    metadata = json.loads((path / "session.json").read_text())
    if not staged:
        require((path / "COMPLETE").read_text().strip() == metadata["config_id"], "Sesion incompleta")
    for marker in ["MEASURE_COMPLETE", "MEMORY_COMPLETE"]:
        require((path / marker).read_text().strip() == "rmq-v1", "Falta " + marker)
    p, runs = metadata["protocol"], metadata["runs"]
    require(runs >= (32 if metadata["profile"] == "official" else 4), "Repeticiones insuficientes")
    frames = []
    for experiment in EXPERIMENTS:
        if bar:
            bar.show("Validando " + experiment)
        frame = pd.read_csv(path / f"{experiment}.csv", dtype={"range": str, "checksum": str})
        required = set(KEY + ["experiment", "operations", "elapsed_ns", "time_ns", "checksum", "input_sha256", "session_id", "config_id"])
        require(required <= set(frame.columns), "CSV sin columnas requeridas")
        require(not frame.empty and not frame.isna().any().any(), "CSV vacio o con valores ausentes")
        identity(frame, metadata)
        require((frame.experiment == experiment).all(), "Experimentos mezclados")
        require(not frame.duplicated(KEY).any(), "Muestras duplicadas")
        for column in ["n", "k", "seed", "run", "cycle", "operations"]:
            require(np.isfinite(frame[column]).all() and (frame[column] == np.floor(frame[column])).all(), "Indice no entero")
        require(np.isfinite(frame[["elapsed_ns", "time_ns"]]).all().all(), "Tiempos no finitos")
        require((frame[["elapsed_ns", "time_ns"]] >= 0).all().all() and (frame.operations > 0).all(), "Medicion negativa o sin operaciones")
        require(np.allclose(frame.time_ns, frame.elapsed_ns / frame.operations, rtol=1e-12, atol=1e-12), "Normalizacion incorrecta")
        require(frame.input_sha256.str.fullmatch(r"[0-9a-f]{64}").all(), "Hash de entradas invalido")
        require(frame.checksum.str.fullmatch(r"[0-9]{1,20}").all(), "Checksum invalido")
        expected = {}
        for n in p["sizes"]:
            rs = ranges(n)
            for seed in p["seeds"]:
                for method in METHODS:
                    for run in range(runs):
                        if experiment == "construction":
                            expected[(method, n, 0, "none", seed, run, -1)] = 1
                        elif experiment == "static_queries":
                            for name, k in rs:
                                expected[(method, n, k, name, seed, run, -1)] = None
                        else:
                            for cycle in range(p["cycles"]):
                                if experiment == "maintenance":
                                    expected[(method, n, 0, "none", seed, run, cycle)] = 1
                                else:
                                    for j, (name, k) in enumerate(rs):
                                        count = p["dynamic_queries"] // len(rs) + ((j + cycle) % len(rs) < p["dynamic_queries"] % len(rs))
                                        expected[(method, n, k, name, seed, run, cycle)] = count
        actual = dict(zip(frame[KEY].itertuples(index=False, name=None), frame.operations))
        require(actual.keys() == expected.keys(), f"Cobertura incompleta en {experiment}")
        for key, operations in expected.items():
            if operations is not None:
                require(actual[key] == operations, "Cantidad de operaciones incorrecta")
        if experiment == "static_queries":
            require(frame.operations.between(p["static_queries"], p["max_batch"]).all(), "Lote estatico fuera de limites")
            require((frame.groupby(["method", "n", "range", "seed"]).operations.nunique() == 1).all(), "Calibracion variable entre repeticiones")
        if experiment == "dynamic_queries":
            require((frame.groupby(["n", "k", "seed", "run", "cycle"]).checksum.nunique() == 1).all(), "Respuestas dinamicas distintas")
        frames.append(frame)
        if bar:
            bar.advance(experiment + " validado")
    samples = pd.concat(frames, ignore_index=True)
    require((samples.groupby(["n", "seed"]).input_sha256.nunique() == 1).all(), "Entradas incompatibles")
    calibration = pd.read_csv(path / "calibration.csv", dtype={"range": str})
    calibration_key = ["method", "n", "seed", "range", "k"]
    require(not calibration.duplicated(calibration_key).any(), "Calibracion duplicada")
    expected_calibration = {(method, n, seed, name, k) for n in p["sizes"] for seed in p["seeds"]
                            for method in METHODS for name, k in ranges(n)}
    require(set(calibration[calibration_key].itertuples(index=False, name=None)) == expected_calibration, "Calibracion incompleta")
    require(np.isfinite(calibration[["operations", "elapsed_ns", "target_reached"]]).all().all(), "Calibracion no finita")
    require((calibration.elapsed_ns >= 0).all() and calibration.target_reached.isin([0, 1]).all(), "Calibracion invalida")
    require((calibration.target_reached == (calibration.elapsed_ns >= p["target_ns"]).astype(int)).all(), "Objetivo de calibracion inconsistente")
    chosen = samples[samples.experiment == "static_queries"].groupby(calibration_key).operations.first()
    for row in calibration.itertuples(index=False):
        require(chosen.loc[(row.method, row.n, row.seed, row.range, row.k)] == row.operations, "Lote distinto de su calibracion")
    if bar:
        bar.advance("Calibracion validada")
        bar.show("Validando memoria")
    memory = pd.read_csv(path / "memory.csv")
    identity(memory, metadata)
    memory_key = ["method", "phase", "n", "seed"]
    wanted = {(method, phase, n, seed) for n in p["sizes"] for seed in p["seeds"]
              for method, phases in [("sparse_table", ["construction", "reconstruction"]), ("segment_tree", ["construction", "update"])]
              for phase in phases}
    require(not memory.duplicated(memory_key).any(), "Memoria duplicada")
    require(set(memory[memory_key].itertuples(index=False, name=None)) == wanted, "Memoria incompleta")
    numeric = [column for column in memory.columns if column.endswith("bytes")]
    require(np.isfinite(memory[numeric]).all().all() and (memory[numeric] >= 0).all().all(), "Memoria invalida")
    require((memory.total_peak_bytes == memory.baseline_bytes + memory.allocation_peak_bytes).all(), "Pico de memoria inconsistente")
    require((memory.allocation_peak_bytes >= memory.new_retained_bytes).all(), "Memoria retenida mayor que el pico")
    require((memory.index_bytes > 0).all() and (memory.input_bytes >= 16 * memory.n).all(), "Almacenamiento invalido")
    built = memory[memory.phase == "construction"]
    require((built.baseline_bytes == 0).all() and (built.index_bytes == built.new_retained_bytes).all(), "Construccion con memoria previa")
    updates = memory[memory.phase == "update"]
    require((updates.new_retained_bytes == 0).all() and (updates.allocation_peak_bytes == 0).all(), "Update reservo memoria inesperada")
    if bar:
        bar.advance("Memoria y cobertura validadas")
    return metadata, samples, memory


def quartiles(values):
    # Convencion uhr de LELE conservada del boletin 03
    values = np.sort(np.asarray(values, dtype=float))
    n = len(values)
    require(n >= 4, "Faltan repeticiones para resumir")
    if n % 4 >= 2:
        q1, q3 = values[n // 4], values[3 * n // 4]
    else:
        q1 = .25 * values[n // 4 - 1] + .75 * values[n // 4]
        q3 = .75 * values[3 * n // 4 - 1] + .25 * values[3 * n // 4]
    return values[0], q1, np.median(values), q3, values[-1]


def summarize(samples):
    keys = ["experiment", "method", "n", "k", "range", "seed"]
    per_run = samples.groupby(keys + ["run"], as_index=False).agg(elapsed_ns=("elapsed_ns", "sum"), operations=("operations", "sum"))
    per_run["time_ns"] = per_run.elapsed_ns / per_run.operations
    rows = []
    for key, group in per_run.groupby(keys):
        values = group.time_ns.to_numpy()
        row = dict(zip(keys, key))
        row.update(runs=len(values), t_mean=values.mean(), t_stdev=values.std(ddof=1))
        row.update(zip(["t_Q0", "t_Q1", "t_Q2", "t_Q3", "t_Q4"], quartiles(values)))
        rows.append(row)
    return per_run, pd.DataFrame(rows)


def render(summary, memory, destination):
    plt.rcParams.update({"font.family": "serif", "font.size": 10, "axes.grid": True, "grid.alpha": .3,
                          "figure.figsize": (8, 5), "savefig.bbox": "tight", "savefig.dpi": 160})
    by_size = summary.groupby(["experiment", "range"])
    by_range = summary[summary.experiment.isin(["static_queries", "dynamic_queries"])].groupby(["experiment", "n"])
    progress = Progress(by_size.ngroups + by_range.ngroups + 3, "Graficos PNG y PDF")

    def figure(frame, x, y, title, filename, error=False):
        fig, ax = plt.subplots()
        try:
            markers = {seed: ["o", "s", "^"][i % 3] for i, seed in enumerate(sorted(frame.seed.unique()))}
            for (method, seed), group in frame.groupby(["method", "seed"]):
                group = group.sort_values(x)
                color = "C0" if method == "sparse_table" else "C1"
                label = f"{method} semilla {seed}"
                if error:
                    mean, sd = group[y].to_numpy(), group.t_stdev.to_numpy()
                    ax.errorbar(group[x], mean, yerr=[np.minimum(mean, sd), sd], marker=markers[seed], capsize=3, color=color, label=label)
                else:
                    ax.plot(group[x], group[y], marker=markers[seed], color=color, label=label)
            ax.set(xlabel="Elementos n" if x == "n" else "Longitud del rango k", ylabel="ns por operacion" if error else "Bytes solicitados", title=title)
            ax.set_xscale("log")
            ax.set_ylim(bottom=0)
            ax.legend(fontsize=7)
            for extension in ["png", "pdf"]:
                progress.show("Guardando " + filename + '.' + extension)
                fig.savefig(destination / f"{filename}.{extension}")
        finally:
            plt.close(fig)
        progress.advance(filename + " PNG y PDF guardados")

    titles = {"construction": "Construccion inicial", "static_queries": "Consultas estaticas",
              "maintenance": "Mantenimiento dinamico", "dynamic_queries": "Consultas dinamicas"}
    for (experiment, range_name), frame in by_size:
        figure(frame, "n", "t_mean", f"{titles[experiment]} rango {range_name}\nMedia y desviacion estandar",
               f"{experiment}_{range_name}", True)
    for (experiment, n), frame in by_range:
        figure(frame, "k", "t_mean", f"{titles[experiment]} n={n}\nMedia y desviacion estandar", f"{experiment}_ranges_{n}", True)
    frame = memory[memory.phase == "construction"]
    figure(frame, "n", "index_bytes", "Memoria retenida al construir", "memory_retained")
    for phase in ["construction", "maintenance"]:
        frame = memory[memory.phase == "construction"] if phase == "construction" else memory[memory.phase != "construction"]
        figure(frame, "n", "total_peak_bytes", f"Pico de asignaciones {phase}", f"memory_peak_{phase}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", type=Path, required=True)
    parser.add_argument("--staged", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    _, samples, memory = load_session(args.session, args.staged, progress=True)
    progress = Progress(4, "Estadisticas y tablas")
    per_run, summary = summarize(samples)
    progress.advance("Estadisticas calculadas")
    destination = args.session / "plots"
    destination.mkdir(exist_ok=True)
    per_run.to_csv(destination / "run_means.csv", index=False)
    progress.advance("run_means.csv guardado")
    summary.to_csv(destination / "summary.csv", index=False)
    progress.advance("summary.csv guardado")
    memory.to_csv(destination / "memory.csv", index=False)
    progress.advance("memory.csv guardado")
    render(summary, memory, destination)
    print(f"Validadas {len(samples)} muestras y {len(memory)} registros de memoria")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
