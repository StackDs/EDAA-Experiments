# Boletín 04: Range Queries

**Informe:** [report/Boletin04.pdf](report/Boletin04.pdf)

**Estado:** sesiones, CSV, gráficos y binarios anteriores eliminados para iniciar
una ejecución limpia con `make all`. El análisis dinámico incluye el mantenimiento
y las consultas del mismo ciclo.
Los CSV y gráficos se publican por experimento en `data/experimento_*` y
`plots/experimento_*`, como en el boletín 3.
El informe compila, pero aún falta redactar e incorporar la sección de resultados.

---

## Objetivos del Boletín

Este boletín aborda la implementación y evaluación empírica de dos estructuras para Range Minimum Query (RMQ), comparando construcción, consultas, mantenimiento y espacio utilizado:

1. **Sparse Table:** Preprocesa bloques de longitud potencia de dos para responder consultas estáticas de mínimo en tiempo constante. La implementación incorporada se reconstruye tras una modificación.
2. **Segment Tree:** Organiza los mínimos en una jerarquía y permite modificar un elemento mediante `update`, sin reconstruir toda la estructura.

La respuesta es el **índice del mínimo** del rango; ante empates se elige el menor índice. El enunciado está en [Boletin4.md](Boletin4.md).

---

## Estructura del Directorio

```text
Boletin_04/
├── Makefile                        # Compilación, ejecución, gráficos, informe y limpieza
├── README.md
├── benchmarks/                     # Orquestación Bash basada en el boletín 3
│   ├── common.sh                   # Parámetros, afinidad, bloqueo, sesiones y presupuesto
│   ├── progress.sh                 # Barras por etapa y tiempo transcurrido
│   ├── run_all.sh                  # Piloto automático y batería, o reutilización de PILOT
│   ├── run_pilot.sh                # Piloto con cuatro repeticiones
│   └── run_smoke.sh                # Comprobaciones pequeñas, no oficiales
├── data/                           # CSV por experimento y archivo de sesiones
│   ├── experimento_1_construccion/
│   ├── experimento_2_consultas_estaticas/
│   ├── experimento_3_mantenimiento/
│   ├── experimento_4_consultas_dinamicas/
│   └── <perfil-fecha-id>/           # Se crea al ejecutar
│       ├── construction.csv
│       ├── static_queries.csv
│       ├── maintenance.csv
│       ├── dynamic_queries.csv
│       ├── calibration.csv
│       ├── memory.csv
│       ├── session.json
│       ├── execution.log           # Salida y errores en las nuevas sesiones
│       ├── COMPLETE
│       └── plots/                  # PNG y tablas de esa sesión
├── include/                        # Estructuras y funciones auxiliares en C++
│   ├── range_queries/
│   │   ├── sparse_table.hpp
│   │   └── segment_tree.hpp
│   └── utils/                      # Generación, secuencias, medición, memoria y SHA-256
├── plots/                          # Scripts generadores de gráficos
│   ├── experimento_1_construccion/
│   ├── experimento_2_consultas_estaticas/
│   ├── experimento_3_mantenimiento/
│   ├── experimento_4_consultas_dinamicas/
│   ├── plot_experiments.py         # Validación y gráficos oficiales de una sesión
│   ├── plot_boletin04.py           # Graficador genérico
│   ├── progress_utils.py            # Barra de validación, tablas y gráficos
│   └── plot_utils.py
├── report/                         # Informe en LaTeX y PDF
│   ├── Boletin04.tex
│   ├── Boletin04.pdf
│   └── sections/
├── scripts/
│   ├── build_config.py             # Registro de la configuración de compilación
│   ├── session_metadata.py         # Metadatos y validación del piloto, sin ejecutar benchmarks
│   └── check.py                    # Comprobaciones creadas y eliminadas en temporales
├── src/
│   ├── bench_rmq.cpp               # Motor compartido de los cuatro experimentos
│   ├── bench_memory.cpp            # Pasada independiente de asignaciones
│   └── memory_tracking.cpp         # Seguimiento de reservas dinámicas
└── obsidian/                       # Notas locales, excluidas de Git
```

Las sesiones se conservan por separado. El plan inicial de `docs/` es histórico y no dirige la ejecución.

Cada carpeta `data/experimento_*` contiene `samples.csv`, `summary.csv`,
`plot_summary.csv`, `run_means.csv`, `session.json`, `publication.json` y `COMPLETE`. La calibración
acompaña las consultas estáticas; la memoria de construcción y mantenimiento
acompaña los experimentos 1 y 3. Las carpetas correspondientes de `plots/`
contienen las figuras PNG y su procedencia en `publication.json`.
`summary.csv` conserva los estadísticos por semilla; `plot_summary.csv` contiene
los puntos graficados, con igual peso por semilla: `t_mean` es la media de sus
medias, `t_mean_between_seed_sd` es la desviación estándar muestral entre ellas
y `seeds` indica cuántas semillas contribuyen. La memoria tiene un resumen
equivalente en `memory_plot_summary.csv`.
El piloto permanece archivado en su sesión y no reemplaza los resultados oficiales.
En el experimento 4, `samples.csv` contiene los ciclos derivados: `maintenance_ns`
incluye reconstrucción o `update`, `query_ns` suma los lotes de consultas y
`elapsed_ns` es la suma de ambos. `queries` y `updates` registran sus cantidades;
`operations=1` identifica un ciclo y `range=cycle` combina las longitudes.
`query_samples.csv` conserva el detalle original de consultas sin mantenimiento.

