"""Lectura y resumen de muestras CSV; no genera datos experimentales."""

from pathlib import Path

import numpy as np
import pandas as pd


def find_csv(paths):
    """Expande directorios y evita leer dos veces un mismo archivo."""
    files = set()
    for path in map(Path, paths):
        if path.is_dir():
            matches = list(path.rglob("*.csv"))
            if not matches:
                raise ValueError(f"No hay archivos CSV en {path}")
            files.update(item.resolve() for item in matches)
        elif path.is_file() and path.suffix.lower() == ".csv":
            files.add(path.resolve())
        else:
            raise ValueError(f"Entrada CSV inexistente o inválida: {path}")
    if not files:
        raise ValueError("No se encontraron muestras CSV")
    return sorted(files)


def load_samples(paths, x, y, group_by, facet_by):
    required = list(dict.fromkeys([x, y, *group_by, *facet_by]))
    frames = []
    for path in find_csv(paths):
        try:
            frame = pd.read_csv(path)
        except (OSError, pd.errors.ParserError, pd.errors.EmptyDataError) as error:
            raise ValueError(f"No se pudo leer {path}: {error}") from error
        missing = set(required) - set(frame.columns)
        if missing:
            raise ValueError(f"{path}: faltan columnas {sorted(missing)}")
        frame = frame[required].copy()
        if frame.empty:
            raise ValueError(f"{path}: no contiene muestras")
        if frame.isna().any().any():
            raise ValueError(f"{path}: hay valores ausentes en las columnas seleccionadas")
        for column in (x, y):
            try:
                frame[column] = pd.to_numeric(frame[column], errors="raise")
            except (TypeError, ValueError) as error:
                raise ValueError(f"{path}: {column} debe ser numérica") from error
            if not np.isfinite(frame[column].to_numpy(dtype=float)).all():
                raise ValueError(f"{path}: {column} contiene valores no finitos")
        if (frame[y] < 0).any():
            raise ValueError(f"{path}: {y} contiene mediciones negativas")
        frames.append(frame)
    return pd.concat(frames, ignore_index=True)


def summarize(samples, x, y, group_by, facet_by):
    keys = [*facet_by, *group_by, x]
    if len(set([*keys, y])) != len(keys) + 1:
        raise ValueError("Ejes, agrupaciones y facetas deben usar columnas distintas")
    if set(keys) & {"median", "q1", "q3", "samples"}:
        raise ValueError("Los nombres median, q1, q3 y samples están reservados para el resumen")
    return (
        samples.groupby(keys, sort=True, observed=True, dropna=False)[y]
        .agg(
            median="median",
            q1=lambda values: values.quantile(0.25),
            q3=lambda values: values.quantile(0.75),
            samples="size",
        )
        .reset_index()
    )


def groups(frame, columns):
    if not columns:
        yield (), frame
        return
    for key, part in frame.groupby(columns, sort=True, observed=True, dropna=False):
        yield key if isinstance(key, tuple) else (key,), part
