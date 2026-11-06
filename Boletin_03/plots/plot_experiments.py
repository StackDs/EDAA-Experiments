#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, ScalarFormatter
import numpy as np
import pandas as pd

BASE = Path(__file__).resolve().parents[1]
METHODS = ("brute_force", "kmp", "rabin_karp", "suffix_array")
LABELS = {"brute_force": "Fuerza bruta", "kmp": "KMP",
          "rabin_karp": "Rabin–Karp", "suffix_array": "Suffix Array"}
CONTROLS = ["experiment", "dataset", "n", "m", "z", "pattern_id", "method", "phase",
            "corpus_sha256", "session_id", "config_id"]
EXPERIMENTS = ("text_size", "pattern_size", "occurrences")
EXPERIMENT_DIRS = {
    "text_size": "experimento_1_largo_texto",
    "pattern_size": "experimento_2_largo_patron",
    "occurrences": "experimento_3_numero_ocurrencias",
}
EXPERIMENT_TITLES = {
    "text_size": "Experimento 1: variación del largo del texto",
    "pattern_size": "Experimento 2: variación del largo del patrón",
    "occurrences": "Experimento 3: variación del número de ocurrencias",
}
UNARY_DIR = Path(EXPERIMENT_DIRS["occurrences"]) / "casos_unarios"
FIGURES = {
    "text_size": Path(EXPERIMENT_DIRS["text_size"]) / "tiempo_consulta_vs_largo_texto",
    "text_size_memory": Path(EXPERIMENT_DIRS["text_size"]) / "memoria_auxiliar_consulta_vs_largo_texto",
    "construction_text_size_english":
        Path(EXPERIMENT_DIRS["text_size"]) / "tiempo_construccion_suffix_array_vs_largo_texto",
    "index_memory_text_size_english":
        Path(EXPERIMENT_DIRS["text_size"]) / "memoria_indice_y_construccion_suffix_array_vs_largo_texto",
    "pattern_size": Path(EXPERIMENT_DIRS["pattern_size"]) / "tiempo_consulta_vs_largo_patron",
    "pattern_size_memory": Path(EXPERIMENT_DIRS["pattern_size"]) / "memoria_auxiliar_consulta_vs_largo_patron",
    "index_memory_pattern_size_english":
        Path(EXPERIMENT_DIRS["pattern_size"]) / "memoria_indice_y_construccion_suffix_array_texto_fijo",
    "occurrences": Path(EXPERIMENT_DIRS["occurrences"]) / "tiempo_consulta_vs_numero_ocurrencias",
    "index_memory_occurrences_english":
        Path(EXPERIMENT_DIRS["occurrences"]) / "memoria_indice_y_construccion_suffix_array_texto_fijo",
    "unary_A": UNARY_DIR / "tiempo_consulta_patron_A_presente_vs_largo_texto",
    "unary_B": UNARY_DIR / "tiempo_consulta_patron_B_ausente_vs_largo_texto",
    "index_memory_occurrences_unary":
        UNARY_DIR / "memoria_indice_y_construccion_suffix_array_vs_largo_texto_unario",
}
LEGACY_FIGURES = ("text_size", "text_size_memory", "pattern_size", "pattern_size_memory",
                  "occurrences", "unary_A", "unary_B") + tuple(
    f"{kind}_{experiment}_{dataset}"
    for kind in ("construction", "index_memory")
    for experiment in EXPERIMENTS for dataset in ("english", "unary"))

plt.rcParams.update({
    "font.family": "serif",
    "font.size": 11,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "grid.linestyle": "--",
    "figure.figsize": (8, 5.2),
    "savefig.dpi": 300,
    "savefig.bbox": "tight",
})