---

## Resumen de los Experimentos

Se utilizarán arreglos numéricos de **1.000, 4.000, 16.000, 64.000, 256.000 y 1.000.000** de elementos, con valores uniformes en `[0, 10^9]` y semillas **1729, 2718 y 31415**. Los generadores adaptan las plantillas de los boletines 1 y 2 con `mt19937_64`, muestreo por rechazo y semillas derivadas. Ambas estructuras reciben las mismas entradas, representadas mediante pares `pair<int64_t,uint64_t>` de valor e índice. Las consultas inclusivas `[L,R]` del informe se traducen a `[L,R+1)` para las clases del curso.

Cada configuración y semilla tendrá **32 repeticiones**, con calentamiento fuera de muestra y medición en nanosegundos mediante una adaptación de `uhr` con `steady_clock`. La generación, preparación de pares, validación, escritura, restauración y destrucción quedan fuera del reloj. Cada método completa su recorrido por separado, alternando su orden entre repeticiones y reutilizando la instancia entre construcción, consultas estáticas y recorrido dinámico.

### 1. Experimento 1: Construcción inicial en el escenario estático

- **Dominio:** Los seis tamaños de arreglo y las tres semillas.
- **Condición:** Mismos pares de entrada para ambas estructuras, preparados antes de medir.
- **Medición:** Tiempo por construcción; las instancias se reutilizan en las consultas, sin volver a cargarles el costo inicial.

### 2. Experimento 2: Consultas en el escenario estático

- **Dominio:** Rangos de longitud `1, 8, 63, 511, floor(n/2), n`, con posición inicial uniforme entre `0` y `n-k`.
- **Condición:** Arreglo fijo e instancias ya construidas. Se preparan 10.000 consultas por longitud y se calibran lotes hasta 5 ms o un máximo de 1.000.000 de consultas, conservándolos entre repeticiones. Se registra si no se alcanza el objetivo. Los rangos medio y completo se identifican por separado, porque crecen con `n`.
- **Medición:** Tiempo por consulta de `query_idempotent` de Sparse Table frente a `query` de Segment Tree. El rango completo tiene una sola posición posible.

### 3. Experimento 3: Mantenimiento en el escenario dinámico

- **Dominio:** Diez ciclos por arreglo, semilla y repetición; cada ciclo contiene un cambio y 1.000 consultas totales.
- **Condición:** Mismas posiciones y nuevos valores, distintos del vigente. Las posiciones se eligen uniformemente con reemplazo. No se agrupan cambios antes de reconstruir la tabla.
- **Medición:** Tiempo de `update` en Segment Tree frente a reconstrucción de Sparse Table, sin consultas ni construcción inicial. La tabla anterior se libera fuera del cronómetro.

### 4. Experimento 4: Costo dinámico de mantenimiento y consultas

- **Dominio:** Diez ciclos del Experimento 3: cada uno incluye un cambio y 1.000 consultas, para un total de 10 cambios y 10.000 consultas por estructura y repetición.
- **Condición:** Ambas estructuras representan el mismo estado actualizado. Las 1.000 consultas de cada ciclo se reparten entre longitudes con diferencia máxima de una consulta, sin repetir cambios o reconstrucciones por longitud. No se fuerza a incluir la posición modificada; esta se verifica por separado fuera del reloj.
- **Medición:** Tiempo por ciclo: reconstrucción de Sparse Table más las 1.000 consultas, frente a `update` de Segment Tree más las mismas consultas. Se suman los intervalos ya cronometrados del mismo método, tamaño, semilla, repetición y ciclo; el mantenimiento se cuenta una sola vez, no por longitud. Se excluye la construcción inicial. Es una suma de componentes medidos, no un cronómetro de extremo a extremo: generación, validación, escritura y liberación siguen fuera del reloj.
- **Resumen:** Se promedian los diez ciclos dentro de cada repetición antes de calcular media, desviación estándar y cuartiles entre las 32 repeticiones de cada semilla. Para el gráfico se promedian las tres medias por semilla con igual peso y se muestra su dispersión. Los CSV usan nanosegundos por ciclo y el gráfico milisegundos por ciclo, con el eje X en elementos. Los gráficos de solo consultas no representan el resultado principal del escenario dinámico.

