#!/usr/bin/env python3
"""Valida sesiones RMQ y genera tablas y figuras desde muestras reales."""

import argparse
import json
from pathlib import Path
import shutil
import sys
import tempfile

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from progress_utils import Progress

EXPERIMENTS = ["construction", "static_queries", "maintenance", "dynamic_queries"]
EXPERIMENT_DIRS = {
    "construction": "experimento_1_construccion",
    "static_queries": "experimento_2_consultas_estaticas",
    "maintenance": "experimento_3_mantenimiento",
    "dynamic_queries": "experimento_4_consultas_dinamicas",
}
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


def dynamic_cycles(samples):
    keys = ["method", "n", "seed", "run", "cycle", "input_sha256", "session_id", "config_id"]
    maintenance = samples[samples.experiment == "maintenance"]
    queries = samples[samples.experiment == "dynamic_queries"]
    require(not maintenance.duplicated(keys).any(), "Mantenimiento duplicado en el ciclo")
    require(not queries.duplicated(keys + ["k", "range"]).any(), "Consultas duplicadas en el ciclo")
    require((maintenance.operations == 1).all(), "El ciclo debe contener un solo cambio")
    maintenance = maintenance[keys + ["elapsed_ns"]].rename(columns={"elapsed_ns": "maintenance_ns"})
    queries = queries.groupby(keys, as_index=False).agg(query_ns=("elapsed_ns", "sum"), queries=("operations", "sum"))
    cycles = maintenance.merge(queries, on=keys, how="outer", validate="one_to_one", indicator=True)
    require(not cycles.empty and (cycles._merge == "both").all(), "Ciclo sin mantenimiento o consultas compatibles")
    cycles = cycles.drop(columns="_merge")
    cycles["elapsed_ns"] = cycles.maintenance_ns + cycles.query_ns
    cycles["time_ns"] = cycles.elapsed_ns
    cycles["operations"] = 1
    cycles["updates"] = 1
    cycles["experiment"] = "dynamic_queries"
    cycles["k"] = 0
    cycles["range"] = "cycle"
    return cycles


def summarize(samples):
    # Suma el mantenimiento una sola vez antes de resumir cada repeticion
    cycles = dynamic_cycles(samples)
    samples = pd.concat([samples[samples.experiment != "dynamic_queries"], cycles], ignore_index=True)
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


def average_seeds(frame, keys, columns):
    require(not frame.duplicated(keys + ["seed"]).any(), "Resumen duplicado por semilla")
    aggregations = {"seeds": ("seed", "nunique")}
    for column in columns:
        aggregations[column] = (column, "mean")
        aggregations[column + "_between_seed_sd"] = (column, "std")
    return frame.groupby(keys, as_index=False).agg(**aggregations)


def plot_summaries(summary, memory):
    times = average_seeds(summary, ["experiment", "method", "n", "k", "range"], ["t_mean"])
    space = average_seeds(memory, ["method", "phase", "n"], ["index_bytes", "total_peak_bytes"])
    return times, space


