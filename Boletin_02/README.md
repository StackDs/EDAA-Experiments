# Boletín 02: Árboles de Búsqueda Binaria y Estructuras Autoajustables

**Reporte Completo:** pendiente; el directorio `report/` aún no contiene un informe.

---

## Objetivos del Boletín

Este boletín compara tres implementaciones de árboles de búsqueda que reciben las mismas claves y las mismas secuencias de operaciones:

1. **Árbol AVL:** Mantiene el equilibrio mediante rotaciones y garantiza búsqueda, inserción y eliminación en $\mathcal{O}(\log n)$ en el peor caso.
2. **Árbol Rojo-Negro mediante `std::set`:** Sirve como referencia de la biblioteca estándar. En la implementación de `g++` usada aquí, `std::set` se basa en un árbol Rojo-Negro.
3. **Árbol Splay:** Reorganiza el árbol después de cada búsqueda para estudiar el efecto de consultar repetidamente un subconjunto de claves.

---

## Estructura del Directorio

```text
Boletin_02/
├── Makefile                        # Compilación y objetivos de ejecución
├── README.md
├── benchmarks/                     # Scripts de ejecución automatizada
│   ├── run_all.sh                  # Compila y ejecuta toda la batería
│   ├── run_basic_ops.sh            # Inserción y vaciado completo D0
│   └── run_distributions.sh        # Búsqueda y eliminación D1 de Splay
├── data/                           # Resultados experimentales en CSV
│   ├── basic_ops/                  # Inserción y vaciado completo de los tres árboles
│   ├── splay_distributions/        # Búsquedas uniforme/binomial negativa y D1 de Splay
│   └── splay_locality/             # Reservado; no participa en la batería actual
├── include/                        # Implementaciones y funciones auxiliares en C++
│   ├── trees/
│   │   ├── avl_tree.h
│   │   ├── red_black_tree.h
│   │   └── splay_tree.h
│   └── utils/
│       ├── tree_sequences.h       # Población y orden de eliminación; usa el generador del Boletín 1
│       ├── discrete_queries.h     # Generación de consultas
│       └── tree_benchmark.h        # Inserción, búsqueda y eliminación por lotes
├── plots/
│   └── plot_boletin02.py           # Gráficos a partir de los CSV
├── report/                         # Reservado para el informe
└── src/
    └── bench_trees.cpp             # Conductor y medición mediante uhr
```

---

## Resumen de los Experimentos

La batería usa ocho tamaños ($n = 10^3, 2\cdot10^3, \ldots, 128\cdot10^3$), 64 repeticiones por tamaño, afinidad al núcleo 2 y una pausa de dos segundos entre series. Cada repetición genera las claves únicas $1, \ldots, n$ y las baraja. Las semillas dependen solo de $n$ y del número de repetición, de modo que los tres árboles reciben los mismos datos. La generación queda fuera del intervalo medido. Los CSV contienen tiempo medio por operación en nanosegundos, desviación estándar y cuartiles.

### 1. Experimento 1: Inserción variando el tamaño ($n$)

- **Dominio:** $n \in \{10^3, 2\cdot10^3, \ldots, 128\cdot10^3\}$.
- **Condición:** Insertar las $n$ claves barajadas en un árbol inicialmente vacío.
- **Árboles evaluados:** AVL, Rojo-Negro (`std::set`) y Splay; tres CSV en `data/basic_ops/`.

### 2. Experimento 2: Búsqueda bajo dos distribuciones

- **Dominio:** Los mismos ocho tamaños y tres árboles; seis CSV en `data/splay_distributions/`.
- **Condición uniforme:** Cada consulta elige una de las claves presentes con igual probabilidad.
- **Condición binomial negativa:** Se barajan las claves para asignarles un rango de popularidad. Cada consulta genera ese rango con una binomial negativa de parámetros $r=2$ y $p=3/(0{,}05n+3)$. Si el rango supera $n-1$, se vuelve a sortear. Esta elección concentra aproximadamente el 80 % de las consultas en el primer 5 % de los rangos, que corresponde a claves presentes elegidas al azar.
- **Medición:** Primero se ejecutan $5n$ consultas de preparación fuera del reloj; luego se mide un vector nuevo de $10n$ consultas. Las consultas pueden repetir claves, pero la población inicial contiene claves únicas.

### 3. Experimento 3: Vaciado completo y eliminación tras consultas

- **Vaciado D0:** En los tres árboles se insertan las mismas $n$ claves y luego se eliminan todas en el mismo orden barajado. Se mide el tiempo medio por clave eliminada y se comprueba que el árbol quede vacío. Produce tres CSV en `data/basic_ops/` y un gráfico comparativo de los tres árboles.
- **Eliminación D1:** Solo en Splay se ejecutan antes las consultas uniformes o binomiales negativas del Experimento 2. Después se mide el vaciado completo. Produce dos CSV en `data/splay_distributions/`.

En total se generan **14 CSV** con las operaciones y distribuciones descritas arriba.

---

## Comandos de Compilación y Ejecución

Desde el directorio `Boletin_02/`:

| Comando | Acción |
| :--- | :--- |
| `make all` | Compila `bin/bench_trees` con `g++`, C++20 y `-O3 -march=native`. |
| `make run_all` | Ejecuta la batería completa de los tres experimentos. |
| `make run_basic_ops` | Ejecuta inserción y vaciado D0 en los tres árboles. |
| `make run_distributions` | Ejecuta las búsquedas y los D1 de Splay. |
| `make plot` | Genera los gráficos a partir de los CSV disponibles. |
| `make clean` | Elimina el binario `bin/bench_trees`. |

Los valores predeterminados pueden cambiarse mediante `RUNS`, `LOWER`, `UPPER`, `STEP`, `CPU_CORE`, `PAUSE_SECONDS` y `DATA_ROOT`. Repetir una serie sobrescribe su CSV; los archivos de otras series permanecen.

---

## Documentación de Módulos

- [AVL](include/trees/avl_tree.h)
- [Rojo-Negro](include/trees/red_black_tree.h)
- [Splay](include/trees/splay_tree.h).
