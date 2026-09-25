# Módulo de Algoritmos de Búsqueda en Secuencias Monótonas

---

## Descripción General

Este módulo provee implementaciones genéricas de alto rendimiento para búsqueda de claves numéricas sobre secuencias unidimensionales ordenadas monótonamente en memoria contigua (`std::vector<key>`). 

Todas las funciones están parametrizadas mediante el concepto de C++20 `std::integral key = std::int64_t`, restringiendo el tipo a representaciones enteras exactas para evitar pérdida de precisión de mantisa o anomalías de absorción numérica inherentes a punto flotante IEEE 754.

Retornan el **índice de base 0** donde reside el elemento buscado (`std::int64_t`), o `-1` si el elemento no se encuentra en la secuencia.

---

## Archivos del Módulo

| Archivo | Contenido |
| :--- | :--- |
| [search.hpp](search.hpp) | Cabecera agregadora que incluye todas las variantes de búsqueda. |
| [sequential_search.hpp](sequential_search.hpp) | Búsqueda Secuencial (Lineal): variantes propia y basada en STL (`std::find`). |
| [binary_search.hpp](binary_search.hpp) | Búsqueda Binaria (Dicotómica): variante propia con cálculo seguro de punto medio y STL (`std::lower_bound`). |
| [galloping_search.hpp](galloping_search.hpp) | Búsqueda Galopante (Exponencial): expansión de saltos en potencias de 2 + bisección acotada. |
| [../generator.hpp](../generator.hpp) | Generador de secuencias monótonas en $\mathcal{O}(n)$ vía pasos aleatorios acumulativos. |

---

## Análisis Detallado de Implementaciones

### 1. Búsqueda Secuencial (Linear Search)
* **`sequential_search_custom`:** Itera secuencialmente desde el índice $0$ hasta $n-1$.
* **`sequential_search_STL`:** Delega en `std::find(vect.begin(), vect.end(), target)` y traduce el iterador con `std::distance`.
* **Comportamiento en Hardware:**
  - Acceso estrictamente secuencial y contiguo en memoria.
  - El prefetcher de hardware de la CPU anticipa las siguientes líneas de caché (64 bytes) sin fallos (*cache hits* continuos).
  - La versión STL de GCC aprovecha vectorización automática con instrucciones SIMD (AVX2/AVX-512), evaluando múltiples enteros de 64 bits en un solo ciclo de reloj.

### 2. Búsqueda Binaria (Binary Search)
* **`binary_search_custom`:** Bisección iterativa clásica.
  - **Cálculo seguro de punto medio:** Se calcula mediante `mid = left + (right - left) / 2`. Esto previene el desbordamiento aritmético (*integer overflow*) que sucedería con `(left + right) / 2` cuando el tamaño de la secuencia o los índices superan el rango representable.
  - **Estructura de saltos:** Bifurcación de tres ramas con terminación anticipada (*early exit*) ante igualdad estricta.
* **`binary_search_STL`:** 
  - **Decisión de diseño:** No se utiliza `std::binary_search` de `<algorithm>` porque dicha función descarta la posición y retorna únicamente un `bool`. Al inspeccionar `libstdc++`, `std::binary_search` es un envoltorio directo sobre `std::lower_bound`.
  - Se invoca directamente `std::lower_bound(vect.begin(), vect.end(), target)`, se valida `*it == target` y se computa el índice mediante `std::distance(vect.begin(), it)`.
  - **Optimización de Saltos:** `std::lower_bound` utiliza una bifurcación de una sola rama estricta (`<`), eliminando el early exit. Esto permite al compilador sustituir ramas condicionales por instrucciones de movimiento condicional (`cmov`), reduciendo sustancialmente los fallos de predicción de saltos (*branch mispredictions*).
  - **Complejidad de `std::distance`:** Al operar sobre iteradores de acceso aleatorio (`RandomAccessIterator`), `std::distance` se evalúa en $\mathcal{O}(1)$ tiempo y se compila a solo 2 instrucciones en ensamblador x86_64: `sub` (resta de punteros) y `sar $3` (desplazamiento a la derecha de 3 bits para dividir por 8 bytes).