Se conservarán los cuatro CSV crudos, la calibración, la memoria y los metadatos de cada sesión. Las filas identifican estructura, `n`, `k`, semilla, repetición, ciclo, operaciones, tiempo, checksum, huella de entrada y sesión/configuración. Se comprueban hasta ocho consultas por longitud y estado con una referencia lineal independiente, además de contrastar los checksums dinámicos. En las comprobaciones pequeñas se verifican todos los rangos y casos de borde.

La memoria se analiza en una pasada separada del cronometraje, distinguiendo almacenamiento retenido, nuevas asignaciones y picos durante construcción y reconstrucción. Se excluyen la entrada externa, RSS y metadatos del asignador. El ejecutable de tiempos no enlaza el reemplazo de `new/delete`; cero reservas nuevas en `update` no significa cero memoria total del árbol.

Los gráficos oficiales se generan únicamente en **PNG**, con **eje X logarítmico y eje Y lineal** y **dos curvas**, una por estructura. Cada punto es el promedio de las medias de las tres semillas, con igual peso. Las barras muestran la **desviación estándar muestral entre esas tres medias**, no la dispersión de las 96 ejecuciones ni un intervalo de confianza. Con una sola semilla, como en las comprobaciones pequeñas, no se estima dispersión entre semillas. Los CSV por semilla conservan media, desviación, mínimo, Q1, mediana, Q3 y máximo con la convención de `uhr`. Las 32 repeticiones no son arreglos independientes ni garantizan significancia estadística.

Las figuras usan milisegundos para construcción, mantenimiento y ciclos dinámicos;
nanosegundos para consultas; y MiB para memoria. Los CSV conservan nanosegundos
y bytes, respectivamente. La conversión de unidades no cambia las mediciones.

---

## Comandos de Compilación y Ejecución

Desde el directorio `Boletin_04/`:

| Comando | Acción |
| :--- | :--- |
| `make all` | Compila con C++20 y `-O3 -march=native`, ejecuta el piloto, valida el presupuesto, corre la batería oficial y memoria, genera gráficos PNG y limpia la compilación incluso ante fallos. Es también la acción de `make`. No genera PDF; el informe se compila aparte con `make report`. |
| `make build` | Compila ambos ejecutables en `bin/`, sin ejecutar experimentos. |
| `make pilot` | Compila y recorre la misma matriz con cuatro repeticiones para estimar la duración. |
| `make run_all PILOT=ruta` | Compila y ejecuta la batería oficial, memoria y gráficos utilizando un piloto compatible aprobado. |
| `make plot_session SESSION=ruta` | Valida una sesión oficial completa, regenera sus tablas y gráficos y los publica por experimento, sin repetir mediciones. |
| `make plot PLOT_ARGS='--help'` | Muestra las opciones del graficador genérico; `make plot_generic` es su alias. |
| `make check` | Comprueba encabezados, generadores, RMQ, actualizaciones, sanitizadores y pipeline pequeño en temporales que se eliminan al terminar. |
| `make report` | Compila `Boletin04.tex` con Tectonic y produce `report/Boletin04.pdf`. |
| `make clean` | Elimina los ejecutables y archivos de compilación conocidos; conserva fuentes, CSV, gráficos e informe. |
| `make help` | Muestra los comandos principales. |


---

## Documentación de Módulos

- [Sparse Table](include/range_queries/sparse_table.hpp) y [Segment Tree](include/range_queries/segment_tree.hpp).
- [Generación de arreglos, consultas y actualizaciones](include/utils/rmq_inputs.hpp).
- [Preparación de secuencias y referencia independiente](include/utils/rmq_workload.hpp).
- [Medición adaptada de uhr](include/utils/measurement.hpp) y [seguimiento de memoria](include/utils/memory_tracking.hpp).
- [Motor de los cuatro experimentos](src/bench_rmq.cpp) y [pasada de memoria](src/bench_memory.cpp).
- [Orquestación Bash común](benchmarks/common.sh), [piloto](benchmarks/run_pilot.sh) y [ejecución completa](benchmarks/run_all.sh).
- [Metadatos de sesión y validación del piloto](scripts/session_metadata.py), sin ejecutar benchmarks ni gráficos.
- [Validación, tablas y gráficos oficiales](plots/plot_experiments.py).

Las estructuras proceden del [repositorio del curso](https://github.com/jfuentess/edaa/tree/550e2199f82e74e38d253eba2e64f8c72af38296/rmq), con autoría original de LELE y fuente citada en los encabezados. Se conservaron sus algoritmos e interfaces semiabiertas; solo se sustituyeron comentarios y se añadió `<bit>` a Sparse Table. La generación adapta los patrones de `Boletin_01/src/bench_heaps.cpp` y `Boletin_02/include/utils/discrete_queries.h`, no el generador creciente del boletín 1. La medición y el seguimiento de memoria reutilizan las plantillas del boletín 3, conservando la atribución de `uhr` a [Leonardo Lovera (LELE)](https://github.com/leonardlover).
