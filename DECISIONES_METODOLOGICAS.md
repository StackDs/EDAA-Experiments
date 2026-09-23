# BITÁCORA METODOLÓGICA Y DECISIONES DE DISEÑO
**Asignatura:** Estructuras de Datos y Algoritmos Avanzados (2026-2)  
**Boletín:** Boletín 1 — Búsqueda y Colas de Prioridad  
**Autor:** Bryan Eliseo Aguirre Fuentes  
**Matrícula:** 2024443402  
**Nomenclatura oficial de entrega:** `aguirre_bryan_01.pdf` y `aguirre_bryan_01.zip`  
**Profesor:** José Fuentes | **Ayudante:** Leonardo Lovera  

---

## 1. ESPECIFICACIONES TÉCNICAS DEL ENTORNO EXPERIMENTAL (HARDWARE Y SOFTWARE)

Los experimentos se ejecutan en un entorno estrictamente documentado para garantizar reproducibilidad científica:

* **Procesador (CPU):** Intel(R) Core(TM) i5-13420H (13ª generación, arquitectura x86_64, Raptor Lake).
  * **Topología:** 8 núcleos físicos (4 P-cores de alto rendimiento + 4 E-cores de alta eficiencia) / 12 hilos lógicos.
  * **Frecuencias:** Mínima base de 400 MHz; Máxima turbo de 4.60 GHz (4600 MHz).
  * **Jerarquía de Caché:**
    * L1 Data: 320 KiB (8 instancias).
    * L1 Instruction: 384 KiB (8 instancias).
    * L2 Cache: 7 MiB (5 instancias).
    * L3 Cache compartida (*Intel Smart Cache*): 12 MiB (1 instancia compartida).
* **Memoria Principal (RAM):** 16 GB (15.0 GiB reconocidos por el sistema operativo), tecnología DDR.
* **Almacenamiento Secundario:** NVMe SSD de 512 GB, modelo `WD PC SN740 SDDQNQD-512G-1014` (interfaz PCIe Gen4 x4).
* **Entorno de Software:**
  * Sistema Operativo: Arch Linux (Kernel x86_64, Rolling Release).
  * Compilador: GCC versión 16.2.1.
  * Dialecto estándar: C++20 (`-std=c++20`).
  * Flags base de compilación: `-O3 -march=native -Wall -Wextra`.

---

## 2. REQUISITOS NORMATIVOS Y RESTRICCIONES FORMALES DEL BOLETÍN

* **Extensión del Informe:** Entre 5 y 15 páginas en formato PDF. Menos de 5 páginas implica rechazo por falta de contenido técnico; más de 15 páginas viola las bases del boletín.
* **Portada:** Obligatoriamente debe contener Nombre Completo y Número de Matrícula.
* **Empaquetado:** Código fuente completo dentro de un archivo comprimido denominado `aguirre_bryan_01.zip`.
* **Repeticiones Estadísticas:** Mínimo estricto de **32 repeticiones** por cada punto experimental para reducir el ruido instrumental y satisfacer el Teorema del Límite Central.
* **Métricas a Reportar:** Media muestral ($\mu$) y Desviación estándar insesgada ($\sigma$). Opcionalmente cuartiles (mínimo, $Q_1$, mediana $Q_2$, $Q_3$, máximo).
* **Atribución de Fuentes:** Referenciar explícitamente el repositorio del curso (`https://github.com/jfuentess/edaa/`) tanto para el arnés `uhr.cpp` como para las implementaciones de Heaps.

---

## 3. MAPEO Y FUNDAMENTACIÓN TEÓRICA DE LOS ALGORITMOS DE BÚSQUEDA

### A. Algoritmos Evaluados y sus Pares de la STL
1. **Búsqueda Secuencial Propia (`sequential_search`) vs. STL (`std::find`):**
   * *Complejidad:* $O(n)$ en el peor caso y caso promedio; $O(1)$ en el mejor caso.
   * *Decisión de diseño:* Ambas funciones recorren elemento por elemento comprobando igualdad estricta (`==`). No se aplica salida temprana en datos ordenados para mantener paridad idéntica con el contrato de `std::find`.
