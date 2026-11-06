# Boletín 03: String Matching

**Reporte Completo:** [report/Boletin03.pdf](report/Boletin03.pdf)

---

## Objetivos del Boletín

Este boletín aborda la implementación y evaluación empírica de cuatro métodos de búsqueda de patrones en texto, comparando sus tiempos de consulta y su consumo de memoria:

1. **Fuerza Bruta:** Compara el patrón en cada posición posible del texto, como referencia sin preprocesamiento.
2. **Knuth–Morris–Pratt (KMP):** Preprocesa el patrón para evitar comparaciones redundantes durante la búsqueda.
3. **Rabin–Karp:** Utiliza un hash rodante con módulo $q = 101$ y verifica las coincidencias candidatas.
4. **Suffix Array:** Construye un índice reutilizable del texto; su construcción se evalúa por separado de las consultas.

---

## Estructura del Directorio

```text
Boletin_03/
├── Makefile                        # Compilación, ejecución, gráficos y reporte
├── README.md
├── benchmarks/                     # Scripts de ejecución automatizada
│   ├── common.sh                   # Preparación, bloqueo y configuración común
│   ├── run_all.sh                  # Ejecutor de los tres experimentos y sus gráficos
│   ├── run_text_size.sh            # Experimento 1: largo del texto
│   ├── run_pattern_size.sh         # Experimento 2: largo del patrón
│   └── run_occurrences.sh          # Experimento 3: número de ocurrencias
├── data/                           # CSV y controles de cada experimento
│   ├── experimento_1_largo_texto/
│   ├── experimento_2_largo_patron/
│   └── experimento_3_numero_ocurrencias/
├── include/                        # Algoritmos y funciones auxiliares en C++
│   ├── string_matching/
│   │   ├── brute_force.hpp
│   │   ├── kmp.hpp
│   │   ├── rabin_karp.hpp
│   │   └── suffix_array.hpp
│   └── utils/                      # Lectura, patrones, medición, memoria y progreso
├── plots/                          # Graficadores e imágenes generadas en PNG
│   ├── plot_experiments.py         # Gráficos oficiales y validación de resultados
│   ├── plot_boletin03.py           # Graficador genérico
│   ├── experimento_1_largo_texto/
│   ├── experimento_2_largo_patron/
│   └── experimento_3_numero_ocurrencias/
│       └── casos_unarios/
├── report/                         # Informe en LaTeX y PDF
│   ├── Boletin03.tex
│   └── Boletin03.pdf
├── scripts/
│   ├── build_config.py             # Registro de la configuración de compilación
│   └── prepare_text.py             # Preparación y verificación del corpus
├── src/
│   ├── bench_text_size.cpp         # Conductor del Experimento 1
│   ├── bench_pattern_size.cpp      # Conductor del Experimento 2
│   ├── bench_occurrences.cpp       # Conductor del Experimento 3
│   ├── memory_tracking.cpp         # Seguimiento de reservas dinámicas
│   └── prepare_patterns.cpp        # Selección y validación de patrones
└── texts/                          # Corpus, manifiestos y patrones preparados
    ├── README.md
    └── english.50MB.gz             # Original de Pizza & Chili
```

---

## Resumen de los Experimentos

La batería utiliza los primeros 10.000.000 bytes del corpus inglés de Pizza & Chili, sin decodificarlo ni normalizarlo; 1 MB equivale a 1.000.000 bytes. La preparación verifica el gzip y el SHA-256 del prefijo, y registra su procedencia en `texts/manifest.json`. Los cuatro métodos reciben las mismas entradas y sus resultados se contrastan con un conteo independiente que incluye coincidencias solapadas.

Cada configuración se mide con 32 repeticiones como mínimo, calentamiento fuera de muestra y lotes calibrados hasta al menos 5 ms o un millón de consultas. Se utiliza `steady_clock` mediante una adaptación de `uhr`, con tiempo por consulta en nanosegundos, media, desviación estándar muestral y cuartiles. La lectura, selección y validación quedan fuera del reloj; el preprocesamiento de KMP y Rabin–Karp forma parte de cada consulta.

### 1. Experimento 1: Búsqueda variando el largo del texto ($n$)

- **Dominio:** Textos de 1, 2, 4, 8 y 10 MB.
- **Condición:** Cinco patrones fijos de 32 bytes, extraídos del primer MB y únicos en el corpus completo, manteniendo $z = 1$.
- **Medición:** Tiempo y memoria auxiliar de consulta de los cuatro métodos; construcción del Suffix Array medida por separado con 32 repeticiones por tamaño.

### 2. Experimento 2: Búsqueda variando el largo del patrón ($m$)

- **Dominio:** Texto fijo de 10 MB y longitudes de patrón de 8, 16, 32, 64, 128 y 256 bytes.
- **Condición:** Cinco patrones únicos por longitud, con $z = 1$.
- **Medición:** Tiempo y memoria auxiliar de consulta; el Suffix Array se construye una sola vez y se reutiliza, sin repetir la evaluación temporal de su construcción.

### 3. Experimento 3: Búsqueda variando el número de ocurrencias ($z$)

- **Dominio:** Texto fijo de 10 MB y patrones de 4 bytes.
- **Condición:** Hasta cinco patrones por cada tercio logarítmico de las frecuencias observadas y cinco patrones ausentes comprobados, formados con el alfabeto del corpus.
- **Casos unarios:** Textos $A^n$ de los cinco tamaños del Experimento 1, buscando `A` (presente) y `B` (ausente), registrados dentro del mismo experimento.
- **Interpretación:** La frecuencia natural no aísla causalmente el efecto de $z$, porque también cambia el contenido de los patrones. El índice se prepara sin repetir la medición temporal de construcción.