def render(summary, memory, destination, queries_per_cycle=None):
    summary, memory = plot_summaries(summary, memory)
    plt.rcParams.update({"font.family": "serif", "font.size": 11, "axes.grid": True,
                          "axes.grid.axis": "y", "grid.alpha": .22, "axes.spines.top": False,
                          "axes.spines.right": False, "figure.figsize": (8, 5.3),
                          "savefig.bbox": "tight", "savefig.dpi": 160})
    by_size = summary.groupby(["experiment", "range"])
    by_range = summary[summary.experiment == "static_queries"].groupby(["experiment", "n"])
    progress = Progress(by_size.ngroups + by_range.ngroups + 3, "Graficos PNG")

    def figure(frame, x, y, title, filename, subtitle="", unit="Tiempo (ns)", divisor=1):
        fig, ax = plt.subplots()
        try:
            styles = {"sparse_table": ("Sparse Table", "#0072B2", "o"),
                      "segment_tree": ("Segment Tree", "#D55E00", "s")}
            for method, group in frame.groupby("method"):
                group = group.sort_values(x)
                label, color, marker = styles[method]
                mean = group[y].to_numpy() / divisor
                sd = group[y + "_between_seed_sd"].to_numpy() / divisor
                # Una sola semilla no permite estimar dispersion entre semillas
                errors = None if group.seeds.eq(1).all() else [np.minimum(mean, sd), sd]
                ax.errorbar(group[x], mean, yerr=errors, marker=marker, markersize=6,
                            linewidth=1.8, elinewidth=1, capsize=4, color=color, label=label)
            ax.set(xlabel="Elementos del arreglo (n)" if x == "n" else "Longitud del rango (k)", ylabel=unit)
            fig.suptitle(title, fontsize=15, y=.98)
            ax.set_title(subtitle, fontsize=10, color="#555555", pad=12)
            ax.set_xscale("log")
            ax.set_ylim(bottom=0)
            ax.legend(fontsize=10, frameon=False)
            seeds = int(frame.seeds.iloc[0])
            require(frame.seeds.eq(seeds).all(), "Cantidad de semillas distinta entre puntos")
            note = (f"Media de {seeds} medias por semilla · Barras: DE muestral entre esas medias (no IC)"
                    if seeds > 1 else "Una semilla · Sin estimación de dispersión entre semillas")
            fig.text(.5, .015, note, ha="center", fontsize=9, color="#555555")
            fig.subplots_adjust(bottom=.18, top=.83)
            progress.show("Guardando " + filename + '.png')
            fig.savefig(destination / f"{filename}.png")
        finally:
            plt.close(fig)
        progress.advance(filename + " PNG guardado")

    titles = {"construction": "Construcción inicial", "static_queries": "Consultas estáticas",
              "maintenance": "Mantenimiento dinámico", "dynamic_queries": "Costo dinámico por ciclo"}
    number = lambda value: f"{int(value):,}".replace(",", ".")
    for (experiment, range_name), frame in by_size:
        if experiment == "dynamic_queries":
            batch = f"{number(queries_per_cycle)} consultas" if queries_per_cycle is not None else "lote de consultas"
            figure(frame, "n", "t_mean", titles[experiment], f"{experiment}_{range_name}",
                   f"1 cambio + {batch} · Incluye reconstrucción o update", "Tiempo por ciclo (ms)", 1e6)
        elif experiment == "construction":
            figure(frame, "n", "t_mean", titles[experiment], f"{experiment}_{range_name}",
                   "Mismos arreglos de entrada", "Tiempo por construcción (ms)", 1e6)
        elif experiment == "maintenance":
            figure(frame, "n", "t_mean", titles[experiment], f"{experiment}_{range_name}",
                   "Reconstrucción de Sparse Table frente a update de Segment Tree", "Tiempo por cambio (ms)", 1e6)
        else:
            length = {"half": "Rango medio (k = ⌊n/2⌋)", "full": "Rango completo (k = n)"}.get(range_name, f"Longitud del rango k = {range_name}")
            figure(frame, "n", "t_mean", titles[experiment], f"{experiment}_{range_name}", length, "Tiempo por consulta (ns)")
    for (experiment, n), frame in by_range:
        figure(frame, "k", "t_mean", titles[experiment], f"{experiment}_ranges_{n}",
               f"Arreglo de {number(n)} elementos", "Tiempo por consulta (ns)")
    frame = memory[memory.phase == "construction"]
    figure(frame, "n", "index_bytes", "Memoria retenida al construir", "memory_retained", unit="Memoria (MiB)", divisor=2**20)
    for phase in ["construction", "maintenance"]:
        frame = memory[memory.phase == "construction"] if phase == "construction" else memory[memory.phase != "construction"]
        title = "Pico de memoria durante la construcción" if phase == "construction" else "Pico de memoria durante el mantenimiento"
        figure(frame, "n", "total_peak_bytes", title, f"memory_peak_{phase}", unit="Memoria (MiB)", divisor=2**20)


