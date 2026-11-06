# Boletín 04: Range Queries

Base de trabajo para comparar Sparse Table y Segment Tree en consultas de
mínimo en un rango (RMQ). El enunciado se conserva en `Boletin4.md`.

Se reutilizan la organización, el formato del informe y el generador genérico
de gráficos del Boletín 03. Se incorporaron las implementaciones del curso de
Sparse Table y Segment Tree; todavía no hay benchmarks ni resultados
experimentales y falta integrarlas con el diseño del boletín.

El [plan de implementación por fases](docs/superpowers/plans/2026-11-06-boletin04.md)
detalla requisitos, pruebas, experimentos y criterios de validación. Sus
parámetros son propuestas pendientes de aprobación.

## Organización

```text
benchmarks/          Ejecución de experimentos (pendiente)
data/                Entradas y resultados experimentales (pendiente)
include/
  range_queries/     Sparse Table y Segment Tree del repositorio del curso
  utils/             Auxiliares de medición y validación (pendiente)
src/                 Programas y benchmarks en C++ (pendiente)
scripts/             Preparación y automatización (pendiente)
plots/
  plot_boletin04.py   Generador genérico de gráficos desde CSV
  plot_utils.py      Lectura, validación y resumen de muestras
report/
  Boletin04.tex       Documento principal y portada
  sections/          Secciones del informe
  referencias.tex    Referencias
  logo_udec.png      Logo reutilizado del Boletín 03
obsidian/            Notas de trabajo (pendiente)
```

Las carpetas todavía vacías contienen `.gitkeep` para conservarlas en Git,
salvo `obsidian/`, que es local y está excluida mediante `.gitignore`.
No se copian el corpus, los algoritmos ni los experimentos de string matching.

## Implementaciones incorporadas

- `include/range_queries/sparse_table.hpp`: clase genérica `sparse_table<T>`.
- `include/range_queries/segment_tree.hpp`: clase genérica `segment_tree<T>`.

Proceden de [rmq del repositorio del curso](https://github.com/jfuentess/edaa/tree/550e2199f82e74e38d253eba2e64f8c72af38296/rmq),
con autoría original de LELE conservada en los encabezados. Se sustituyeron
los comentarios por explicaciones breves y se añadió `<bit>` a Sparse Table
para que compile por sí sola con C++20; no se cambió la lógica.

Ambas clases consultan intervalos **semiabiertos `[i,j)`** y devuelven el
elemento agregado. Sparse Table ofrece `query` para combinar bloques
disjuntos y `query_idempotent` para la consulta con bloques solapados.
Segment Tree ofrece `query` y `update`.

Para obtener el índice del mínimo se pueden almacenar pares `(valor, índice)`.
Todavía no se adaptaron los intervalos a la interfaz inclusiva propuesta en
el plan ni los rechazos mediante `exit` a excepciones. Las representaciones
originales también difieren de las propuestas: Sparse Table reserva filas
completas de `n` elementos y Segment Tree reserva `2n` elementos sin redondear
a una potencia de dos. Estas decisiones deben revisarse antes de medir memoria.

## Informe

```bash
make report
```

Requiere `tectonic` en el `PATH` (configurable con `TECTONIC`). Genera
`report/Boletin04.pdf`. La portada conserva los datos del autor y de la
asignatura del Boletín 03, con el título actualizado.

El informe mantiene las secciones Resumen, Análisis teórico, Implementación,
Metodología experimental y Referencias. Los subtítulos propios de string
matching se adaptan a Sparse Table, Segment Tree, tamaño del arreglo, tamaño
del rango y espacio utilizado. Los textos pendientes están marcados
explícitamente: la plantilla no constituye todavía el informe final de
5 a 15 páginas solicitado por el enunciado.

## Gráficos

Requieren Python con `numpy`, `pandas` y `matplotlib`. Make selecciona
`../.venv/bin/python3` si existe; se puede usar otro intérprete mediante
`PYTHON=/ruta/a/python3`.

```bash
make plot PLOT_ARGS='--help'
# Cuando existan muestras reales con las columnas n, method y time_ns:
make plot PLOT_ARGS='data/samples.csv --x n --y time_ns --group-by method --pdf'
```

También se puede invocar directamente `plots/plot_boletin04.py` con Python.
`make plot_generic` es un alias de `make plot`.

El generador permite elegir ejes, series (`--group-by`), figuras separadas
(`--facet-by`) y escalas lineales o logarítmicas. Acepta archivos CSV o
directorios de CSV; estos últimos deben contener únicamente muestras
compatibles, no resúmenes ni otras tablas.

Cada fila debe ser una muestra y contener las columnas seleccionadas. El
script valida que no falten datos, que los ejes sean numéricos y finitos y
que las mediciones no sean negativas. Genera PNG, PDF opcional y un
`summary.csv` en `plots/generated/` (configurable con `--output-dir`).
Resume las muestras con mediana e intervalo intercuartílico [Q1, Q3],
**no intervalos de confianza**. Este comportamiento se conserva del
generador genérico del Boletín 03; el resumen definitivo para los
experimentos del Boletín 04 aún debe acordarse.

## Pendiente de definir

- Integración y validación completa de Sparse Table y Segment Tree.
- Convenciones de índices y desempate de mínimos; alcance de las actualizaciones.
- Tamaños de arreglos y rangos, distribución de valores y semillas aleatorias.
- Protocolo de medición, repeticiones, formato CSV y análisis de memoria.
- Ventajas, limitaciones y generalización a otras operaciones binarias,
  según los ejercicios 3 y 4 del enunciado.
