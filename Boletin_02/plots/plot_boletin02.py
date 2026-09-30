#!/usr/bin/env python3
"""Grafica los resúmenes CSV de uhr para los árboles del Boletín 02."""

import argparse
import sys
import unicodedata
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


BASE = Path(__file__).resolve().parents[1]
ESTILOS = {
    "AVL": {"color": "#1f77b4", "marker": "o"},
    "Rojo-Negro": {"color": "#d95f02", "marker": "s"},
    "Splay": {"color": "#2ca02c", "marker": "^"},
}
ESTILOS_DISTRIBUCION = {
    "uniforme": {"color": "#1f77b4", "marker": "o"},
    "binomial_negativa": {"color": "#d95f02", "marker": "s"},
}
FASES = {"insercion": "Inserción", "busqueda": "Búsqueda", "eliminacion": "Eliminación"}
ESCENARIOS = {"general": "", "d0": "sin consultas previas", "d1": "tras las consultas"}

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


def normalizar(valor):
    if pd.isna(valor):
        return ""
    texto = unicodedata.normalize("NFKD", str(valor).lower())
    return "".join(c for c in texto if not unicodedata.combining(c))


def reconocer_arbol(valor):
    texto = normalizar(valor)
    if "splay" in texto:
        return "Splay"
    if "avl" in texto:
        return "AVL"
    if any(nombre in texto for nombre in ("red_black", "red-black", "redblack", "rojo", "std_set", "std::set", "rbtree")):
        return "Rojo-Negro"
    return None


def reconocer_fase(valor):
    texto = normalizar(valor)
    if "inser" in texto or "insert" in texto:
        return "insercion"
    if "busqu" in texto or "search" in texto or "consulta" in texto:
        return "busqueda"
    if any(nombre in texto for nombre in ("elimin", "erase", "delete", "remove")):
        return "eliminacion"
    return None


def reconocer_distribucion(valor):
    texto = normalizar(valor)
    if "negative_binomial" in texto or "binomial_negativa" in texto:
        return "binomial_negativa"
    if "geometr" in texto or "geometric" in texto:
        return "geometrica"
    if "poisson" in texto:
        return "poisson"
    if any(nombre in texto for nombre in ("sesgad", "biased", "80_5", "80-5")):
        return "sesgada"
    if "uniform" in texto:
        return "uniforme"
    return "sin_especificar"


def reconocer_escenario(valor):
    texto = normalizar(valor)
    if "d0" in texto or "aislad" in texto:
        return "d0"
    if "d1" in texto or "tras_consultas" in texto:
        return "d1"
    return "general"


def buscar_columna(df, opciones):
    return next((col for col in opciones if col in df.columns), None)


def cargar_csv(ruta):
    try:
        datos = pd.read_csv(ruta)
    except (OSError, pd.errors.ParserError, pd.errors.EmptyDataError) as error:
        print(f"[Aviso] No se pudo leer {ruta}: {error}", file=sys.stderr)
        return []

    necesarias = {"n", "t_mean", "t_stdev"}
    if not necesarias.issubset(datos.columns):
        print(f"[Aviso] Se omite {ruta}: faltan columnas de uhr {sorted(necesarias - set(datos.columns))}", file=sys.stderr)
        return []

    columna_arbol = buscar_columna(datos, ("arbol", "tree", "algoritmo"))
    columna_fase = buscar_columna(datos, ("fase", "phase", "operacion"))
    columna_distribucion = buscar_columna(datos, ("distribucion", "distribution"))
    columna_tamano = buscar_columna(datos, ("tamano_inicial", "block_start_size"))
    filas = []
    omitidas = 0

    for _, fila in datos.iterrows():
        arbol = reconocer_arbol(fila[columna_arbol]) if columna_arbol else None
        arbol = arbol or reconocer_arbol(ruta.stem)
        fase_bruta = fila[columna_fase] if columna_fase else ruta.stem
        fase = reconocer_fase(fase_bruta) or reconocer_fase(ruta.stem)
        distribucion = reconocer_distribucion(fila[columna_distribucion]) if columna_distribucion else "sin_especificar"
        if distribucion == "sin_especificar":
            distribucion = reconocer_distribucion(ruta.stem)
        detalle = f"{fase_bruta} {ruta.stem}"
        escenario = reconocer_escenario(detalle)
        es_tramo = "tramo" in normalizar(detalle) or "block" in normalizar(detalle)

        try:
            n = int(fila["n"])
            media = float(fila["t_mean"])
            desviacion = float(fila["t_stdev"])
            tamano_inicial = float(fila[columna_tamano]) if columna_tamano and pd.notna(fila[columna_tamano]) else np.nan
        except (TypeError, ValueError):
            omitidas += 1
            continue

        if not arbol or not fase or n <= 0 or media < 0 or desviacion < 0 or not np.isfinite([media, desviacion]).all():
            omitidas += 1
            continue
        if es_tramo and not np.isfinite(tamano_inicial):
            omitidas += 1
            continue

        filas.append({
            "n": n,
            "t_mean": media,
            "t_stdev": desviacion,
            "arbol": arbol,
            "fase": fase,
            "distribucion": distribucion,
            "escenario": escenario,
            "tipo": "tramo" if es_tramo else "total",
            "tamano_inicial": tamano_inicial,
            "archivo": str(ruta),
        })

    if omitidas:
        print(f"[Aviso] {ruta}: {omitidas} filas sin metadatos reconocibles o con tiempos inválidos", file=sys.stderr)
    return filas


