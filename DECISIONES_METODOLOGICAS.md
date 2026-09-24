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
4. **Métricas Registradas y Tamaño Muestral ($N = 128$):**  
   El arnés exporta a CSV: $n$ (o $k$), media muestral (`t_mean`), desviación estándar muestral insesgada con corrección de Bessel $N-1$ (`t_stdev`) y los cuartiles de Tukey (`t_Q0` a `t_Q4`).  
   Se adopta formalmente $N = 128$ repeticiones ($2^7$) superando el piso mínimo de 32 de la cátedra. Esto reduce el error estándar de la media ($\text{SEM} = \sigma / \sqrt{128} \approx \sigma / 11.31$, reducción del $50\%$ del margen de error respecto a $N=32$), garantiza particiones enteras exactas de 32 datos por cuartil, y disuelve la influencia de cualquier interrupción espuria del sistema operativo.

---

## 6. PROTOCOLO PARA COLAS DE PRIORIDAD (HEAPS)

* **Implementaciones Oficiales:** Descarga directa de `binary_heap.hpp` y `binomial_heap.hpp` del repositorio oficial (`https://github.com/jfuentess/edaa/tree/main/heaps`).
* **Principio de Integridad Hipotética:** Se formulan hipótesis previas e inmutables sobre el rendimiento temporal de cada operación (`insert`, `extract_min`, `merge`, `top`). Si los datos empíricos contradicen la hipótesis teórica (por ejemplo, si la unión del heap binomial es penalizada por asignación dinámica y *cache misses* respecto al heap binario), **la hipótesis se conserva intacta** y se conjeturan las razones arquitectónicas en el informe.
* **Gestión de Memoria y RAII en Heap Binomial (Corrección Crítica de Fuga):**  
  El código base de la cátedra carecía de destructor (`delete`), lo que provocaba fugas acumuladas de más de $5\text{ GB}$ de RAM al ejecutar 128 repeticiones para $n = 10^6$. Se introdujo formalmente:
  1. Destructor `~binomial_heap()` y método `clear()` con liberación recursiva post-orden de nodos (`freeTree`).
  2. Liberación explícita del nodo mínimo extraído en `extractMin` (`delete temp`).
  3. Deshabilitación de copia (`delete` en constructor de copia y operador de asignación) para prevenir *double-free* por punteros compartidos.
  4. Semántica de movimiento (*Move Semantics*) y transferencia de posesión en `meld` (`h.roots.clear()`) para garantizar que la unión de árboles en $\mathcal{O}(\log n)$ transfiera la titularidad de los punteros sin peligro de desreferenciación dangling ni doble liberación.
  5. Optimización de la operación `top`: Al ser de solo lectura, el heap se preconstruye una sola vez por tamaño fuera del bucle de 128 corridas, reduciendo la presión sobre el asignador de memoria a solo $\approx 32\text{ MB}$ planos.
* **Aislamiento de Núcleo (Taskset en P-Core):**  
  Todos los scripts de ejecución fijan la afinidad de CPU mediante `taskset -c 2` hacia el P-Core 2 del Intel Core i5-13420H, neutralizando por completo el ruido del planificador y previniendo caídas de rendimiento por migración hacia E-Cores.

---

## 7. ARQUITECTURA DE ARCHIVOS DEL PROYECTO