def uhr(values):
    data = np.sort(np.asarray(values, dtype=float))
    n = len(data)
    if n < 4 or not np.isfinite(data).all() or (data < 0).any():
        raise ValueError("Muestras insuficientes o invalidas")
    if n % 4 >= 2:
        q1, q3 = data[n // 4], data[3 * n // 4]
    else:
        q1 = .25 * data[n // 4 - 1] + .75 * data[n // 4]
        q3 = .75 * data[3 * n // 4 - 1] + .25 * data[3 * n // 4]
    return dict(runs=n, t_mean=data.mean(), t_stdev=data.std(ddof=1),
                t_Q0=data[0], t_Q1=q1, t_Q2=np.median(data), t_Q3=q3, t_Q4=data[-1])


def summarize(frame, keys, value="time_ns"):
    rows = []
    for group, samples in frame.groupby(keys, sort=True, dropna=False):
        if not isinstance(group, tuple):
            group = (group,)
        rows.append(dict(zip(keys, group)) | uhr(samples[value]))
    return pd.DataFrame(rows)


def read_config(path):
    return dict(line.split("=", 1) for line in path.read_text().splitlines() if line)


def experiment_directory(root, experiment):
    directory = Path(root) / EXPERIMENT_DIRS[experiment]
    legacy = Path(root) / experiment
    return legacy if not directory.exists() and legacy.exists() else directory


def load_session(session):
    session = Path(session)
    all_metadata = {}
    frames, memories = [], []
    for experiment in EXPERIMENTS:
        directory = experiment_directory(session, experiment)
        if not directory.exists():
            continue
        if not (directory / "COMPLETE").is_file():
            raise ValueError(f"Experimento incompleto {experiment}")
        metadata_path = directory / "session.json"
        if not metadata_path.is_file():
            metadata_path = session / "session.json"
        metadata = json.loads(metadata_path.read_text())
        if metadata["profile"] != "official":
            raise ValueError("Solo se admiten sesiones del protocolo oficial")
        if int(metadata["runs"]) < 32:
            raise ValueError("Se requieren al menos 32 repeticiones")
        all_metadata[experiment] = metadata
        config = read_config(directory / "config.txt")
        expected_timing = "repeated" if experiment == "text_size" else "setup_only"
        if config.get("construction_timing") != expected_timing:
            raise ValueError("Medicion de construccion incompatible con el protocolo")
        config_id = config["config_id"]
        config_text = (directory / "config.txt").read_text().rsplit("config_id=", 1)[0]
        if hashlib.sha256(config_text.encode()).hexdigest() != config_id:
            raise ValueError("La configuracion fue modificada")
        if hashlib.sha256((directory / "patterns.csv").read_bytes()).hexdigest() != config["manifest_sha256"]:
            raise ValueError("El manifiesto de patrones fue modificado")
        manifest = pd.read_csv(directory / "patterns.csv", keep_default_na=False)
        if (directory / "COMPLETE").read_text().strip() != config_id:
            raise ValueError("Marca de finalizacion incompatible")
        if config["profile"] != metadata["profile"] or int(config["runs"]) != int(metadata["runs"]):
            raise ValueError("Configuraciones de sesion incompatibles")
        frame = pd.read_csv(directory / "samples.csv", keep_default_na=False)
        required = CONTROLS + ["run", "batch_size", "time_ns"]
        if not set(required).issubset(frame.columns) or frame.empty:
            raise ValueError("CSV de muestras incompleto")
        if set(frame["experiment"]) != {experiment} or set(frame["config_id"]) != {config_id}:
            raise ValueError("Experimentos o configuraciones mezcladas")
        if set(frame["session_id"]) != {metadata.get("session_id", session.name)}:
            raise ValueError("Sesiones mezcladas")
        if not set(frame["phase"]).issubset({"query", "construction"}):
            raise ValueError("Fase desconocida")
        for column in ("n", "m", "z", "run", "batch_size", "time_ns"):
            frame[column] = pd.to_numeric(frame[column], errors="raise")
            values = frame[column].to_numpy(dtype=float)
            if not np.isfinite(values).all() or (values < 0).any():
                raise ValueError(f"Columna invalida {column}")
            if column != "time_ns" and not np.equal(values, np.floor(values)).all():
                raise ValueError("Los controles deben ser enteros")
        if (frame["batch_size"] < 1).any() or (frame["n"] < 1).any():
            raise ValueError("Tamano de lote o texto invalido")
        if frame.duplicated(CONTROLS + ["run"]).any():
            raise ValueError("Repeticiones duplicadas")
        for _, samples in frame.groupby(CONTROLS, dropna=False):
            if set(samples["run"]) != set(range(int(config["runs"]))):
                raise ValueError("Faltan repeticiones")
            if samples["batch_size"].nunique() != 1:
                raise ValueError("El lote cambia entre repeticiones")
        queries = frame[frame["phase"] == "query"]
        if queries.empty or (queries["m"] < 1).any():
            raise ValueError("Consultas ausentes o vacias")
        for _, samples in queries.groupby(["dataset", "n", "pattern_id"]):
            if set(samples["method"]) != set(METHODS):
                raise ValueError("Falta un metodo")
            if any(samples[column].nunique() != 1 for column in ("m", "z", "corpus_sha256")):
                raise ValueError("Los metodos no comparten los controles")
        english = queries[queries["dataset"] == "english"]
        if set(english["pattern_id"]) != set(manifest["pattern_id"]):
            raise ValueError("Faltan patrones del manifiesto")
        for _, part in english.groupby(["n", "method"]):
            if set(part["pattern_id"]) != set(manifest["pattern_id"]):
                raise ValueError("No se midieron los mismos patrones en todos los tamanos")
        manifest_controls = manifest[["pattern_id", "m", "z"]].drop_duplicates()
        comparison = english[["pattern_id", "m", "z"]].drop_duplicates().merge(
            manifest_controls, on=["pattern_id", "m", "z"], how="outer", indicator=True)
        if not (comparison["_merge"] == "both").all():
            raise ValueError("Los conteos no coinciden con el manifiesto")
        if experiment == "text_size":
            expected = {int(n) for n in config["sizes"].strip(",").split(",")}
            if set(english["n"]) != expected or set(english["m"]) != {32} or set(english["z"]) != {1}:
                raise ValueError("Controles de largo del texto incompatibles")
        elif experiment == "pattern_size":
            expected = {int(m) for m in config["lengths"].strip(",").split(",")}
            if set(english["m"]) != expected or set(english["n"]) != {int(config["size"])} or set(english["z"]) != {1}:
                raise ValueError("Controles de largo del patron incompatibles")
        else:
            if set(english["m"]) != {4} or set(english["n"]) != {int(config["size"])}:
                raise ValueError("Controles de ocurrencias incompatibles")
            unary = queries[queries["dataset"] == "unary"]
            if set(unary["m"]) != {1} or set(unary["pattern_id"]) != {"A", "B"}:
                raise ValueError("Faltan los casos unarios")
            expected = {int(n) for n in config["sizes"].strip(",").split(",")}
            if set(unary["n"]) != expected:
                raise ValueError("Faltan tamanos unarios")
            for (_, n), part in unary.groupby(["method", "n"]):
                if set(part["pattern_id"]) != {"A", "B"}:
                    raise ValueError("Falta un caso extremo")
                if set(part[part["pattern_id"] == "A"]["z"]) != {n} or set(part[part["pattern_id"] == "B"]["z"]) != {0}:
                    raise ValueError("Conteo unario incompatible")
        builds = frame[frame["phase"] == "construction"]
        query_texts = set(map(tuple, queries[["dataset", "n", "corpus_sha256"]].drop_duplicates().to_numpy()))
        if experiment == "text_size":
            if builds.empty or set(builds["method"]) != {"suffix_array"} or (builds["batch_size"] != 1).any():
                raise ValueError("Construccion incompatible")
            if set(builds["m"]) != {0} or set(builds["z"]) != {0} or set(builds["pattern_id"]) != {""}:
                raise ValueError("La construccion no es una consulta")
            build_texts = set(map(tuple, builds[["dataset", "n", "corpus_sha256"]].drop_duplicates().to_numpy()))
            if query_texts != build_texts:
                raise ValueError("Falta la construccion de algun texto")
        elif not builds.empty:
            raise ValueError("La construccion solo se cronometra en el experimento del texto")
        maximum = english[english["n"] == int(config["size"])]
        if set(maximum["corpus_sha256"]) != {config["corpus_sha256"]}:
            raise ValueError("Hash del corpus incompatible")
        memory = pd.read_csv(directory / "memory.csv", keep_default_na=False)
        columns = ["auxiliary_peak_bytes", "index_bytes", "construction_peak_bytes", "construction_temporary_bytes"]
        if not set(CONTROLS + columns).issubset(memory.columns):
            raise ValueError("Memoria incompleta")
        if memory.duplicated(CONTROLS).any():
            raise ValueError("Memoria duplicada")
        # La memoria del indice tambien se registra cuando solo se prepara para consultar
        indexes = queries[CONTROLS].drop_duplicates(["dataset", "n", "corpus_sha256"]).copy()
        indexes["m"], indexes["z"], indexes["pattern_id"] = 0, 0, ""
        indexes["method"], indexes["phase"] = "suffix_array", "construction"
        expected_memory = pd.concat([queries[CONTROLS].drop_duplicates(), indexes], ignore_index=True)
        joined = expected_memory.merge(memory[CONTROLS], on=CONTROLS, how="outer", indicator=True)
        if not (joined["_merge"] == "both").all():
            raise ValueError("Memoria y consultas incompatibles")
        for column in columns:
            memory[column] = pd.to_numeric(memory[column], errors="raise")
            if not np.isfinite(memory[column]).all() or (memory[column] < 0).any():
                raise ValueError("Bytes de memoria invalidos")
        frames.append(frame)
        memories.append(memory)
    if not frames:
        raise ValueError("La sesion no contiene experimentos")
    all_samples = pd.concat(frames, ignore_index=True)
    for _, part in all_samples.groupby(["dataset", "n"]):
        if part["corpus_sha256"].nunique() != 1:
            raise ValueError("Se mezclaron textos con hashes diferentes")
    return all_samples, pd.concat(memories, ignore_index=True), all_metadata


def format_x_axis(axis, x, values):
    values = np.sort(np.unique(values))
    if x == "z":
        # Conservamos los patrones ausentes (z=0) sin amontonar las frecuencias pequenas.
        axis.set_xscale("symlog", linthresh=1)
        maximum = max(float(values.max()), 1)
        ticks = [0] + [10 ** power for power in range(int(np.log10(maximum)) + 1)]
        axis.set_xticks(ticks)
        axis.xaxis.set_major_formatter(ScalarFormatter())
    else:
        if x == "m":
            axis.set_xscale("log", base=2)
        axis.set_xticks(values)
        if x == "n":
            axis.xaxis.set_major_formatter(FuncFormatter(lambda value, _position: f"{value / 1e6:g}"))
        else:
            axis.xaxis.set_major_formatter(ScalarFormatter())


def render(frame, x, destination, xlabel, title, ylabel="Tiempo [ns/consulta]", points=False,
           memory=False):
    figure, axis = plt.subplots()
    try:
        zero_methods = []
        for method, series in frame.groupby("method", sort=False):
            series = series.sort_values(x)
            mean = series["t_mean"].to_numpy(dtype=float)
            index = METHODS.index(method)
            style = dict(label=LABELS[method], color=f"C{index}",
                         marker=("o", "s", "^", "D")[index], markerfacecolor="none",
                         linestyle="none" if points else ("-", "--", ":", "-.")[index])
            if memory:
                axis.plot(series[x], mean, **style)
                if (mean == 0).all():
                    zero_methods.append(LABELS[method])
            else:
                sd = series["t_stdev"].to_numpy(dtype=float)
                axis.errorbar(series[x], mean, yerr=[np.minimum(sd, mean), sd], capsize=3, **style)
        format_x_axis(axis, x, frame[x])
        if memory:
            note = "Reservas dinámicas por consulta; excluye entradas, pila e índice preconstruido."
            if zero_methods:
                note += "\nComparten la curva de 0 bytes: " + ", ".join(zero_methods) + "."
        else:
            if (frame["t_mean"] - frame["t_stdev"] > 0).all():
                axis.set_yscale("log")
                scale = "logarítmica"
            else:
                positive = frame.loc[frame["t_mean"] > 0, "t_mean"]
                threshold = float(positive.min()) / 10 if not positive.empty else 1
                axis.set_yscale("symlog", linthresh=threshold)
                scale = "symlog (lineal junto a cero)"
            note = f"Media ± desviación estándar; eje Y en escala {scale}."
            if x == "z":
                note += "\nEje X symlog: admite cero; los patrones no se unen como una trayectoria."
        if x == "m":
            xlabel += " (escala log₂)"
        axis.set(xlabel=xlabel, ylabel=ylabel, title=title)
        # El margen tambien se aplica a los extremos de las barras de error y a y=0.
        axis.margins(x=.07, y=.15)
        axis.grid(alpha=.3, linestyle="--")
        axis.legend(loc="upper left", bbox_to_anchor=(1.02, 1), borderaxespad=0, fontsize=9)
        figure.text(.5, .02, note, ha="center", va="bottom", fontsize=8)
        figure.tight_layout(rect=(0, .14 if "\n" in note else .09, 1, 1))
        figure.savefig(destination.with_suffix(".png"))
    finally:
        plt.close(figure)


def render_index_memory(frame, destination, dataset, title=None):
    figure, axis = plt.subplots()
    try:
        ordered = frame.sort_values("n")
        metrics = (("index_bytes", "Índice persistente"),
                   ("construction_peak_bytes", "Pico total de construcción"),
                   ("construction_temporary_bytes", "Pico temporal de construcción"))
        corpus = "inglés" if dataset == "english" else "unario"
        if ordered["n"].nunique() == 1:
            values = [float(ordered.iloc[0][column]) / 1e6 for column, _label in metrics]
            bars = axis.bar(["Índice\npersistente", "Pico total de\nconstrucción",
                             "Pico temporal de\nconstrucción"], values, color=["C0", "C1", "C2"])
            axis.bar_label(bars, labels=[f"{value:g} MB" for value in values], padding=5)
            n = int(ordered.iloc[0]["n"])
            axis.set_xlabel(f"Texto fijo de {n / 1e6:g} MB — {corpus}")
        else:
            for index, (column, label) in enumerate(metrics):
                axis.plot(ordered["n"], ordered[column] / 1e6,
                          marker=("o", "s", "^")[index], linestyle=("-", "--", ":")[index],
                          label=label)
            format_x_axis(axis, "n", ordered["n"])
            axis.set_xlabel(f"Largo del texto [MB] — {corpus}")
            axis.legend(loc="upper left", bbox_to_anchor=(1.02, 1), borderaxespad=0, fontsize=9)
        axis.set(ylabel="Memoria dinámica [MB; 1 MB = 10⁶ bytes]",
                 title=title or "Memoria del Suffix Array: índice y construcción")
        axis.margins(x=.07, y=.18)
        axis.grid(axis="y", alpha=.3, linestyle="--")
        figure.text(.5, .02,
                    "Magnitudes solapadas: no se suman; el pico total ya incluye índice y temporales.\n"
                    "Excluye entradas compartidas, pila y sobrecoste del asignador.",
                    ha="center", va="bottom", fontsize=8)
        figure.tight_layout(rect=(0, .14, 1, 1))
        figure.savefig(destination.with_suffix(".png"))
    finally:
        plt.close(figure)


def generate(session, output=None):
    samples, memory, _metadata = load_session(session)
    session = Path(session)
    output = Path(output) if output else BASE / "plots"
    output.mkdir(parents=True, exist_ok=True)
    active = set(samples["experiment"])
    active_directories = {EXPERIMENT_DIRS[name] for name in active}
    # Las mediciones independientes no eliminan figuras de otros experimentos.
    figures = [path for path in FIGURES.values() if path.parts[0] in active_directories]
    legacy = [Path(name) for name in LEGACY_FIGURES if any(
        name == experiment or name.startswith(experiment + "_") or f"_{experiment}_" in name
        or (experiment == "occurrences" and name.startswith("unary_")) for experiment in active)]
    for name in legacy + figures:
        for suffix in (".png", ".pdf"):
            (output / name.with_suffix(suffix)).unlink(missing_ok=True)
    for experiment in active:
        (output / f"{experiment}_summary.csv").unlink(missing_ok=True)
        (session / f"{experiment}_summary.csv").unlink(missing_ok=True)

    def save_summary(experiment, stats):
        directory = experiment_directory(session, experiment)
        directory.mkdir(parents=True, exist_ok=True)
        stats.to_csv(directory / "plot_summary.csv", index=False)

    def destination(name):
        path = output / FIGURES[name]
        path.parent.mkdir(parents=True, exist_ok=True)
        return path

    queries = samples[samples["phase"] == "query"]
    for experiment, x, label in (("text_size", "n", "Largo del texto [MB]"),
                                  ("pattern_size", "m", "Largo del patrón [bytes]")):
        part = queries[(queries["experiment"] == experiment) & (queries["dataset"] == "english")]
        if part.empty:
            continue
        # Promediamos patrones con igual peso dentro de cada repeticion
        means = part.groupby([x, "method", "run"], as_index=False)["time_ns"].mean()
        stats = summarize(means, [x, "method"])
        save_summary(experiment, stats)
        title = EXPERIMENT_TITLES[experiment]
        render(stats, x, destination(experiment), label,
               title + "\nTiempo de consulta — texto en inglés")
        mem = memory[(memory["experiment"] == experiment) & (memory["phase"] == "query")]
        mem = mem.groupby([x, "method"], as_index=False)["auxiliary_peak_bytes"].max()
        mem = mem.rename(columns={"auxiliary_peak_bytes": "t_mean"})
        render(mem, x, destination(f"{experiment}_memory"), label,
               title + "\nMemoria dinámica adicional por consulta — texto en inglés",
               ylabel="Pico dinámico adicional [bytes]", memory=True)
    occurrences = queries[(queries["experiment"] == "occurrences") & (queries["dataset"] == "english")]
    if not occurrences.empty:
        stats = summarize(occurrences, ["z", "pattern_id", "method"])
        save_summary("occurrences", stats)
        render(stats, "z", destination("occurrences"), "Ocurrencias verificadas",
               EXPERIMENT_TITLES["occurrences"] + "\nTiempo de consulta por patrón — texto en inglés",
               points=True)
    unary = queries[queries["dataset"] == "unary"]
    for pattern_id, part in unary.groupby("pattern_id"):
        stats = summarize(part, ["n", "method"])
        condition = "presente (z=n)" if pattern_id == "A" else "ausente (z=0)"
        render(stats, "n", destination(f"unary_{pattern_id}"), "Largo del texto [MB]",
               EXPERIMENT_TITLES["occurrences"] + "\nTiempo de consulta en texto unario Aⁿ\n"
               f"Patrón {pattern_id} {condition}")
    for (experiment, dataset), part in samples[samples["phase"] == "construction"].groupby(["experiment", "dataset"]):
        stats = summarize(part, ["n", "method"])
        render(stats, "n", destination(f"construction_{experiment}_{dataset}"), "Largo del texto [MB]",
               EXPERIMENT_TITLES[experiment] + "\nTiempo de construcción del índice Suffix Array",
               ylabel="Tiempo [ns/construcción]")
    for (experiment, dataset), part in memory[memory["phase"] == "construction"].groupby(["experiment", "dataset"]):
        render_index_memory(part, destination(f"index_memory_{experiment}_{dataset}"), dataset,
                            title=EXPERIMENT_TITLES[experiment] +
                            "\nMemoria del índice y construcción — Suffix Array")
    return output


def main():
    parser = argparse.ArgumentParser(description="Grafica CSV experimentales con media y desviacion muestral")
    parser.add_argument("--session", "--data-dir", dest="session", type=Path, default=BASE / "data")
    parser.add_argument("--output-dir", type=Path, default=BASE / "plots")
    args = parser.parse_args()
    if not args.session.is_dir():
        parser.error("No hay resultados en la ruta seleccionada; ejecute make all o indique SESSION")
    try:
        print(generate(args.session, args.output_dir))
    except (OSError, ValueError, KeyError, pd.errors.ParserError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