2. **Búsqueda Binaria Propia (`binary_search_custom`) vs. STL (`std::lower_bound`):**
   * *Complejidad:* $O(\log n)$ en el peor y caso promedio; $O(1)$ en el mejor caso para la implementación de 3 ramas.
   * *Decisión teórica fundamental:* 
     * La versión propia usa la bifurcación clásica de 3 ramas (`mid == target`, `<`, `>`).
     * La STL (`std::lower_bound`) implementa bisección pura con solo **una comparación estricta** (`*mid < val`) para minimizar fallos en la predicción de saltos (*branch mispredictions*) en arquitecturas modernas y garantizar la obtención de la primera ocurrencia.
3. **Búsqueda Galopante Propia (`galloping_search_custom`) vs. Variante STL (`galloping_search_stl`):**
   * *Complejidad:* $O(\log k)$, donde $k$ es el índice donde reside el elemento.
   * *Fase 1 (Acotamiento exponencial):* Saltos de potencias de dos ($1, 2, 4, 8, \dots, 2^m$) hasta que $A[2^m] \ge target$ o $2^m \ge n$. Requiere $\lfloor \log_2 k \rfloor + 1$ pasos.
   * *Fase 2 (Bisección en subrango):* Búsqueda binaria en el rango $[2^{m-1}, \min(2^m, n - 1)]$.
   * *Contraste STL:* Al no existir `std::galloping_search` en `<algorithm>`, se contrasta:
     * Versión propia (acotamiento manual + bisección manual).
     * Versión híbrida STL (acotamiento manual + `std::lower_bound` en el subrango delimitado).
     * Referencia global STL (`std::lower_bound` aplicado sobre todo el vector $[0, n)$).

---

## 4. METODOLOGÍA DE GENERACIÓN DE DATOS (GENERATOR)

### A. Justificación del Rechazo de la Distribución Normal
Se analizó y descartó formalmente la generación de claves mediante distribución gaussiana/normal con elementos no repetidos por dos motivos fundamentales:
1. **Densidad de colisiones en la campana:** La distribución normal concentra más del $68\%$ de los valores en $[\mu \pm \sigma]$. En secuencias de $n = 10^7$ enteros, la densidad de duplicados en el centro es masiva. Forzar unicidad mediante algoritmos de rechazo o estructuras basadas en árboles/hashes deforma la propia distribución y degrada el tiempo de inicialización de segundos a horas.
2. **Independencia algorítmica de la distribución:** Los algoritmos de búsqueda basados en comparación pura (`<`, `==`) no realizan interpolación numérica; su cantidad de pasos depende estrictamente del índice posicional $k$ y del tamaño $n$, siendo matemáticamente inmunes a si los datos provienen de una distribución uniforme, normal o exponencial.

### B. Adopción de "Saltos Aleatorios Positivos Acumulados" (*Random Cumulative Steps*)
Se implementa una generación monótona estrictamente creciente:
$$A[0] = \text{rand}(1, \text{step\_max})$$
$$A[i] = A[i-1] + \text{rand}(1, \text{step\_max})$$

* **Ventajas Metodológicas:**
  1. **Unicidad garantizada:** Dado que el incremento aleatorio es siempre positivo ($\Delta \ge 1$), se garantiza $A[i] > A[i-1]$ sin necesidad de filtrados de duplicados.
  2. **Orden por construcción:** El arreglo queda ordenado de menor a mayor en tiempo lineal $O(n)$, eliminando la sobrecarga computacional de invocar `std::sort` ($O(n \log n)$).
  3. **Biyección índice-clave:** Para el Experimento 2 (Posición $k$), el elemento objetivo es exactamente $A[k]$. Al no existir duplicados, ningún algoritmo puede encontrar el elemento en un índice distinto de $k$.
  4. **Control riguroso de casos infructuosos:** Si se generan únicamente números pares ($A[i] = A[i-1] + 2 \cdot \text{rand}(1, 3)$), la consulta de números impares garantiza formalmente búsquedas no exitosas distribuidas a lo largo de todo el espectro del arreglo.
  5. **Reproducibilidad:** Se fija un parámetro de semilla explícito (`std::uint64_t seed`) empleando el motor de 64 bits `std::mt19937_64`.

---