def cargar_datos(rutas):
    filas = [fila for ruta in rutas for fila in cargar_csv(ruta)]
    if not filas:
        return pd.DataFrame()
    datos = pd.DataFrame(filas)
    datos = datos[
        datos["distribucion"].isin(("uniforme", "binomial_negativa", "sin_especificar"))
        & ~((datos["escenario"] == "d1") & (datos["arbol"] != "Splay"))
    ]
    if datos.empty:
        return datos
    claves = ["n", "arbol", "fase", "distribucion", "escenario", "tipo", "tamano_inicial"]
    duplicadas = datos.duplicated(claves, keep=False)
    if duplicadas.any():
        ejemplo = datos.loc[duplicadas, claves].iloc[0].to_dict()
        raise ValueError(f"Hay resúmenes duplicados para la misma serie: {ejemplo}")
    return datos


def barras_error(medias, desviaciones):
    medias = np.asarray(medias, dtype=float)
    desviaciones = np.asarray(desviaciones, dtype=float)
    return [np.minimum(medias, desviaciones), desviaciones]


def guardar(figura, ruta, pdf):
    ruta.parent.mkdir(parents=True, exist_ok=True)
    figura.savefig(ruta)
    if pdf:
        figura.savefig(ruta.with_suffix(".pdf"))
    plt.close(figura)
    print(f"  -> {ruta}")


def dibujar_serie(eje, datos, columna_x, etiqueta, estilo):
    datos = datos.sort_values(columna_x)
    eje.errorbar(
        datos[columna_x].to_numpy(dtype=float),
        datos["t_mean"].to_numpy(dtype=float),
        yerr=barras_error(datos["t_mean"], datos["t_stdev"]),
        label=etiqueta,
        color=estilo["color"],
        marker=estilo["marker"],
        linewidth=1.7,
        markersize=5,
        capsize=3,
    )


def configurar_ejes(eje, titulo, etiqueta_x, x_log=False):
    eje.set_title(titulo)
    eje.set_xlabel(etiqueta_x)
    eje.set_ylabel("Tiempo medio [ns/operación] ± desviación estándar")
    eje.set_ylim(bottom=0)
    if x_log:
        eje.set_xscale("log")
    eje.legend(loc="best", framealpha=0.9)


def graficar_por_tamano(datos, salida, pdf, fases):
    total = datos[(datos["tipo"] == "total") & datos["fase"].isin(fases)]
    generadas = 0
    for (fase, distribucion, escenario), grupo in total.groupby(["fase", "distribucion", "escenario"]):
        figura, eje = plt.subplots()
        for arbol, serie in grupo.groupby("arbol"):
            dibujar_serie(eje, serie, "n", arbol, ESTILOS[arbol])
        detalle = f" ({ESCENARIOS[escenario]})" if escenario != "general" else ""
        if fase == "insercion":
            titulo = "Inserción de claves barajadas"
        elif fase == "eliminacion" and escenario == "d0":
            titulo = "Vaciado completo del árbol (D0): AVL, Rojo-Negro y Splay"
        else:
            titulo = f"{FASES[fase]}{detalle} — {distribucion.replace('_', ' ')}"
        configurar_ejes(eje, titulo, "Número de claves (n)", x_log=True)
        ruta = salida / "por_tamano" / f"fig_{fase}_{escenario}_{distribucion}.png"
        guardar(figura, ruta, pdf)
        generadas += 1
    return generadas


