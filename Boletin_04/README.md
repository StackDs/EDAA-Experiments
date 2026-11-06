# Boletín 04: Range Queries

**Informe:** [report/Boletin04.pdf](report/Boletin04.pdf)

**Estado:** batería preparada y comprobada con casos pequeños; todavía no se han ejecutado el piloto completo ni las mediciones oficiales.

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
├── data/                           # Sesiones nuevas, sin sobrescribir resultados
│   └── <perfil-fecha-id>/           # Se crea al ejecutar
│       ├── construction.csv
│       ├── static_queries.csv
│       ├── maintenance.csv
│       ├── dynamic_queries.csv
│       ├── calibration.csv
│       ├── memory.csv
│       ├── session.json
│       ├── COMPLETE
│       └── plots/                  # PNG, PDF y tablas de esa sesión
├── include/                        # Estructuras y funciones auxiliares en C++
│   ├── range_queries/
│   │   ├── sparse_table.hpp
│   │   └── segment_tree.hpp
│   └── utils/                      # Generación, secuencias, medición, memoria y SHA-256
├── plots/                          # Scripts generadores de gráficos
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

Las sesiones del árbol son la organización de salida prevista, no resultados ya obtenidos. El plan inicial de `docs/` es histórico y no dirige la ejecución.

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

### 4. Experimento 4: Consultas en el escenario dinámico

- **Dominio:** Los lotes posteriores a cada cambio del Experimento 3: 10.000 consultas medidas por estructura y repetición.
- **Condición:** Ambas estructuras representan el mismo estado actualizado. Las 1.000 consultas de cada ciclo se reparten entre longitudes con diferencia máxima de una consulta, sin repetir cambios o reconstrucciones por longitud. No se fuerza a incluir la posición modificada; esta se verifica por separado fuera del reloj.
- **Medición:** Tiempo por consulta tras las modificaciones, separado del mantenimiento. Los diez ciclos se agregan dentro de cada repetición normalizando por operaciones; no se cuentan como repeticiones independientes.

Se conservarán los cuatro CSV crudos, la calibración, la memoria y los metadatos de cada sesión. Las filas identifican estructura, `n`, `k`, semilla, repetición, ciclo, operaciones, tiempo, checksum, huella de entrada y sesión/configuración. Se comprueban hasta ocho consultas por longitud y estado con una referencia lineal independiente, además de contrastar los checksums dinámicos. En las comprobaciones pequeñas se verifican todos los rangos y casos de borde.

La memoria se analiza en una pasada separada del cronometraje, distinguiendo almacenamiento retenido, nuevas asignaciones y picos durante construcción y reconstrucción. Se excluyen la entrada externa, RSS y metadatos del asignador. El ejecutable de tiempos no enlaza el reemplazo de `new/delete`; cero reservas nuevas en `update` no significa cero memoria total del árbol.

Los gráficos oficiales se generan en **PNG y PDF**, con **eje X logarítmico y eje Y lineal**, y muestran media ± desviación estándar muestral, no intervalos de confianza. Los resúmenes incluyen mínimo, Q1, mediana, Q3 y máximo con la convención de cuartiles de `uhr`. Cada semilla conserva su resumen: las 32 repeticiones no son arreglos independientes ni garantizan significancia estadística.

---

## Comandos de Compilación y Ejecución

Desde el directorio `Boletin_04/`:

| Comando | Acción |
| :--- | :--- |
| `make all` | Compila con C++20 y `-O3 -march=native`, ejecuta el piloto, valida el presupuesto, corre la batería oficial y memoria, genera gráficos, compila el informe y limpia la compilación incluso ante fallos. Es también la acción de `make`. |
| `make build` | Compila ambos ejecutables en `bin/`, sin ejecutar experimentos. |
| `make pilot` | Compila y recorre la misma matriz con cuatro repeticiones para estimar la duración. |
| `make run_all PILOT=ruta` | Compila y ejecuta la batería oficial, memoria y gráficos utilizando un piloto compatible aprobado. |
| `make plot_session SESSION=ruta` | Valida una sesión completa y regenera sus tablas y gráficos oficiales, sin repetir mediciones. |
| `make plot PLOT_ARGS='--help'` | Muestra las opciones del graficador genérico; `make plot_generic` es su alias. |
| `make check` | Comprueba encabezados, generadores, RMQ, actualizaciones, sanitizadores y pipeline pequeño en temporales que se eliminan al terminar. |
| `make report` | Compila `Boletin04.tex` con Tectonic y produce `report/Boletin04.pdf`. |
| `make clean` | Elimina los ejecutables y archivos de compilación conocidos; conserva fuentes, CSV, gráficos e informe. |
| `make help` | Muestra los comandos principales. |

`bin/` es temporal: contiene ejecutables, dependencias y configuración de compilación. Se retira al finalizar `make all` o ejecutar `make clean`, si queda vacía. `make build`, `make pilot`, `make check` y `make run_all` conservan los binarios para poder reutilizarlos. `make -n all` no produce efectos. Las comprobaciones no dejan datos simulados en `data/`; se evita generar cachés Python y la caché de Matplotlib del lanzador es temporal.

La orquestación se realiza en **Bash**, con `common.sh` y scripts de entrada como en el boletín 3. Se usa `flock` desde la compilación hasta la limpieza de `make all`. No iniciar ejecuciones concurrentes. La afinidad predeterminada es `CPU_CORE=2`; el núcleo debe pertenecer a la afinidad permitida. No se cambia silenciosamente: cualquier ajuste debe registrarse también en el informe.

Sin `PILOT`, `make all` genera uno y utiliza automáticamente su ruta; con `PILOT=ruta`, lo reutiliza si es compatible. El piloto debe coincidir en parámetros, fuentes, binarios, compilador, equipo y versiones de Python y bibliotecas.
```bash
make all CPU_CORE=2

# Ejecución por etapas
make pilot CPU_CORE=2
make run_all CPU_CORE=2 PILOT=data/ruta-del-piloto

# Ajustes previos al piloto que deben justificarse en el informe
make all BENCH_ARGS='--sizes 1000,4000,16000 --cycles 5'
```

`BENCH_ARGS` permite configurar `--sizes`, `--seeds`, `--cycles`, `--dynamic-queries`, `--static-queries`, `--target-ns`, `--max-batch`, `--data-root` y `--timeout-seconds`. Los ajustes deben ser idénticos al generar y reutilizar el piloto. No usar el perfil de comprobación como resultado oficial.

Se requiere Linux, Bash, GCC con C++20, Make, `taskset`, `flock`, `timeout` y Python con NumPy, pandas y Matplotlib. Tectonic compila el informe. Make utiliza `../.venv` si existe y permite elegir otro intérprete con `PYTHON`. También se pueden invocar los scripts tras compilar, por ejemplo `PYTHON=../.venv/bin/python3 bash benchmarks/run_pilot.sh --cpu-core 2`. El graficador genérico conserva mediana e intervalo intercuartílico; no sustituye al analizador oficial de sesiones.

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