## 5. AUDITORÍA Y ADAPTACIÓN DEL ARNÉS EXPERIMENTAL (`uhr.cpp`)

Se adopta como referencia el código `uhr.cpp` de la cátedra, introduciendo cuatro salvaguardas metodológicas críticas:

1. **Barrera de Ensamblador contra *Dead-Code Elimination*:**  
   Bajo optimización `-O3`, GCC detecta que las funciones de búsqueda carecen de efectos secundarios y no mutan el estado global. Si el valor de retorno no se utiliza, el compilador elimina por completo la llamada en tiempo de compilación.  
   *Solución:* Se inserta una barrera de optimización por registro:  
   `asm volatile("" : : "r,m"(result) : "memory");`
2. **Paso Multiplicativo vs. Aditivo:**
   * **Experimento 1 (Tamaño $n$ variable):** Se utiliza paso multiplicativo (potencias de 2 o factores de 10) para cubrir eficientemente órdenes de magnitud desde $10^2$ hasta $10^7$ elementos.
   * **Experimento 2 (Posición $k$ variable, $n$ fijo):** Se fija $n$ en $10^7$ (o $10^6$) y se aplica paso aditivo sobre el índice $k$ en percentiles de $0\%$ a $100\%$ (saltos uniformes de $2\%$ o $5\%$).
3. **Aislamiento Estricto del Cronómetro:**  
   La generación del vector y la selección de la clave a buscar se realizan estrictamente **fuera** de la ventana temporal delimitada por `begin_time` y `end_time`.
4. **Métricas Registradas:**  
   El arnés exporta a CSV: $n$ (o $k$), media muestral (`t_mean`), desviación estándar muestral insesgada con corrección de Bessel $N-1$ (`t_stdev`) y los cuartiles de Tukey (`t_Q0` a `t_Q4`).

---

## 6. PROTOCOLO PARA COLAS DE PRIORIDAD (HEAPS)

* **Implementaciones Oficiales:** Descarga directa de `binary_heap.hpp` y `binomial_heap.hpp` del repositorio oficial (`https://github.com/jfuentess/edaa/tree/main/heaps`).
* **Principio de Integridad Hipotética:** Se formulan hipótesis previas e inmutables sobre el rendimiento temporal de cada operación (`insert`, `extract_min`, `merge`, `top`). Si los datos empíricos contradicen la hipótesis teórica (por ejemplo, si la unión del heap binomial es penalizada por asignación dinámica y *cache misses* respecto al heap binario), **la hipótesis se conserva intacta** y se conjeturan las razones arquitectónicas en el informe.

---

## 7. ARQUITECTURA DE ARCHIVOS DEL PROYECTO

```text
EDAA-Experiments/
├── DECISIONES_METODOLOGICAS.md  <-- Bitácora de diseño y justificación científica
├── specs.txt                    <-- Especificaciones de hardware del sistema
└── Boletin_01/                  <-- Directorio autocontenido para aguirre_bryan_01.zip
    ├── Makefile                 <-- Automatización de compilación (-O3, C++20)
    ├── src/
    │   ├── search.hpp           <-- 6 algoritmos de búsqueda (propios y STL)
    │   └── generator.hpp        <-- Generador monótono determinista en O(n)
    ├── benchmarks/
    │   ├── bench_search_size.cpp <-- Experimento 1 (n variable)
    │   └── bench_search_pos.cpp  <-- Experimento 2 (k variable, n fijo)
    ├── heaps/
    │   ├── binary_heap.hpp      <-- Heap binario del curso comentado
    │   ├── binomial_heap.hpp    <-- Heap binomial del curso comentado
    │   └── bench_heaps.cpp      <-- Benchmark de operaciones de heaps
    ├── data/                    <-- Archivos CSV de salida
    ├── plots/
    │   └── plot_boletin01.py    <-- Generador de gráficos con barras de error
    └── report/
        ├── main.tex             <-- Documento LaTeX (5 a 15 páginas)
        └── references.bib       <-- Bibliografía
```

---

## 4.1 FUNDAMENTO TEÓRICO Y REFERENCIAS ACADÉMICAS SOBRE DISTRIBUCIONES