def graficar_distribuciones(datos, salida, pdf):
    busquedas = datos[(datos["fase"] == "busqueda") & (datos["tipo"] == "total")]
    generadas = 0
    for arbol, grupo in busquedas.groupby("arbol"):
        if grupo["distribucion"].nunique() < 2:
            continue
        figura, eje = plt.subplots()
        for distribucion, serie in grupo.groupby("distribucion"):
            estilo = ESTILOS_DISTRIBUCION.get(distribucion, {"color": "#7f7f7f", "marker": "x"})
            dibujar_serie(eje, serie, "n", distribucion.replace("_", " "), estilo)
        configurar_ejes(eje, f"Búsqueda en {arbol}: efecto de la distribución", "Número de claves (n)", x_log=True)
        ruta = salida / "distribuciones" / f"fig_distribuciones_{arbol.lower().replace('-', '_')}.png"
        guardar(figura, ruta, pdf)
        generadas += 1
    return generadas


def graficar_eliminacion_por_tramos(datos, salida, pdf):
    tramos = datos[(datos["fase"] == "eliminacion") & (datos["tipo"] == "tramo")]
    generadas = 0
    for (n, distribucion, escenario), grupo in tramos.groupby(["n", "distribucion", "escenario"]):
        figura, eje = plt.subplots()
        for arbol, serie in grupo.groupby("arbol"):
            dibujar_serie(eje, serie, "tamano_inicial", arbol, ESTILOS[arbol])
        detalle = f" ({ESCENARIOS[escenario]})" if escenario != "general" else ""
        configurar_ejes(eje, f"Eliminación por tamaño del árbol{detalle}; n={n}", "Claves antes del tramo")
        ruta = salida / "eliminacion_tramos" / f"fig_eliminacion_tramos_n{n}_{escenario}_{distribucion}.png"
        guardar(figura, ruta, pdf)
        generadas += 1
    return generadas


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, default=BASE / "data", help="Carpeta con CSV, examinada recursivamente")
    parser.add_argument("--input", type=Path, action="append", help="CSV concreto; se puede repetir")
    parser.add_argument("--output-dir", type=Path, default=BASE / "plots", help="Carpeta para las figuras")
    parser.add_argument("--pdf", action="store_true", help="Guardar también cada figura como PDF")
    seleccion = parser.add_mutually_exclusive_group()
    seleccion.add_argument("--basic", action="store_true", help="Inserción y búsqueda por tamaño")
    seleccion.add_argument("--queries", action="store_true", help="Búsqueda por tamaño y por distribución")
    seleccion.add_argument("--deletion", action="store_true", help="Eliminación total y por tramos")
    args = parser.parse_args()

    rutas = args.input if args.input else sorted(args.data_dir.rglob("*.csv"))
    if not rutas:
        parser.exit(1, f"No hay archivos CSV en {args.data_dir}.\n")
    try:
        datos = cargar_datos(rutas)
    except ValueError as error:
        parser.exit(2, f"Error: {error}\n")
    if datos.empty:
        parser.exit(1, "No se encontraron filas válidas de resúmenes uhr.\n")

    if args.basic:
        fases = {"insercion", "busqueda"}
    elif args.queries:
        fases = {"busqueda"}
    elif args.deletion:
        fases = {"eliminacion"}
    else:
        fases = set(FASES)

    cantidad = graficar_por_tamano(datos, args.output_dir, args.pdf, fases)
    if not args.basic and not args.deletion:
        cantidad += graficar_distribuciones(datos, args.output_dir, args.pdf)
    if not args.basic and not args.queries:
        cantidad += graficar_eliminacion_por_tramos(datos, args.output_dir, args.pdf)
    print(f"Figuras generadas: {cantidad}")
    if cantidad == 0:
        parser.exit(1, "Los CSV no contienen fases graficables para la selección indicada.\n")


if __name__ == "__main__":
    main()
