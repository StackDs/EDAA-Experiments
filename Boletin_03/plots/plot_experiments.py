#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

METHODS = ("brute_force", "kmp", "rabin_karp", "suffix_array")
LABELS = {"brute_force": "Fuerza bruta", "kmp": "KMP",
          "rabin_karp": "Rabin–Karp", "suffix_array": "Suffix Array"}
CONTROLS = ["experiment", "dataset", "n", "m", "z", "pattern_id", "method", "phase",
            "corpus_sha256", "session_id", "config_id"]


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


def load_session(session):
    session = Path(session)
    metadata = json.loads((session / "session.json").read_text())
    if metadata["profile"] != "official":
        raise ValueError("Solo se admiten sesiones del protocolo oficial")
    frames, memories = [], []
    for experiment in ("text_size", "pattern_size", "occurrences"):
        directory = session / experiment
        if not directory.exists():
            continue
        if not (directory / "COMPLETE").is_file():
            raise ValueError(f"Experimento incompleto {experiment}")
        config = read_config(directory / "config.txt")
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
        if set(frame["session_id"]) != {session.name}:
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
        if builds.empty or set(builds["method"]) != {"suffix_array"} or (builds["batch_size"] != 1).any():
            raise ValueError("Construccion incompatible")
        if set(builds["m"]) != {0} or set(builds["z"]) != {0} or set(builds["pattern_id"]) != {""}:
            raise ValueError("La construccion no es una consulta")
        query_texts = set(map(tuple, queries[["dataset", "n", "corpus_sha256"]].drop_duplicates().to_numpy()))
        build_texts = set(map(tuple, builds[["dataset", "n", "corpus_sha256"]].drop_duplicates().to_numpy()))
        if query_texts != build_texts:
            raise ValueError("Falta la construccion de algun texto")
        maximum = english[english["n"] == int(config["size"])]
        if set(maximum["corpus_sha256"]) != {config["corpus_sha256"]}:
            raise ValueError("Hash del corpus incompatible")
        memory = pd.read_csv(directory / "memory.csv", keep_default_na=False)
        columns = ["auxiliary_peak_bytes", "index_bytes", "construction_peak_bytes", "construction_temporary_bytes"]
        if not set(CONTROLS + columns).issubset(memory.columns):
            raise ValueError("Memoria incompleta")
        if memory.duplicated(CONTROLS).any():
            raise ValueError("Memoria duplicada")
        joined = frame[CONTROLS].drop_duplicates().merge(memory[CONTROLS], on=CONTROLS, how="outer", indicator=True)
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
    return all_samples, pd.concat(memories, ignore_index=True), metadata


def render(frame, x, destination, xlabel, title, ylabel="Tiempo [ns/consulta]", points=False):
    figure, axis = plt.subplots(figsize=(8, 5))
    try:
        for method, series in frame.groupby("method", sort=False):
            series = series.sort_values(x)
            mean = series["t_mean"].to_numpy(dtype=float)
            sd = series["t_stdev"].to_numpy(dtype=float)
            axis.errorbar(series[x], mean, yerr=[np.minimum(sd, mean), sd],
                          label=LABELS[method], color=f"C{METHODS.index(method)}",
                          marker="o", linestyle="none" if points else "-", capsize=3)
        axis.set(xlabel=xlabel, ylabel=ylabel, title=title)
        axis.set_ylim(bottom=0)
        axis.grid(alpha=.3, linestyle="--")
        axis.legend()
        figure.tight_layout()
        figure.savefig(destination.with_suffix(".png"), dpi=200)
        figure.savefig(destination.with_suffix(".pdf"))
    finally:
        plt.close(figure)


def generate(session, output=None):
    samples, memory, _metadata = load_session(session)
    output = Path(output) if output else Path(session) / "plots"
    output.mkdir(parents=True, exist_ok=False)
    queries = samples[samples["phase"] == "query"]
    for experiment, x, label in (("text_size", "n", "Largo del texto [bytes]"),
                                  ("pattern_size", "m", "Largo del patrón [bytes]")):
        part = queries[(queries["experiment"] == experiment) & (queries["dataset"] == "english")]
        if part.empty:
            continue
        # Promediamos patrones con igual peso dentro de cada repeticion
        means = part.groupby([x, "method", "run"], as_index=False)["time_ns"].mean()
        stats = summarize(means, [x, "method"])
        stats.to_csv(output / f"{experiment}_summary.csv", index=False)
        render(stats, x, output / experiment, label, "Búsqueda con conteo")
        mem = memory[(memory["experiment"] == experiment) & (memory["phase"] == "query")]
        mem = mem.groupby([x, "method"], as_index=False)["auxiliary_peak_bytes"].max()
        mem = mem.rename(columns={"auxiliary_peak_bytes": "t_mean"}).assign(t_stdev=0)
        render(mem, x, output / f"{experiment}_memory", label, "Memoria dinámica auxiliar",
               ylabel="Bytes solicitados al asignador")
    occurrences = queries[(queries["experiment"] == "occurrences") & (queries["dataset"] == "english")]
    if not occurrences.empty:
        stats = summarize(occurrences, ["z", "pattern_id", "method"])
        stats.to_csv(output / "occurrences_summary.csv", index=False)
        render(stats, "z", output / "occurrences", "Ocurrencias verificadas", "Búsqueda por patrón", points=True)
    unary = queries[queries["dataset"] == "unary"]
    for pattern_id, part in unary.groupby("pattern_id"):
        stats = summarize(part, ["n", "method"])
        render(stats, "n", output / f"unary_{pattern_id}", "Largo del texto [bytes]",
               f"Texto unario — patrón {pattern_id}")
    for (experiment, dataset), part in samples[samples["phase"] == "construction"].groupby(["experiment", "dataset"]):
        stats = summarize(part, ["n", "method"])
        render(stats, "n", output / f"construction_{experiment}_{dataset}", "Largo del texto [bytes]",
               "Construcción del índice", ylabel="Tiempo [ns/construcción]")
    for (experiment, dataset), part in memory[memory["phase"] == "construction"].groupby(["experiment", "dataset"]):
        figure, axis = plt.subplots(figsize=(8, 5))
        try:
            for column, label in (("index_bytes", "Índice persistente"),
                                  ("construction_peak_bytes", "Pico total de construcción"),
                                  ("construction_temporary_bytes", "Temporales de construcción")):
                ordered = part.sort_values("n")
                axis.plot(ordered["n"], ordered[column], marker="o", label=label)
            axis.set(xlabel="Largo del texto [bytes]", ylabel="Bytes solicitados al asignador",
                     title="Memoria dinámica del Suffix Array")
            axis.set_ylim(bottom=0)
            axis.legend()
            axis.grid(alpha=.3)
            figure.tight_layout()
            path = output / f"index_memory_{experiment}_{dataset}"
            figure.savefig(path.with_suffix(".png"), dpi=200)
            figure.savefig(path.with_suffix(".pdf"))
        finally:
            plt.close(figure)
    return output


def main():
    parser = argparse.ArgumentParser(description="Grafica CSV experimentales con media y desviacion muestral")
    parser.add_argument("--session", required=True, type=Path)
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args()
    if not args.session.is_dir():
        parser.error("Seleccione una sesion existente con SESSION")
    try:
        print(generate(args.session, args.output_dir))
    except (OSError, ValueError, KeyError, pd.errors.ParserError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