Cada experimento guarda `samples.csv`, `summary.csv`, `memory.csv`, `patterns.csv`, `config.txt`, `session.json` y el marcador `COMPLETE` en su carpeta de `data/`. El graficador genera además `plot_summary.csv` junto a los datos. Una ejecución sustituye solo sus propios resultados; un fallo conserva las salidas parciales, pero estas no se grafican sin los controles de completitud.

La memoria se mide en una pasada separada del cronometraje, distinguiendo reservas dinámicas adicionales por consulta, índice persistente y picos total y temporal de construcción. Se excluyen entradas compartidas, pila, RSS y metadatos del asignador. **Cero bytes adicionales por consulta no significa cero memoria total**, y los picos de construcción no deben sumarse entre sí.

Los gráficos se generan únicamente en PNG a 300 dpi y muestran media ± desviación estándar, no intervalos de confianza. Para $n$ y $m$ se promedian los patrones con igual peso dentro de cada repetición; para $z$ se conserva el resumen por patrón. El graficador valida controles, hashes, repeticiones y memoria antes de reemplazar las figuras de los experimentos disponibles.

---

## Comandos de Compilación y Ejecución

Desde el directorio `Boletin_03/`:

| Comando | Acción |
| :--- | :--- |
| `make all` | Compila con C++20 y `-O3 -march=native`, prepara el corpus, ejecuta los tres experimentos, genera gráficos y limpia los temporales, incluso ante fallos. Es también la acción de `make`. |
| `make build` | Compila el preparador y los tres benchmarks en `bin/`, sin ejecutar experimentos ni limpiar los binarios. |
| `make prepare` | Compila, descarga o verifica el corpus y prepara los patrones. |
| `make run_all` | Ejecuta la misma batería completa y limpieza que `make all`. |
| `make run_text_size` | Ejecuta únicamente el Experimento 1 (Largo del texto). |
| `make run_pattern_size` | Ejecuta únicamente el Experimento 2 (Largo del patrón). |
| `make run_occurrences` | Ejecuta únicamente el Experimento 3 (Ocurrencias y casos unarios). |
| `make plot` | Valida los resultados disponibles y regenera los gráficos oficiales, sin repetir las mediciones. |
| `make plot_generic` | Ejecuta el graficador genérico y genera imágenes PNG. |
| `make check` | Verifica que cada encabezado de algoritmos compile de forma independiente. |
| `make report` | Compila `Boletin03.tex` con Tectonic y produce `report/Boletin03.pdf`. |
| `make clean` | Elimina binarios, objetos, dependencias de compilación y cachés de Python; conserva corpus, patrones, CSV y gráficos. |

`bin/` es una carpeta temporal: contiene los ejecutables, objetos (`.o`), dependencias (`.d`) y la configuración de compilación. Se crea al compilar y se retira al finalizar `make` o ejecutar `make clean`, si queda vacía; no contiene código fuente ni resultados experimentales.

Los scripts fijan afinidad al núcleo lógico 2, usan `flock` para serializar ejecuciones y pausan dos segundos entre series. Los valores pueden cambiarse mediante `RUNS` (32, mínimo 32), `CPU_CORE` (2), `PAUSE_SECONDS` (2), `TARGET_NS` (5000000), `MAX_BATCH` (1000000), `DATA_ROOT`, `TEXT_FILE`, `PATTERN_DIR` y `CORPUS_ARCHIVE` (gzip local para evitar la descarga).

Los gráficos requieren Python con NumPy, pandas y Matplotlib. Make utiliza `../.venv` si existe y permite elegir otro intérprete con `PYTHON`. `make plot` respeta `DATA_ROOT`; se puede seleccionar otra raíz o una sesión histórica con `SESSION` y cambiar la salida con `PLOT_ARGS='--output-dir plots/otra-salida'`. Para verificar la compilación de los encabezados sin repetir los experimentos, se puede ejecutar `make check`.

---

## Documentación de Módulos

- [Fuerza Bruta](include/string_matching/brute_force.hpp)
- [Knuth–Morris–Pratt (KMP)](include/string_matching/kmp.hpp)
- [Rabin–Karp](include/string_matching/rabin_karp.hpp)
- [Suffix Array](include/string_matching/suffix_array.hpp)
- [Lectura del corpus](include/utils/text_data.hpp) y [selección de patrones](include/utils/pattern_selection.hpp)
- [Medición de consultas](include/utils/string_benchmark.hpp), [cronómetro uhr](include/utils/uhr.hpp) y [seguimiento de memoria](include/utils/memory_tracking.hpp)
- [Corpus y procedencia](texts/README.md)

Los algoritmos reciben primero el patrón y después el texto. Se conserva el centinela ETX=3: el cargador experimental rechaza bytes de 0 a 3 y patrones vacíos, sin prometer soporte binario general. El Suffix Array mantiene una vista interna y no debe copiarse ni moverse; el arnés lo construye y destruye en una ubicación fija.

Fuerza bruta se agregó en este boletín; KMP, Rabin–Karp y Suffix Array proceden del [repositorio del curso](https://github.com/jfuentess/edaa/tree/main/strings), con sus atribuciones en los encabezados y correcciones para interpretar los bytes como `unsigned char`. La adaptación de `uhr` conserva la atribución a [Leonardo Lovera (LELE)](https://github.com/leonardlover).
