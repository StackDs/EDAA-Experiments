# Boletín 03: String Matching

Esta etapa contiene implementaciones de Suffix Array, KMP y Rabin–Karp,
herramientas para graficar futuros CSV y la estructura del informe.
No incluye un diseño experimental, benchmarks nuevos ni resultados medidos.
El informe contiene únicamente títulos de secciones, portada y referencias.

## Organización

```text
include/string_matching/
    suffix_array.hpp
    kmp.hpp
    rabin_karp.hpp
plots/
    plot_boletin03.py
    plot_utils.py
report/
    Boletin03.tex
    referencias.tex
    sections/
Makefile
```

Los archivos conservan el código del repositorio del curso, sin modificaciones
funcionales, eliminando los comentarios y dejando únicamente el enlace de origen.
Se mantienen los nombres locales `.hpp` y se requiere C++20.

## Uso de los módulos

```cpp
#include <cstdint>
#include <string_view>
#include "string_matching/suffix_array.hpp"
#include "string_matching/kmp.hpp"
#include "string_matching/rabin_karp.hpp"

suffix_array index("banana");
auto a = index.count("ana");
auto b = kmp("ana", "banana");
auto c = rk("ana", "banana", 101);
```

- Las funciones de búsqueda reciben **patrón primero, texto después**.
- Suffix Array conserva el centinela ETX (`char(3)`) del original.
- Se conservan las limitaciones originales: no se añaden validaciones para
  patrones vacíos, entradas binarias ni módulos inválidos de Rabin–Karp.

Comprobar la compilación conjunta, sin generar binarios:

```bash
cd Boletin_03
make
```

## Gráficos

Requieren Python con `numpy`, `pandas` y `matplotlib`. El Makefile utiliza el
entorno `../.venv` si está disponible. El script usa un backend no interactivo.

El script recibe **muestras crudas**, una fila por observación. Las columnas de
los ejes se seleccionan con `--x` y `--y`; `--group-by` identifica las series
(por defecto, `method`), y `--facet-by` separa figuras. No se infieren unidades,
métodos, tamaños, fases ni configuraciones a partir del nombre del archivo.

Ejemplo de uso futuro, cuando exista un CSV real con las columnas indicadas:

```bash
../.venv/bin/python3 plots/plot_boletin03.py data/results.csv \
    --x n --y time_per_query_ns --group-by method \
    --facet-by phase m --xlabel 'Largo del texto [bytes]' \
    --ylabel 'Tiempo [ns/consulta], mediana e IQR' --pdf
```

También se puede usar `make plot PLOT_ARGS='...'` con los mismos argumentos.

- Cada punto muestra la mediana y el intervalo entre Q1 y Q3; `summary.csv`
  conserva esos estadísticos y la cantidad de muestras.
- Se generan PNG y, con `--pdf`, PDF en `plots/generated/`, o en `--output-dir`.
- Los CSV deben contener todas las columnas seleccionadas, sin valores ausentes.
  Los ejes deben ser numéricos y finitos; las mediciones no pueden ser negativas.
- `--xscale log` y `--yscale log` requieren valores estrictamente positivos.
  Para un eje de ocurrencias que incluya cero, usar escala lineal.
- Seleccionar o filtrar explícitamente todos los controles relevantes antes de
  graficar: solo se agrupa por las columnas solicitadas, no por metadatos ocultos.
- No se generan datos de ejemplo ni gráficos sin entradas. Estas herramientas
  no definen todavía las baterías experimentales.

## Informe

```bash
make report
```

Compila `report/Boletin03.tex` con Tectonic. Las secciones están separadas en
`report/sections/`; sus contenidos se completarán después. Las referencias ya
incluyen los tres archivos originales.

## Procedencia

Fuente: <https://github.com/jfuentess/edaa/tree/main/strings>.

| Módulo | Enlace original |
|---|---|
| Suffix Array (autor indicado: LELE) | <https://github.com/jfuentess/edaa/blob/main/strings/suffix_array.cpp> |
| KMP | <https://github.com/jfuentess/edaa/blob/main/strings/kmp.cpp> |
| Rabin–Karp | <https://github.com/jfuentess/edaa/blob/main/strings/rk.cpp> |

No se encontró un archivo de licencia en la raíz ni en `strings/` del repositorio
consultado; no se atribuye una licencia que la fuente no declara.

Las pruebas son temporales y no se conservan en el repositorio.
`make check` comprueba compilación, no sustituye las pruebas funcionales.