### A. El Modelo de Árbol de Decisión e Invarianza de Distribución
La justificación formal de por qué la distribución métrica de las claves no afecta a los algoritmos evaluados proviene de la teoría clásica de la computación:

1. **Donald E. Knuth — *The Art of Computer Programming, Volume 3: Sorting and Searching* (1973/1998):**
   * *Section 5.3.1 (Minimum-Comparison Sorting)* y *Section 6.2.1 (Searching an Ordered Table)*.
   * Knuth formaliza que todo algoritmo de búsqueda basado en comparaciones binarias o ternarias (`<`, `==`, `>`) puede modelarse como un **árbol de decisión**.
   * Cada comparación solo evalúa el orden relativo entre elementos. Para cualquier transformación estrictamente monótona creciente $f(x)$ aplicada a los datos (por ejemplo, mapear una distribución uniforme a una gaussiana mediante la función de distribución acumulada inversa $\Phi^{-1}$), se preserva la relación de orden:
     $$a < b \iff f(a) < f(b)$$
   * Por consiguiente, la secuencia exacta de ramas y comparaciones ejecutadas por una búsqueda binaria para localizar el elemento en el índice $k$ en una secuencia ordenada de tamaño $n$ depende **exclusivamente del índice $k$ y del tamaño $n$**, siendo estrictamente independiente de la función de densidad de probabilidad subyacente.

2. **Cormen, Leiserson, Rivest, Stein (CLRS) — *Introduction to Algorithms* (MIT Press):**
   * *Capítulo 8 (Sorting in Linear Time)* y análisis de cotas inferiores. Se formaliza que los algoritmos por comparación trazan un camino en el árbol de decisión determinado solo por la permutación y orden relativo de las claves, sin extraer información numérica de sus magnitudes.

3. **Contraejemplo Teórico: Búsqueda por Interpolación (*Interpolation Search*):**
   * *Referencia:* Yao, A. C., & Yao, F. F. (1976). *The expected analysis of interpolation search*. IEEE FOCS.
   * A diferencia de la búsqueda binaria, la búsqueda por interpolación calcula numéricamente la posición estimada:
     $$\text{pos} = \text{low} + \left\lfloor \frac{\text{target} - A[\text{low}]}{A[\text{high}] - A[\text{low}]} \cdot (\text{high} - \text{low}) \right\rfloor$$
   * En este caso, la distribución métrica es determinante: bajo distribución uniforme toma $O(\log \log n)$, pero bajo distribuciones asimétricas o gaussianas se degrada a peor caso $O(n)$. Al no ser un algoritmo evaluado en este boletín, la distribución métrica de los valores carece de impacto en el costo operacional.

### B. Distribución de las Claves frente a Distribución de las Consultas (Ley de Zipf)
Es crucial distinguir entre:
* **Distribución de las claves en el arreglo:** Irrelevante para algoritmos basados en comparaciones (demostrado por Knuth).
* **Distribución de las consultas (frecuencia de acceso a las posiciones $k$):**
  * **Consultas Uniformes ($P(k) = 1/n$):** La posición esperada es $E[k] = n/2$. La búsqueda galopante tarda $O(\log(n/2)) = O(\log n - 1) = O(\log n)$, empatando en clase asintótica con la búsqueda binaria tradicional.
  * **Consultas sesgadas / Ley de Potencias (Distribución de Zipf / 80-20):** En entornos del mundo real, el 80% de las consultas buscan el 20% de los elementos más frecuentes. Si estos elementos residen al inicio de la estructura ($k \ll n$), la **Búsqueda Galopante supera exponencialmente a la Búsqueda Binaria**, resolviendo las consultas en $O(\log k) \approx O(1)$ comparaciones, mientras que la binaria sigue pagando $\sim \log_2 n$ comparaciones en cada consulta.

---

## 4.2 DECISIÓN DE DOMINIO: RESTRICCIÓN EXCLUSIVA A TIPOS ENTEROS (std::int64_t)

Se ha tomado la decisión deliberada de restringir todo el entorno experimental y los tipos de datos evaluados estrictamente a **enteros de 64 bits (`std::int64_t`)**, descartando el uso de números de punto flotante (`float`, `double`).