```text
EDAA-Experiments/
├── DECISIONES_METODOLOGICAS.md  <-- Bitacora de diseno y justificacion cientifica
├── specs.txt                    <-- Especificaciones de hardware del sistema
├── utils/                       <-- Utilidades de catedra (uhr.cpp, quartile_nth.cpp, etc.)
└── Boletin_01/                  <-- Directorio autocontenido para aguirre_bryan_01.zip
    ├── Makefile                 <-- Automatizacion de compilacion (-O3, C++20) y corridas
    ├── bin/                     <-- Binarios compilados (bench_search_size, bench_search_pos, bench_heaps)
    ├── include/
    │   ├── generator.hpp        <-- Generador monotono determinista en O(n)
    │   ├── search/              <-- Cabeceras de busqueda (secuencial, binaria, galopante)
    │   └── heaps/               <-- binary_heap, binomial_heap y agregador heaps.hpp
    ├── src/
    │   ├── bench_search_size.cpp <-- Experimento 1 (n variable)
    │   ├── bench_search_pos.cpp  <-- Experimento 2 (k variable, n fijo)
    │   └── bench_heaps.cpp       <-- Experimento 3 (Heaps: 5 operaciones)
    ├── benchmarks/              <-- Scripts de orquestacion Bash
    │   ├── run_search_size.sh   <-- Ejecucion Experimento 1
    │   ├── run_search_pos.sh    <-- Ejecucion Experimento 2
    │   ├── run_heaps.sh         <-- Ejecucion Experimento 3
    │   └── run_all.sh           <-- Ejecucion suite completa
    ├── data/                    <-- Archivos CSV de salida
    │   ├── search_size/         <-- 6 CSVs de busqueda por tamano
    │   ├── search_pos/          <-- 6 CSVs de busqueda por posicion
    │   └── heaps/               <-- 10 CSVs de operaciones de heaps
    ├── plots/                   <-- Scripts Python y graficos generados
    └── report/                  <-- Documento LaTeX (5 a 15 paginas)
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

---

## 5.1 ARQUITECTURA Y CICLO DE VIDA DE `bench_search_size.cpp` (EXPERIMENTO 1)

Se diseñó e implementó el ejecutable experimental para estudiar la escalabilidad temporal en función del tamaño de entrada ($n$):

1. **Reutilización del Núcleo de `utils/uhr.cpp`:**
   * Se preserva la función de visualización de progreso en terminal `display_progress(u, v)`.
   * Se adopta la función `quartiles(times, q)` para el cómputo de los cinco números de Tukey ($Q_0, Q_1, Q_2, Q_3, Q_4$).
   * Se preserva el cálculo de la media muestral ($\mu$) y de la desviación estándar insesgada con corrección de Bessel ($\sigma$ sobre $N - 1$).
   * Se mantiene el encabezado canónico de salida: `n,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4`.

2. **Diferenciación y Mejoras Metodológicas frente a `uhr.cpp`:**
   * **Selector de Algoritmo por Argumentos (CLI):** El ejecutable parametriza la variante a medir (`seq`, `seq_stl`, `bin`, `bin_stl`, `gal`, `gal_stl`) mediante argumentos de línea de comandos, desacoplando la lógica de medición de la recompilación del binario.
   * **Paso Multiplicativo ($n \leftarrow n \times \text{factor}$):** Para abarcar órdenes de magnitud entre $10^3$ y $10^7$, se implementa avance multiplicativo (por defecto duplicación: $\times 2.0$), generando una densidad uniforme de puntos sobre escalas logarítmicas.
   * **Aislamiento Estricto del Setup:** La generación del vector mediante `generar_vector<int64_t>(n, seed)` y la precomputación determinista de las claves de consulta $k_i \in [0, n-1]$ ocurren **fuera** de la ventana cronometrada por `std::chrono::high_resolution_clock::now()`.
   * **Condición de Consulta Justa (Caso Promedio):** Las claves consultadas varían en cada una de las 32 repeticiones utilizando una semilla fija reproducible (`11223344ULL + n`), asegurando que todos los algoritmos evalúen exactamente el mismo conjunto de consultas.
   * **Barrera Inline contra Dead-Code Elimination:** Se añade `asm volatile("" : : "r,m"(res) : "memory");` para permitir compilar con `-O3 -march=native` sin que el optimizador elimine las llamadas a las funciones de búsqueda.
   * **Validación de Corrección:** Se introduce `assert(res == target_indices[i])` tras el cronómetro para garantizar formalmente que la medición corresponde a una búsqueda exitosa en la posición esperada.

---

## 5.2 DETALLE Y JUSTIFICACIÓN DEL PARSEO DE LÍNEA DE COMANDOS (CLI) FRENTE A `uhr.cpp`

En `utils/uhr.cpp` original, la función `validate_input` procesaba 5 argumentos fijos (`argc == 6`), obligando a *hardcodear* la función a probar dentro del código y restringiendo el avance únicamente a pasos aditivos enteros (`std::stoll`).

### Justificación de las Modificaciones en el Arnés:
1. **Inclusión del Selector de Algoritmo (`argv[1]`):**
   * En lugar de recompilar el programa 6 veces para cada algoritmo, se pasa como argumento un identificador textual (`seq`, `seq_stl`, `bin`, `bin_stl`, `gal`, `gal_stl`).
   * La función `parse_algorithm` mapea este texto hacia un `enum class Algorithm`, permitiendo ejecutar barridos automatizados en scripts de Bash con un solo binario.
2. **Paso Multiplicativo con Decimales (`std::stod`):**
   * En el Experimento 1 (Tamaño $n$), evaluar de $10^3$ a $10^7$ mediante incrementos aditivos generaría miles de puntos innecesarios o saltos desproporcionados que omitirían los órdenes de magnitud inferiores.
   * Al parametrizar `step_factor` como un número decimal con `std::stod` (ej. `2.0` para duplicar), el tamaño progresa exponencialmente ($1000 \to 2000 \to 4000 \dots$), generando datos equidistantes en escala semilogarítmica y log-log.
3. **Mapeo para el Experimento de Posición (`bench_search_pos.cpp`):**
   * En el Experimento 2, la variable independiente no es $n$ sino $k$ (la posición del elemento).
   * La firma de entrada se adapta a:
     `./bin/bench_search_pos <ALGO> <CSV_SALIDA> <RUNS> <SIZE_N> <LOWER_K> <UPPER_K> <STEP_K>`
   * El vector de tamaño fijo $n$ se genera **una sola vez fuera de todos los bucles**, ahorrando gigabytes de asignaciones repetitivas de memoria. Para abarcar exactamente del $0\%$ al $100\%$ sin truncamiento de frontera, se fija $N = 10\,000\,001$ con $k \in [0, 10\,000\,000]$ y paso $\Delta k = 500\,000$, produciendo exactamente 21 percentiles equidistantes ($0\%, 5\%, 10\%, \dots, 100\%$).

---

## 5.3 CALIBRACIÓN Y JUSTIFICACIÓN ARQUITECTÓNICA DE LOS RANGOS DE ENTRADA (INPUT SIZES)

Se analizó y fundamentó formalmente la dimensionalidad de las entradas para cada experimento, determinando por qué los rangos seleccionados son óptimos y por qué ampliarlos no incrementa la fiabilidad científica:

### A. Experimento 1 (Búsqueda por Tamaño: $n \in [10^3, 10^7]$)
1. **Recorrido Integral de la Jerarquía de Memoria del Hardware:**
   En el procesador del sistema experimental (Intel Core i5-13420H), el tamaño en memoria del arreglo de enteros (`int64_t`, 8 bytes) atraviesa todos los estratos físicos de almacenamiento:
   * **$n = 10^3$ ($8\text{ KB}$):** Reside íntegramente en la **Caché L1 de datos** (48 KB por P-Core). Acceso con latencia inferior a $1\text{ ns}$.
   * **$n = 10^4 - 10^5$ ($80\text{ KB} - 800\text{ KB}$):** Desborda L1 pero reside completamente en la **Caché L2** (1.25 MB por P-Core).
   * **$n = 10^6$ ($8\text{ MB}$):** Desborda L2 y reside en la **Caché L3 compartida** (Smart Cache de 12 MB).
   * **$n = 10^7$ ($80\text{ MB}$):** Desborda masivamente la memoria caché L3 (12 MB), forzando al controlador de memoria a realizar transferencias de líneas de caché (*cache line fills*) desde la **memoria principal RAM DDR**.
2. **Inutilidad de Expandir a $n \ge 10^8$ ($800\text{ MB}$):**
   * Superar $10^7$ elementos no expone ningún nivel jerárquico de memoria nuevo (continúa siendo RAM DDR).
   * Incrementa en un orden de magnitud la latencia de asignación e I/O, aumentando la carga térmica sostenida sobre el procesador y el riesgo de *thermal throttling* en la laptop, sin aportar información asintótica adicional.
3. **Amplitud Asintótica:** El rango cubre **4 órdenes de magnitud** ($10^3, 10^4, 10^5, 10^6, 10^7$), estándar universal para discriminar rigurosamente pendientes lineales ($\mathcal{O}(n)$) de logarítmicas ($\mathcal{O}(\log n)$) en escalas log-log con coeficientes de correlación $R^2 > 0.999$.

### B. Experimento 2 (Búsqueda por Posición: $k \in [0, 10^7]$ con $N = 10^7 + 1$)
1. **Aislamiento del Peor Caso Absoluto ($100\%$):**
   Al fijar $N = 10\,000\,001$ con $\Delta k = 500\,000$, se evalúan exactamente 21 percentiles equidistantes ($0\%, 5\%, 10\%, \dots, 95\%, 100\%$).
2. **Densidad Óptima:** 21 puntos garantizan una interpolación lineal perfecta en búsqueda secuencial y capturan con alta definición el codo logarítmico temprano de la búsqueda galopante ($\mathcal{O}(\log k)$).

### C. Experimento 3 (Colas de Prioridad: Heaps con $n \in [10^3, 10^6]$)
1. **Límite Natural por Punteros Dinámicos:**
   A diferencia del Binary Heap (vector plano continuo de $4\text{ MB}$ a $n = 10^6$), el Binomial Heap reserva $10^6$ nodos individuales dispersos en el heap del sistema operativo mediante `new Node` ($\approx 32\text{ MB}$ por instancia).
2. **Prevención de Saturación de Memoria:**
   Evaluar hasta $n = 10^6$ cubre 3 órdenes de magnitud suficientes para evidenciar la divergencia entre $\mathcal{O}(n)$ y $\mathcal{O}(n \log n)$ en la construcción, y entre $\mathcal{O}(n)$ y $\mathcal{O}(\log n)$ en la fusión (*meld*), sin desbordar el asignador de memoria (`glibc malloc`). 
---
## 6. METODOLOGÍA DE VISUALIZACIÓN CIENTÍFICA Y ESTRUCTURA DE CONTRASTES
### A. Rechazo de la Gráfica Única Sobrecargada (*Plot Cluttering*)
En versiones iniciales de evaluación algorítmica, graficar simultáneamente los 6 algoritmos de búsqueda en un único lienzo introduce severas distorsiones visuales y analíticas:
1. **Colapso de escala por dominancia lineal:** La búsqueda secuencial ($\mathcal{O}(n)$) alcanza más de $5\text{ ms} = 5\,000\,000\text{ ns}$ en $n = 10^7$, comprimiendo a $0$ a las búsquedas binaria y galopante ($\approx 30 - 800\text{ ns}$).
2. **Ocultamiento de diferencias sutiles en escalas logarítmicas:** Aunque una escala log-log permite visualizar simultáneamente $\mathcal{O}(n)$ y $\mathcal{O}(\log n)$, la proximidad de 6 curvas superpuestas oculta el impacto del diseño de bajo nivel (p. ej., bifurcación de 3 ramas en `binary_search_custom` vs. bisección pura de 1 rama en `std::lower_bound`).
### B. Estructura y Taxonomía Modular por Experimento
La generación gráfica se organiza estrictamente en tres módulos independientes con almacenamiento segregado en subdirectorios (`plots/Experimento_1_Size/`, `plots/Experimento_2_Posicion/`, `plots/Experimento_3_Heaps/`):

1. **Experimento 1: Variación por Tamaño de Secuencia ($n$):**
   * **Contraste 1 a 1 (Cada búsqueda vs. su equivalente en la STL):**
     * Secuencial: `seq` vs `seq_stl` (`std::find`). Escala adecuada en $\mu\text{s}$ para evidenciar paridad de compilación vectorial.
     * Binaria: `bin` vs `bin_stl` (`std::lower_bound`). Eje en $\text{ns}$, contrastando 3 ramas vs 1 rama.
     * Galopante: `gal` vs `gal_stl`. Eje en $\text{ns}$, analizando bisección manual vs iteradores de la STL.
   * **Contraste entre Algoritmos Custom (Propios):**
     * Panorámica General: `seq` vs `bin` vs `gal` (Escala Log-Log para contrastar $\mathcal{O}(n)$, $\mathcal{O}(\log n)$ y $\mathcal{O}(\log k)$).
     * Detalle Sublineal: `bin` vs `gal` (Escala lineal en $\text{ns}$).

2. **Experimento 2: Variación por Posición del Objetivo ($k$):**
   * **Contraste 1 a 1 (Cada búsqueda vs. su equivalente en la STL):**
     * Secuencial: `seq` vs `seq_stl` (`std::find`). Eje en $\text{ms}$ en función de $k/n$ ($0\%$ a $100\%$).
     * Binaria: `bin` vs `bin_stl` (`std::lower_bound`). Eje en $\text{ns}$ mostrando invariancia posicional.
     * Galopante: `gal` vs `gal_stl`. Eje en $\text{ns}$ mostrando aceleración en posiciones tempranas.
   * **Contraste entre Algoritmos Custom (Propios):**
     * Panorámica General: `seq` vs `bin` vs `gal` (Escala Log-Y vs $k$).
     * Detalle Sublineal: `bin` vs `gal` (Escala lineal en $\text{ns}$, evidenciando el cruce en $k$ temprano).

3. **Experimento 3: Colas de Prioridad (Heaps):**
   * Se evalúan las 5 operaciones fundamentales (`build`, `top`, `insert`, `extract_min`, `meld`), contrastando directamente `binary_heap` (vectorial continuo STL) frente a `binomial_heap` (bosque de árboles dinámicos con punteros).

### C. Tratamiento Riguroso de Barras de Error y Límites Físicos
* **Acotamiento Inferior al Cero Físico:** En experimentos donde la varianza muestral es alta debido a interrupciones del kernel o fallos de caché (`t_stdev > t_mean`), las barras de error simétricas estándar penetran en tiempos negativos ($\le 0\text{ ns}$), lo cual es físicamente imposible.
* **Solución Implementada:** La función `get_clamped_yerr` acota el extremo inferior del error a $\min(\mu, \sigma)$ y fija `ax.set_ylim(bottom=0)` en escalas lineales, manteniendo la fidelidad estadística del percentil superior sin violar la causalidad temporal física.
* **Exportación Exclusiva a PNG (300 DPI):** Las figuras se exportan directamente en formato rasterizado `.png` a 300 DPI (alta resolución) sin generar archivos `.pdf` redundantes, resultando suficiente para la inspección y análisis visual de los experimentos. Si se requiriera exportación vectorial en PDF en el futuro, el arnés dispone del flag opcional `--pdf`.
