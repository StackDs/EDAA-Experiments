# Boletín 01: Búsqueda en Secuencias y Colas de Prioridad

**Reporte Completo:** [report/Boletin01.pdf](report/Boletin01.pdf)  

---

## Objetivos del Boletín

Este boletín aborda el diseño, implementación, optimización y evaluación empírica de dos familias esenciales de estructuras de datos y algoritmos:
1. **Algoritmos de Búsqueda en Secuencias Monótonas:** Búsqueda Secuencial (Lineal), Búsqueda Binaria y Búsqueda Galopante (Exponencial), contrastando implementaciones propias frente a las utilidades provistas por la biblioteca estándar de C++ (`std::find`, `std::lower_bound`).
2. **Colas de Prioridad (Priority Queues / Min-Heaps):** Heap Binario basado en un arreglo contiguo (`std::vector<int64_t>`) frente a Heap Binomial basado en un bosque dinámico de árboles enlazados mediante punteros.

---

## Estructura del Directorio

```text
Boletin_01/
├── Makefile                        # Automatizacion de compilacion
├── README.md                       
├── benchmarks/                     # Scripts de ejecución automatizada
│   ├── run_all.sh                  # Ejecutor global de todos los experimentos 
│   ├── run_search_size.sh          
│   ├── run_search_pos.sh           
│   └── run_heaps.sh                
├── data/                           # Resultados experimentales en formato CSV
│   ├── search_size/                # Tiempos basados en el tamaño del arreglo
│   ├── search_pos/                 # Tiempos basados en la posición de la clave buscada
│   └── heaps/                      # Tiempos de las 5 operaciones claves en heaps
|
├── include/                        # Código fuente C++ 
│   ├── generator.hpp               # Generador O(n) de secuencias monótonas crecientes pseudoaleatorias
|   |
│   ├── search/                     # Módulo de búsqueda (Sequential, Binary, Galloping)
│   │   ├── README.md               
│   │   ├── search.hpp              # Header unificado
│   │   ├── sequential_search.hpp   # Búsqueda secuencial
│   │   ├── binary_search.hpp       # Búsqueda binaria 
│   │   └── galloping_search.hpp    # Búsqueda galopante
|   |
│   └── heaps/                      # Módulo de colas de prioridad (Binary y Binomial)
│       ├── README.md               
│       ├── heaps.hpp               # Header Unificado
│       ├── binary_heap.cpp         # Implementación de Binary Min-Heap
│       └── binomial_heap.cpp       # Implementación corregida de Binomial Min-Heap
|
├── plots/                          # Gráficos generados y script de visualización
│   ├── plot_boletin01.py           # Generador de gráficos con matplotlib y barras de error
│   ├── Experimento_1_Size/         # Gráficas de búsqueda según tamaño n
│   ├── Experimento_2_Posicion/     # Gráficas de búsqueda según posición k
│   └── Experimento_3_Heaps/        # Gráficas de las 5 operaciones de heaps
|
├── report/                         # Reporte final en formato .tex y su variante compilada en PDF
│   ├── Boletin01.tex               
│   ├── Boletin01.pdf               
└── src/                            # Programas que ejecutan los algoritmos y sus mediciones de tiempo
    ├── bench_search_size.cpp       # Driver para Experimento 1
    ├── bench_search_pos.cpp        # Driver para Experimento 2
    └── bench_heaps.cpp             # Driver para Experimento 3
```

---

## Resumen de los Experimentos

### 1. Experimento 1: Búsqueda variando Tamaño (n)
- **Dominio:** $n \in [10^1, 10^7]$ (15 tamaños evaluados).
- **Condición:** Búsqueda en el peor caso ($k = n - 1$), forzando a los algoritmos a recorrer la máxima profundidad de decisión.
- **Algoritmos evaluados:**
  - `seq_custom` vs `seq_stl` (`std::find`)
  - `bin_custom` vs `bin_stl` (`std::lower_bound` + `std::distance`)
  - `gal_custom` vs `gal_stl` (Galopante con bisección STL)

### 2. Experimento 2: Búsqueda variando Posición (k)
- **Dominio:** Tamaño constante $n = 10^7$, posición $k \in [0, 10^7]$ muestreada en 21 puntos equidistantes ($\Delta = 500{,}000$).
- **NOTA:** Muestrear todos los $10^7$ enteros secuencialmente con 32 repeticiones habría requerido $\approx 38.8$ días de cómputo ininterrumpido. El muestreo uniforme de 21 puntos proporciona significancia estadística con $R^2 > 0.999$.

### 3. Experimento 3: Heaps y Colas de Prioridad
- **Dominio:** $n \in [10^1, 10^7]$ evaluando 5 operaciones fundamentales:
   - `build_heap`: Construcción ascendente de Floyd $\mathcal{O}(n)$ vs inserciones repetidas $\mathcal{O}(n \log n)$.
   - `insert`: Inserción de un nuevo elemento ($\mathcal{O}(\log n)$ peor caso, $\mathcal{O}(1)$ amortizado).
   - `top`: Inspección del mínimo global ($\mathcal{O}(1)$ en Binary vs $\mathcal{O}(\log n)$ en Binomial).
   - `extract_min`: Extracción del elemento mínimo y reestructuración.
   - `meld`: Fusión de dos colas disjuntas de igual tamaño ($\mathcal{O}(n)$ en Binary vs $\mathcal{O}(\log n)$ en Binomial).

---

## Comandos de Compilación y Ejecución

Desde el directorio `Boletin_01/`:

| Comando | Acción |
| :--- | :--- |
| `make all` | Compila todos los ejecutables en `bin/` con `-O3 -march=native -std=c++20`. |
| `make run_all` | Ejecuta la batería completa de los 3 experimentos vía `run_all.sh`. |
| `make run_search_size` | Ejecuta únicamente el Experimento 1 (Búsqueda por tamaño). |
| `make run_search_pos` | Ejecuta únicamente el Experimento 2 (Búsqueda por posición). |
| `make run_heaps` | Ejecuta únicamente el Experimento 3 (Heaps). |
| `make plot` | Ejecuta el script de graficación Python y actualiza todas las figuras. |
| `make report` | Compila `Boletin01.tex` produciendo `report/Boletin01.pdf`. |
| `make clean` | Elimina binarios compilados en `bin/`. |
| `make clean_data` | Limpia los archivos CSV de resultados en `data/`. |
| `make clean_plots` | Elimina los gráficos generados en `plots/`. |
| `make clean_report` | Elimina artefactos temporales de compilación de LaTeX. |

---

## Documentación de Módulos

Para consultar las descripciones detalladas, diagramas de memoria y tablas de complejidad por componente:
* [Módulo de Algoritmos de Búsqueda](include/search/README.md)
* [Módulo de Colas de Prioridad (Heaps)](include/heaps/README.md)