### 3. Búsqueda Galopante (Galloping / Exponential Search)
* **Fase 1 (Expansión Exponencial):** Comienza en `left = 1` y duplica sucesivamente el paso (`left *= 2`) mientras `left < n && vect[left] < target`.
* **Fase 2 (Acotamiento):** Determina el límite superior `right = std::min(left, n - 1)` y retrocede `left = left / 2`.
* **Fase 3 (Bisección Acotada):** Aplica búsqueda binaria únicamente en el subrango $[2^{i-1}, \min(2^i, n-1)]$.
* **Ventaja Arquitectónica:** Si el elemento objetivo se encuentra en una posición $k \ll n$, el algoritmo ejecuta únicamente $\mathcal{O}(\log k)$ comparaciones en lugar de $\mathcal{O}(\log n)$. Además, todo el rango consultado reside caliente en la memoria caché L1d (320 KiB) o L2 (7 MiB), minimizando los accesos a RAM externa.

---

## Tabla Exhaustiva de Complejidades

A continuación se detalla la comparación formal entre los algoritmos implementados, considerando tanto el análisis asintótico tradicional como las propiedades de bajo nivel:

| Algoritmo | Variante | Mejor Caso | Caso Promedio | Peor Caso | Espacio Auxiliar | Localidad de Caché | Patrón de Saltos (Branching) |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Búsqueda Secuencial** | `seq_custom` | $\mathcal{O}(1)$ | $\mathcal{O}(n)$ | $\mathcal{O}(n)$ | $\mathcal{O}(1)$ | **Excelente** (Espacial y temporal, prefetcher activo) | Altamente predecible (bucle lineal) |
| **Búsqueda Secuencial** | `seq_stl` | $\mathcal{O}(1)$ | $\mathcal{O}(n)$ | $\mathcal{O}(n)$ | $\mathcal{O}(1)$ | **Excelente** (Vectorizado con SIMD / AVX2) | Mínimo impacto por salto |
| **Búsqueda Binaria** | `bin_custom` | $\mathcal{O}(1)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | **Baja** (Saltos a saltos de $N/2$, frecuentes cache misses) | 3 ramas: susceptible a *branch misprediction* |
| **Búsqueda Binaria** | `bin_stl` | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | **Baja** (Saltos dicotómicos en memoria principal) | 1 rama con `cmov`: mitiga penalización de predicción |
| **Búsqueda Galopante** | `gal_custom` | $\mathcal{O}(1)$ | $\mathcal{O}(\log k)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | **Alta para $k \ll n$** (Subrango confinado en L1/L2) | Predecible en fase 1; 3 ramas en fase 2 |
| **Búsqueda Galopante** | `gal_stl` | $\mathcal{O}(1)$ | $\mathcal{O}(\log k)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | **Alta para $k \ll n$** (Subrango confinado en L1/L2) | Predecible en fase 1; rama única `cmov` en fase 2 |

*Nota:* $n$ denota la longitud total de la secuencia; $k$ representa la posición real de índice donde se ubica el elemento buscado ($0 \le k < n$).

---

## Generador de Datos: Random Cumulative Steps

En [generator.hpp](../generator.hpp), las secuencias de prueba no se generan ordenando un arreglo con `std::sort` ($\mathcal{O}(n \log n)$), sino mediante acumulación monótona directa en $\mathcal{O}(n)$:

```cpp
data.reserve(size);
std::mt19937_64 gen(seed);
std::uniform_int_distribution<key> distrib(step_min, step_max); // step_min >= 1

key acumulador = distrib(gen);
data.push_back(acumulador);

for (std::size_t i = 1; i < size; ++i) {
    acumulador += distrib(gen);
    data.push_back(acumulador);
}
```

### Justificación Teórica:
1. **Garantía Estricta:** Al requerir $\text{step\_min} \ge 1$, se cumple $\Delta_i \ge 1$ para todo $i$, garantizando matemáticamente que:
   $$v[0] < v[1] < v[2] < \dots < v[n-1]$$
   asegurando unicidad y orden ascendente en tiempo estrictamente lineal $\mathcal{O}(n)$.
2. **Rechazo de Distribución Normal (Gaussiana):** Conforme al modelo de árboles de decisión de Knuth, la complejidad de la búsqueda sobre secuencias ordenadas depende exclusivamente del orden relativo de las claves ($\le, =, >$). Una distribución normal genera acumulación artificial en la media y colas dispersas, sesgando las mediciones de memoria sin aportar variabilidad algorítmica real.

---

## Referencias Bibliográficas

1. **Knuth, D. E.** (1998). *The Art of Computer Programming, Volume 3: Sorting and Searching* (2nd ed.). Addison-Wesley.
2. **Bentley, J. L., & Yao, A. C.** (1976). *An almost optimal algorithm for unbounded searching*. Information Processing Letters, 5(3), 82–87.
3. **ISO/IEC 14882:2020.** *Programming Languages — C++ (C++20 Standard)*. International Organization for Standardization.
