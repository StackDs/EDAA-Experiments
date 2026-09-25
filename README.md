# EDAA-Experiments

---

## Objetivo del Repositorio

El propósito central de este repositorio es proporcionar un entorno experimental riguroso, reproducible y de alto rendimiento para el análisis empírico y teórico de estructuras de datos y algoritmos avanzados. 

A diferencia de los análisis asintóticos convencionales ($\mathcal{O}$, $\Omega$, $\Theta$), este proyecto profundiza en la interacción entre la **complejidad teórica** y las **características de la microarquitectura moderna**, tales como:
- **Jerarquía de memoria y efectos de caché:** Transición de datos a través de los niveles L1d, L2, L3 y RAM externa.
- **Predicción de saltos (*Branch Prediction*):** Impacto de bifurcaciones condicionales frente a instrucciones condicionales (`cmov`).
- **Contigüidad espacial y localidad temporal:** Costo de la indirección por punteros dispersos frente a arreglos secuenciales continuos en memoria.
- **Optimizaciones del compilador:** Comportamiento bajo compilación agresiva (`-O3 -march=native`, C++20).

---

## Estructura General del Proyecto

```text
EDAA-Experiments/
├── Boletin_0X/                    # Boletín 0X: Topico asociado
│   ├── benchmarks/                # Scripts bash de automatización y ejecución de experimentos
│   ├── data/                      # Datos crudos recolectados en formato CSV           
│   ├── include/                   # Código fuente de cabeceras e implementaciones          
│   │   ├── Estructuras/                 
│   │   └── Algoritmos/                
│   ├── plots/                     # Scripts de graficación en Python y figuras generadas
│   ├── report/                    # Reporte académico formal en LaTeX 
│   ├── src/                       # Archivos cpp que ejecutan las pruebas y la medición de tiempo
│   ├── Makefile                   # Automatización completa de compilación, ejecución y reporte
│   └── README.md                  
├── utils/                         # Herramientas de instrumentación estadística provistas por la cátedra
│   ├── uhr.cpp                    # Cronómetro de alta precisión con desviación estándar (por Leonardo Lovera)
│   ├── groesse.cpp                # Medición de huella de memoria con Valgrind
│   ├── quartile_nth.cpp           # Cálculo rápido de cuartiles con std::nth_element
│   ├── valgrind_parser.py         # Parser automatizado de logs de consumo de memoria
│   └── README.md                  # Documentación de las herramientas de instrumentación
└── README.md                      
```

---

## Especificaciones del Entorno Experimental

Para garantizar reproducibilidad científica estricta, todos los experimentos fueron ejecutados bajo un entorno Linux bare-metal con aislamiento de afinidad de CPU:

| Parámetro de Hardware / Software | Especificación de la Plataforma |
| :--- | :--- |
| **Procesador (CPU)** | Intel Core i5-13420H (13ª Generación, Raptor Lake, x86_64) |
| **Configuración de Núcleos** | 8 núcleos físicos (4 P-Cores de rendimiento + 4 E-Cores de eficiencia) / 12 hilos |
| **Frecuencia del CPU** | Base: 400 MHz — Frecuencia Turbo Boost máxima: 4.60 GHz |
| **Aislamiento de Afinidad** | Fijado a P-Core #2 mediante `taskset -c 2` para mitigar migración entre hilos |
| **Caché L1d (Datos)** | 320 KiB |
| **Caché L1i (Instrucciones)** | 384 KiB |
| **Caché L2 unificada** | 7 MiB |
| **Caché L3 compartida (LLC)** | 12 MiB (Intel Smart Cache) |
| **Memoria RAM** | 16 GB DDR (15.0 GiB reconocidos por el sistema operativo) |
| **Almacenamiento** | NVMe SSD 512 GB PCIe Gen4 x4 (WD PC SN740) |
| **Sistema Operativo** | Arch Linux (Linux Kernel 6.13+ rolling release) |
| **Compilador C++** | GCC 16.2.1 |
| **Estándar y Flags de Compilación** | C++20 (`-std=c++20`), `-O3`, `-march=native`, `-Wall -Wextra` |
| **Motor de LaTeX** | Tectonic v0.15.0+ |

---

## Referencias y Atribuciones de Código

1. **Repositorio EDAA:**
   - La base conceptual, lineamientos de experimentación y las implementaciones de referencia de *Binary Heap* y *Binomial Heap* provienen del repositorio oficial del curso:  
     Repositorio oficial: [https://github.com/jfuentess/edaa](https://github.com/jfuentess/edaa)  

2. **Instrumentación y Cronometría (Ayudantía):**
   - La herramienta de medición estadística `uhr.cpp` fué desarollada por **Leonardo Lovera**, ayudante del curso.  
     Perfil: [leonardlover](https://github.com/leonardlover)  
     Repositorio de obtención: [uhr](https://github.com/jfuentess/edaa/experimentos/uhr.cpp)  
     Fuente original: 
    
3. **Adaptaciones, Correcciones y Algoritmos Adicionales:**
   - Diseñados e implementados por **Stack**:
     - Motor generador de secuencias monótonas en $\mathcal{O}(n)$ vía *Random Cumulative Steps* ([generator.hpp](Boletin_01/include/generator.hpp)).
     - Variantes de búsqueda secuencial, binaria y galopante con cálculo seguro de punto medio e interfaces STL compatibles.
     - Corrección completa de fugas de memoria (*memory leaks*) en el Heap Binomial (`freeTree`, `delete temp`).
     - Script en Python con barras de error acotadas para generar los graficos de analisis ($\min(\mu, \sigma)$).

---

## Inicio Rápido

### Requisitos Previos
- **Compilador:** `g++` con soporte completo de C++20.
- **Python:** Python 3.10+ con `matplotlib`, `numpy` y `pandas`.
- **LaTeX:** Compilador `tectonic` instalado en el path de usuario.

### Compilación y Ejecución (Boletín 01)
```bash
# Navegar a la carpeta del boletín
cd Boletin_01

# Compilar todos los ejecutables de benchmark (-O3 -march=native)
make all

# Ejecutar la batería completa de experimentos (Búsqueda por tamaño, posición y Heaps)
make run_all

# Generar todos los gráficos estadísticos en la carpeta plots/
make plot

# Compilar el manuscrito en PDF (produce Boletin01.pdf directamente)
make report
```

---

## Módulos Disponibles

* [Boletín 01: Búsqueda en Secuencias y Colas de Prioridad](Boletin_01/README.md)
  * [Módulo de Algoritmos de Búsqueda](Boletin_01/include/search/README.md)
  * [Módulo de Colas de Prioridad (Heaps)](Boletin_01/include/heaps/README.md)
* [Boletín 02: Árboles Binarios de Búsqueda y Autoajustables](Boletin_02/README.md)
* [Herramientas de Soporte Experimental (utils)](utils/README.md)


## Uso Académico

Esta serie de experimentos fue desarrollado exclusivamente con fines académicos para el estudio de Estructura de Datos y Algoritmos Avanzados, diseño de experimentos, el análisis de rendimiento algorítmico y estadistica descriptiva.

**Laboratorio Experimental de Estructuras de Datos y Algoritmos Avanzados**  
*Universidad de Concepción — Departamento de Ingeniería Informática y Ciencias de la Computación (DIICC)*   
**Autor:** Bryan Eliseo Aguirre Fuentes  
**Docente:** Dr. José Fuentes  
**Ayudante:** Leonardo Lovera — GitHub: [leonardlover](https://github.com/leonardlover)  