### Justificación Metodológica:
1. **Prevención de Absorción y Pérdida de Precisión (Estándar IEEE 754):**
   * En arreglos grandes ($n = 10^7$), la técnica de acumulación incremental supera valores de $3 \times 10^7$. En tipos de precisión simple (`float`, 24 bits de mantisa), cualquier valor superior a $2^{24} = 16\,777\,216$ tiene una resolución (ULP) mayor a 1.0. Esto provocaría absorción numérica (redondeo a cero del incremento), generando duplicados artificiales y destruyendo la propiedad de monotonicidad estricta.
2. **Semántica de Igualdad Exacta (`==`):**
   * Los algoritmos de búsqueda tradicionales (`sequential_search`, `binary_search_custom`) y la STL (`std::find`) operan bajo igualdad estricta. En punto flotante, la igualdad exacta es susceptible a errores de representación decimal y redondeo de la FPU.
3. **Aislamiento del Rendimiento Algorítmico Puro:**
   * Las operaciones de enteros de 64 bits se resuelven en la Unidad Aritmético-Lógica (ALU) con latencia de 1 ciclo en registros de propósito general, eliminando el ruido arquitectónico de registros vectoriales/FPU (XMM/YMM) y el manejo de estados de excepción flotante (`NaN`, subnormales, infinitos).

---

## 4.3 COMPATIBILIDAD CON NÚMEROS NEGATIVOS EN EL DOMINIO DE CLAVES ($\mathbb{Z}$)

Se analizó y documentó formalmente el impacto de permitir valores negativos en las claves almacenadas en el arreglo:

1. **Invarianza Algorítmica y Modelo de Árbol de Decisión:**
   * Los algoritmos de búsqueda basados en comparación (`sequential_search`, `binary_search_custom`, `galloping_search_custom`) y sus equivalentes en la STL solo requieren que el conjunto de claves satisfaga una **relación de orden total estricto** ($<$).
   * El conjunto de enteros con signo $\mathbb{Z}$ posee un orden total canónico idéntico en el dominio negativo:
     $$-1000 < -250 < -15 < 0 < 42 < 500$$
   * La secuencia de comparaciones y el camino recorrido en el árbol de decisión de Knuth es estrictamente idéntico tanto si el arreglo contiene números negativos, positivos o mixtos.

2. **Comportamiento a Nivel de Hardware y Arquitectura (x86_64):**
   * En el procesador Intel Core i5-13420H, los enteros de 64 bits con signo (`std::int64_t`) se representan en **complemento a dos**.
   * La comparación de dos valores con signo (`vec[mid] < target`) se ejecuta en la Unidad Aritmético-Lógica (ALU) mediante la instrucción `cmp` seguida de saltos condicionales con signo (`jl` / `jge`).
   * La latencia de ciclo de reloj de la instrucción es de **1 ciclo**, indistinguible de comparaciones entre números no negativos.

3. **Corrección en Búsqueda Galopante:**
   * El caso base `if (vec[0] > target) return -1;` se evalúa correctamente (si el arreglo inicia en $-500$ y se busca $-800$, $-500 > -800$ es verdadero y finaliza en $O(1)$).
   * La fase de acotamiento exponencial `while (bound < n && vec[bound] < target)` evalúa la relación $<$ de forma monótona sin afectación por el signo.

4. **Diseño del Generador:**
   * El generador de datos por saltos acumulados puede aceptar un parámetro de desplazamiento inicial `valor_inicial` (pudiendo ser negativo, ej. `-static_cast<T>(size * 2)`), manteniendo los incrementos estrictamente positivos ($\Delta \ge 1$) para preservar la unicidad y el orden ascendente sin importar el signo del primer elemento.
   * **Distinción crítica:** El espacio de las **claves** (`vec[i]`) pertenece a $\mathbb{Z}$ (puede ser negativo), mientras que el espacio de los **índices** ($k$) y la variable independiente de tamaño ($n$) pertenecen estrictamente a $\mathbb{N}_0$ (no negativos).

---

## 3.1 ANATOMÍA DE LOS ITERADORES STL Y EVALUACIÓN EN TIEMPO CONSTANTE $O(1)$ DE `std::distance`

