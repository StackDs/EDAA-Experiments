# Boletín 02: Árboles de Búsqueda Binaria y Estructuras Autoajustables

**Curso:** Estructuras de Datos y Algoritmos Avanzados (2026-2)  
**Institución:** Universidad de Concepción — DIICC  
**Estudiante:** Bryan Eliseo Aguirre Fuentes (Matrícula: 2024443402)  
**Ayudante:** Leonardo Lovera (LELE) — GitHub: [leonardlover](https://github.com/leonardlover)  

---

## Objetivos del Boletín

Este boletín está dedicado a la investigación experimental y comparativa de estructuras de búsqueda jerárquica:
1. **Árbol Binario de Búsqueda Clásico (BST):** Sin balanceo, susceptible a degeneración lineal ante secuencias ordenadas ($\mathcal{O}(n)$).
2. **Árbol AVL:** Estructura rígidamente balanceada mediante rotaciones basadas en factores de equilibrio ($\Delta h \in \{-1, 0, 1\}$), garantizando altura logarítmica estricta ($\mathcal{O}(\log n)$).
3. **Árbol Splay (Splay Tree):** Estructura autoajustable no balanceada en el peor caso, pero con garantía de complejidad amortizada $\mathcal{O}(\log n)$ mediante la operación `splay` (rotaciones Zig, Zig-Zig y Zig-Zag), optimizada para explotar el principio de localidad temporal de accesos.

---

## Estructura del Directorio

```text
Boletin_02/
├── README.md                       # Este archivo
├── benchmarks/                     # Scripts de ejecución automatizada
├── data/                           # Datos de experimentación
│   ├── basic_ops/                  # Tiempos de inserción, búsqueda y eliminación
│   ├── splay_distributions/        # Desempeño bajo distribuciones Uniforme, Zipf y Gaussiana
│   └── splay_locality/             # Pruebas de acceso con ventana deslizante y 80/20
├── include/                        # Código fuente
│   ├── trees/                      # Implementaciones de BST, AVL y Splay Tree
│   └── utils/                      # Utilidades de generación de claves y rotaciones
├── plots/                          # Visualizaciones gráficas
├── report/                         # Manuscrito y reporte en LaTeX
└── src/                            # Binarios y programas conductores (Drivers)
```

---

## Baterías Experimentales Previstas

1. **Operaciones Básicas en Escenario Equilibrado vs Degenerado:**
   - Inserción secuencial vs aleatoria.
   - Evaluación de la altura alcanzada y costo de rebalanceo (rotaciones AVL vs splaying).
2. **Sensibilidad a la Distribución de Consultas:**
   - Comparación bajo accesos con distribución Uniforme vs distribución Sesgada (Zipfiana/Pareto).
   - Demostración empírica de la propiedad de tiempo de trabajo (*Working Set Property*) del Splay Tree.
3. **Localidad Temporal de Accesos:**
   - Evaluación de patrones de consulta con alta concentración temporal (consultas repetidas en un subconjunto reducido de claves calientes).

---

## Referencias

1. **Adelson-Velsky, G. M., & Landis, E. M.** (1962). *An algorithm for the organization of information*. Soviet Mathematics Doklady.
2. **Sleator, D. D., & Tarjan, R. E.** (1985). *Self-adjusting binary search trees*. Journal of the ACM (JACM), 32(3), 652–686.
3. **Cátedra EDAA UdeC:** Repositorio oficial y guías metodológicas ([https://github.com/jfuentess/edaa](https://github.com/jfuentess/edaa)).
4. **Ayudantía EDAA:** Perfil de herramientas y utilidades ([https://github.com/leonardlover](https://github.com/leonardlover)).
5. **Implementación AVL:** Agarwal, A. *AgAVLTree*. Se adaptaron la organización recursiva de inserción/eliminación y los cuatro casos de rebalanceo LL, LR, RL y RR en [`avl_tree.h`](include/trees/avl_tree.h). Licencia MIT; el aviso de copyright y licencia se conserva en el header. ([Repositorio](https://github.com/Aditya-A-garwal/AgAVLTree), [`src/AgAVLTree.h`](https://github.com/Aditya-A-garwal/AgAVLTree/blob/main/src/AgAVLTree.h)).
6. **Implementación Splay:** Gaztanaga, I. *Boost.Intrusive*, `splaytree_algorithms.hpp`. La secuencia de rotaciones bottom-up (Zig, Zig-Zig y Zig-Zag) se adaptó a nodos propietarios en [`splay_tree.h`](include/trees/splay_tree.h). Licencia Boost Software License 1.0; se conserva el aviso de licencia y la atribución original incluidos en el header. ([Repositorio](https://github.com/boostorg/intrusive), [archivo fuente](https://github.com/boostorg/intrusive/blob/develop/include/boost/intrusive/splaytree_algorithms.hpp)).
7. **Implementación Rojo-Negro:** GCC Project. *GNU libstdc++*, `stl_set.h` y `stl_tree.h`. [`red_black_tree.h`](include/trees/red_black_tree.h) adapta la interfaz de `std::set`; en libstdc++ esta estructura se representa mediante un árbol Rojo-Negro. ([Repositorio GCC](https://github.com/gcc-mirror/gcc), [`stl_set.h`](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/stl_set.h), [`stl_tree.h`](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/stl_tree.h)). El estándar de C++ garantiza las operaciones y complejidades asociativas, pero no exige que `std::set` use una estructura Rojo-Negro ([borrador del estándar, `set`](https://eel.is/c%2B%2Bdraft/set.overview)).
