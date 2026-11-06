# Boletín 03: String Matching

Implementaciones de fuerza bruta, KMP, Rabin–Karp y Suffix Array, con tres
benchmarks separados que reproducen el diseño del informe. `make` y `make all`
compilan, preparan los datos, ejecutan los tres experimentos y generan sus
gráficos PNG/PDF. Para compilar sin medir, usar `make build`.

## Inicio rápido

```bash
make all                     # Compila prepara ejecuta y genera los graficos
make build                   # Compila sin ejecutar experimentos
make prepare                 # Descarga o verifica el corpus y prepara patrones
make run_all                 # Equivale a make all
make report
```

La ejecución imprime la ruta exacta de su sesión y guarda sus gráficos en
`<sesion>/plots/`. No se mezclan ejecuciones diferentes. El objetivo manual
`make plot` necesita la ruta de una sesión, no el directorio genérico `data/`.
Cada sesión usa un directorio nuevo; un experimento solo se considera completo
si existe su archivo `COMPLETE`. Un fallo conserva las salidas parciales para
diagnóstico, pero el graficador las rechaza.

También se pueden ejecutar por separado:

```bash
make run_text_size
make run_pattern_size
make run_occurrences
```

Los scripts de `benchmarks/` fijan afinidad al núcleo lógico 2 y serializan las
ejecuciones con `flock`, incluso entre procesos de Make. Las pausas entre series
son de dos segundos. Para seleccionar otro núcleo explícitamente:

```bash
CPU_CORE=3 make run_all
```

Parámetros: `RUNS` (64, mínimo 4), `CPU_CORE` (2), `PAUSE_SECONDS` (2),
`TARGET_NS` (5000000), `MAX_BATCH` (1000000), `DATA_ROOT`, `TEXT_FILE`,
`PATTERN_DIR` y `CORPUS_ARCHIVE` (gzip local para evitar descarga).

## Corpus y auxiliares

`texts/english.50MB.gz` conserva el original de Pizza & Chili. La preparación
valida el gzip completo y su CRC, y comprueba contra un SHA-256 conocido los
primeros 50.000.000 bytes. `texts/manifest.json` registra procedencia, tamaño y
hashes. `english.bin` se genera localmente y no se duplica en Git.

Las funciones de `include/utils/text_data.hpp` cargan prefijos mediante lectura
binaria exacta y devuelven `std::string`; no decodifican ni normalizan el corpus.
El preparador guarda patrones en hexadecimal para conservar espacios, saltos
de línea y bytes altos. Los manifiestos preparados se verifican al reutilizarlos;
cada sesión conserva además una copia exacta de los patrones utilizados.

El prefijo real contiene, por ejemplo, `his etext was produced by Normal`
(offset 6, 32 bytes) y `Normal W` (offset 32, 8 bytes), ambos únicos en los
50.000.000 bytes. Son subcadenas, no necesariamente palabras completas.

## Experimentos oficiales

1. **Largo del texto:** 1, 5, 10, 20 y 50 MB; diez patrones fijos de 32 bytes
   extraídos del primer MB y únicos en el corpus completo, manteniendo z=1.
2. **Largo del patrón:** texto de 50 MB; diez patrones únicos por longitud
   8, 16, 32, 64, 128 y 256 bytes. Se reutiliza el mismo índice para consultar.
3. **Ocurrencias:** texto de 50 MB, patrones de 4 bytes: hasta diez por cada
   tercio logarítmico de frecuencias observadas y diez ausentes comprobados.
   Incluye por separado los casos obligatorios A^n buscando A/B para los cinco
   tamaños. Los patrones ausentes usan el alfabeto observado, pero no se
   presentan como fragmentos extraídos del corpus.

1 MB = 1.000.000 bytes. Todas las frecuencias incluyen solapamientos. Los cuatro
métodos reciben exactamente las mismas entradas y se contrastan con un conteo
independiente. La variación de frecuencia natural no aísla causalmente z porque
también cambia el contenido de los patrones.