En las implementaciones que utilizan la STL (`sequential_search_stl`, `binary_search_stl`, `galloping_search_stl`), las funciones estándar de C++ (`std::find`, `std::lower_bound`) retornan un **iterador** en lugar de un índice entero. Para unificar las interfaces y retornar la posición base cero, se utiliza:
```cpp
return static_cast<std::int64_t>(std::distance(vec.begin(), it));
```

### Análisis de Complejidad y Funcionamiento Interno:
1. **Categoría de Iteradores en `std::vector` (Contiguous / Random Access):**
   * A diferencia de contenedores basados en nodos enlazados (`std::list`, `std::forward_list`) donde `std::distance` debe avanzar paso a paso mediante un bucle `while` en tiempo $O(k)$, en `std::vector` los elementos residen en un bloque de memoria física contiguo.
   * Por contrato del estándar C++20 (`std::contiguous_iterator` y `std::random_access_iterator`), los iteradores encapsulan punteros directos a memoria (`T*`).
2. **Aritmética de Punteros en $O(1)$:**
   * La biblioteca estándar especializa `std::distance` para iteradores de acceso aleatorio realizando una resta directa de punteros:
     $$\text{distancia} = \frac{\text{dirección de } it - \text{dirección de } vec.begin()}{\text{sizeof}(T)}$$
   * En arquitecturas x86_64 con `-O3`, GCC traduce esta operación a dos instrucciones de máquina en la ALU:
     ```nasm
     sub rax, rdi   ; Resta de direcciones de memoria en registros
     sar rax, 3     ; Desplazamiento aritmético a la derecha (división por 8 bytes de int64_t)
     ```
   * **Tiempo de ejecución:** **1 ciclo de reloj** ($\approx 0.22$ ns a 4.6 GHz) con **cero accesos adicionales a memoria RAM**.
3. **Validez Metodológica:**
   * Se descarta formalmente la sospecha de que `std::distance` realice un segundo recorrido sobre la secuencia. La STL realiza un único recorrido $O(n)$ o $O(\log n)$, y la obtención del índice numérico tiene un costo asintótico y práctico estrictamente $O(1)$, garantizando una comparación perfectamente simétrica con las implementaciones propias.

---

## 3.2 JUSTIFICACIÓN TÉCNICA DE LA ELECCIÓN DE `std::lower_bound` SOBRE `std::binary_search`

Se justificó y documentó formalmente por qué la contraparte de búsqueda binaria de la STL en este proyecto es `std::lower_bound` y no `std::binary_search`:

1. **Destrucción de Información Posicional en `std::binary_search`:**
   * La firma de `std::binary_search` retorna estrictamente un valor booleano (`bool`):
     ```cpp
     template <class ForwardIt, class T>
     bool binary_search(ForwardIt first, ForwardIt last, const T& value);
     ```
   * En el **Experimento 2 (Efecto de la posición $k$ con $n$ fijo)**, es indispensable conocer el índice del elemento para validar formalmente que el algoritmo localizó la posición correcta (`assert(res == k)`). Un valor booleano solo informa presencia, descartando la ubicación en memoria.

2. **Anatomía Interna en GNU `libstdc++` (GCC):**
   * En el código fuente oficial de GCC (`<bits/stl_algo.h>`), `std::binary_search` **no es un algoritmo independiente**, sino un simple envoltorio (*wrapper*) de dos líneas implementado de la siguiente manera:
     ```cpp
     template<typename _ForwardIterator, typename _Tp>
     bool binary_search(_ForwardIterator __first, _ForwardIterator __last, const _Tp& __val) {
         __first = std::lower_bound(__first, __last, __val);
         return __first != __last && !(__val < *__first);
     }
     ```
   * Por lo tanto, el **motor algorítmico real** de búsqueda binaria en la STL es `std::lower_bound`.

3. **Cero Sobrecarga y Simetría de Contrato:**
   * Al invocar directamente `std::lower_bound`, el benchmark accede al algoritmo de bisección puro sin pagar la sobrecarga de una comparación extra para convertir el resultado a booleano, y permite obtener la posición mediante aritmética de punteros en $O(1)$ (`std::distance`), manteniendo paridad contractual absoluta con las implementaciones manuales.