def publish(session, root, metadata, memory, per_run, summary):
    require(metadata["profile"] == "official", "Solo se publican resultados oficiales")
    root = root.resolve()
    progress = Progress(5, "Organizando CSV y graficos por experimento")
    plot_summary, memory_summary = plot_summaries(summary, memory)
    root.mkdir(parents=True, exist_ok=True)
    # Prepara todos los archivos antes de reemplazar la publicacion anterior
    with tempfile.TemporaryDirectory(prefix=".rmq-publish-", dir=root) as temporary:
        staging = Path(temporary)
        replacements = []
        for experiment, directory in EXPERIMENT_DIRS.items():
            data = staging / "data" / directory
            plots = staging / "plots" / directory
            data.mkdir(parents=True)
            plots.mkdir(parents=True)
            if experiment == "dynamic_queries":
                shutil.copy2(session / "plots" / "dynamic_cycles.csv", data / "samples.csv")
                shutil.copy2(session / "dynamic_queries.csv", data / "query_samples.csv")
            else:
                shutil.copy2(session / f"{experiment}.csv", data / "samples.csv")
            summary[summary.experiment == experiment].to_csv(data / "summary.csv", index=False)
            plot_summary[plot_summary.experiment == experiment].to_csv(data / "plot_summary.csv", index=False)
            per_run[per_run.experiment == experiment].to_csv(data / "run_means.csv", index=False)
            if experiment == "static_queries":
                shutil.copy2(session / "calibration.csv", data / "calibration.csv")
            if experiment in ["construction", "maintenance"]:
                selected = memory[memory.phase == "construction"] if experiment == "construction" else memory[memory.phase != "construction"]
                selected.to_csv(data / "memory.csv", index=False)
                selected_summary = memory_summary[memory_summary.phase == "construction"] if experiment == "construction" else memory_summary[memory_summary.phase != "construction"]
                selected_summary.to_csv(data / "memory_plot_summary.csv", index=False)
            names = [f"{experiment}_{name}" for name in summary[summary.experiment == experiment].range.unique()]
            if experiment == "static_queries":
                names += [f"{experiment}_ranges_{n}" for n in summary[summary.experiment == experiment].n.unique()]
            if experiment == "construction":
                names += ["memory_retained", "memory_peak_construction"]
            elif experiment == "maintenance":
                names += ["memory_peak_maintenance"]
            for name in names:
                source = session / "plots" / f"{name}.png"
                require(source.is_file() and source.stat().st_size > 0, f"Falta el grafico {source}")
                shutil.copy2(source, plots / source.name)
            record = {"layout": "rmq-experiments-v1", "source_session": str(session.resolve()),
                      "session_id": metadata["session_id"], "config_id": metadata["config_id"],
                      "experiment": experiment,
                      "analysis": "dynamic-cycle-sum-v1",
                      "plot_aggregation": "equal-weight-seed-means-v1",
                      "error_bars": "sample-standard-deviation-of-seed-means",
                      "unit": "ns_per_cycle" if experiment == "dynamic_queries" else "ns_per_operation"}
            (data / "session.json").write_text(json.dumps(metadata, indent=2) + "\n")
            (data / "COMPLETE").write_text(metadata["config_id"] + "\n")
            for category, prepared in [("data", data), ("plots", plots)]:
                target = root / category / directory
                if target.exists():
                    marker = target / "publication.json"
                    require(marker.is_file() and json.loads(marker.read_text()).get("layout") == record["layout"],
                            f"La carpeta contiene archivos ajenos a esta publicacion: {target}")
                (prepared / "publication.json").write_text(json.dumps(record, indent=2) + "\n")
                replacements.append((prepared, target))
            progress.advance(directory)
        replaced = []
        try:
            for index, (prepared, target) in enumerate(replacements):
                target.parent.mkdir(parents=True, exist_ok=True)
                backup = staging / f"backup-{index}"
                if target.exists():
                    target.rename(backup)
                replaced.append((target, backup))
                prepared.rename(target)
        except OSError:
            for target, backup in reversed(replaced):
                if target.exists():
                    shutil.rmtree(target)
                if backup.exists():
                    backup.rename(target)
            raise
    progress.advance("Publicacion completa")
    print(f"CSV por experimento: {root / 'data' / 'experimento_*'}", flush=True)
    print(f"Graficos por experimento: {root / 'plots' / 'experimento_*'}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", type=Path, required=True)
    parser.add_argument("--staged", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--publish-root", type=Path, help="Publica CSV en data y figuras en plots por experimento")
    args = parser.parse_args()
    metadata, samples, memory = load_session(args.session, args.staged, progress=True)
    if args.publish_root:
        require(metadata["profile"] == "official", "Solo se publican resultados oficiales")
    progress = Progress(6, "Estadisticas y tablas")
    per_run, summary = summarize(samples)
    progress.advance("Estadisticas calculadas")
    destination = args.session / "plots"
    destination.mkdir(exist_ok=True)
    dynamic_cycles(samples).to_csv(destination / "dynamic_cycles.csv", index=False)
    progress.advance("dynamic_cycles.csv con mantenimiento y consultas guardado")
    per_run.to_csv(destination / "run_means.csv", index=False)
    progress.advance("run_means.csv guardado")
    summary.to_csv(destination / "summary.csv", index=False)
    progress.advance("summary.csv guardado")
    memory.to_csv(destination / "memory.csv", index=False)
    progress.advance("memory.csv guardado")
    plot_summary, memory_summary = plot_summaries(summary, memory)
    plot_summary.to_csv(destination / "plot_summary.csv", index=False)
    memory_summary.to_csv(destination / "memory_plot_summary.csv", index=False)
    progress.advance("Promedios y dispersion entre semillas guardados")
    render(summary, memory, destination, metadata["protocol"]["dynamic_queries"])
    if args.publish_root:
        publish(args.session, args.publish_root, metadata, memory, per_run, summary)
    print(f"Validadas {len(samples)} muestras y {len(memory)} registros de memoria")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