## Medición y CSV

El arnés adapta `../utils/uhr.cpp`, de Leonardo Lovera (LELE), siguiendo el estilo
del Boletín 02: `steady_clock`, nanosegundos, calentamiento fuera de muestra,
media, desviación estándar **muestral** y cuartiles con la convención uhr.

Cada consulta se mide en un lote calibrado hasta al menos 5 ms o hasta un millón
de llamadas; el lote queda fijo entre repeticiones. Hay barreras por invocación
para evitar que GCC elimine consultas repetidas. Se divide por consultas, no
por caracteres ni coincidencias. Lectura, selección, validación y escritura de
resultados quedan fuera del reloj. La preparación interna de KMP y Rabin–Karp
sí está incluida; Rabin–Karp usa q=101. La construcción del Suffix Array se mide
aparte, una por repetición y por texto; su destrucción queda fuera del reloj.

Cada experimento genera:

- `samples.csv`: una fila por patrón/método/repetición, con n, m, z, fase,
  `batch_size`, `time_ns`, hashes e identificadores de sesión/configuración.
- `summary.csv`: estadísticas por patrón o construcción con columnas
  `t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4` y cantidad de repeticiones.
- `memory.csv`: pico dinámico auxiliar de consultas, índice persistente, pico
  de construcción y pico temporal, medidos en una pasada separada.
- `patterns.csv`, `config.txt` y `COMPLETE`: entradas exactas y controles.

El seguimiento de memoria está desactivado durante el cronometraje. Cuenta
bytes solicitados al asignador por cadenas y vectores, no RSS ni metadatos del
asignador o del propio seguimiento. Las entradas compartidas y los archivos
del medidor se excluyen. Cero asignaciones en fuerza bruta/Rabin–Karp no significa
cero variables locales: su memoria escalar es constante.

## Gráficos

Requieren Python con numpy, pandas y matplotlib. Make utiliza `../.venv` si
existe; se puede seleccionar otro intérprete con `PYTHON`.

```bash
# Para una sesion sin graficos como las de experimentos individuales
make plot SESSION=data/una-sesion-sin-graficos
# Para regenerar sin sustituir los graficos creados por make all
make plot SESSION=data/una-sesion-completa PLOT_ARGS='--output-dir plots/otra-salida'
```

Produce PNG y PDF para consultas, construcción, memoria y casos unarios.
Las figuras muestran media ± desviación estándar, **no intervalos de confianza**.
Para n/m primero se promedian con igual peso los patrones dentro de cada
repetición. Para z se mantiene el resumen por patrón y los puntos no se unen
como si fueran una trayectoria. El generador valida controles, hashes,
completitud, repeticiones y memoria antes de crear figuras.


## Organización y procedencia

- `include/string_matching/`: algoritmos; patrón primero, texto después.
- `include/utils/`: lectura, patrones, SHA-256, uhr, memoria y medición común.
- `src/`: tres benchmarks separados y preparador de patrones.
- `scripts/`, `benchmarks/`: preparación del corpus y ejecución automatizada.
- `report/`: LaTeX y PDF del informe.

Fuerza bruta se agregó aquí. Suffix Array, KMP y Rabin–Karp proceden del
[repositorio del curso](https://github.com/jfuentess/edaa/tree/main/strings),
con los enlaces originales conservados en sus encabezados. Se corrigieron los
índices de caracteres del Suffix Array y los operandos del hash de Rabin–Karp
para interpretar bytes como `unsigned char`; no se cambian los algoritmos.
Los encabezados incluyen por sí mismos sus dependencias.

Se conserva ETX=3: el cargador experimental rechaza bytes 0 a 3 y patrones vacíos.
No se promete soporte binario general ni se altera el corpus para adaptarlo.
El Suffix Array no debe copiarse/moverse porque conserva una vista interna;
el arnés lo construye y destruye en una ubicación fija.

Se conservan las atribuciones a LELE y la referencia de `uhr` utilizada en informes anteriores:
https://github.com/leonardlover
