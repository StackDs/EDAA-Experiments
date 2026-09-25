# Módulo de Colas de Prioridad: Binary Heap vs Binomial Heap

---

## Descripción General

Este módulo implementa y contrasta dos arquitecturas fundamentales para colas de prioridad (*Priority Queues*) orientadas a la obtención eficiente del elemento mínimo (*Min-Heap*):

1. **Heap Binario (`binary_heap`):** Estructura secuencial contigua donde el árbol binario casi completo se mapea implícitamente sobre un vector dinámico (`std::vector<T>`).
2. **Heap Montículo Binomial (`binomial_heap`):** Estructura enlazada explícita compuesta por un bosque de árboles binomiales $\{B_0, B_1, \dots, B_k\}$, optimizada teóricamente para la operación de fusión (*meld*).

---

## Archivos del Módulo

| Archivo | Contenido |
| :--- | :--- |
| [heaps.hpp](heaps.hpp) | Cabecera agregadora que incluye ambas implementaciones. |
| [binary_heap.cpp](binary_heap.cpp) | Heap Binario basado en vector contiguo y algoritmos de `std::make_heap`. |
| [binomial_heap.cpp](binomial_heap.cpp) | Heap Binomial con árboles binomiales enlazados y gestión RAII de memoria. |

---

## Arquitectura y Funcionamiento Interno

### 1. Montículo Binario (`binary_heap`)
* **Indexación Implícita:** Al ser un árbol binario casi completo, no requiere almacenar punteros entre nodos. Para un nodo en el índice $i$:
  - Hijo izquierdo: $\text{left}(i) = 2i + 1$
  - Hijo derecho: $\text{right}(i) = 2i + 2$
  - Padre: $\text{parent}(i) = \lfloor (i - 1) / 2 \rfloor$
* **Construcción en Tiempo Lineal (`build_heap`):** Utiliza el algoritmo ascendente de Floyd (*sift-down* repetido desde $\lfloor n/2 \rfloor - 1$ hasta la raíz), implementado por `std::make_heap`. Su costo asintótico estricto es $\mathcal{O}(n)$, superando drásticamente a la inserción individual repetida $\mathcal{O}(n \log n)$.
* **Localidad de Memoria:** Almacenado en un bloque de memoria contiguo en el heap. Una línea de caché típica de 64 bytes almacena 8 enteros contiguos de 64 bits (`int64_t`), maximizando la tasa de aciertos (*cache hits*) en L1d (320 KiB) y L2 (7 MiB).

### 2. Montículo Binomial (`binomial_heap`)
* **Estructura del Árbol Binomial $B_k$:**
  - $B_0$ es un nodo aislado.
  - $B_k$ se forma uniendo dos árboles $B_{k-1}$, enlazando la raíz de mayor clave como el hijo más a la izquierda de la raíz de menor clave.
  - Posee exactamente $2^k$ nodos, altura $k$ y su raíz tiene grado $k$.
* **Representación en Memoria:** Emplea el esquema clásico *Left-Child Right-Sibling*:
  ```cpp
  struct Node {
      int data, degree;
      Node *child, *sibling, *parent;
  };
  ```
  Las raíces de los distintos árboles se organizan en orden creciente de grado dentro de una lista doblemente enlazada `std::list<Node*> roots`.
* **Operación de Fusión (`meld`):** Realiza un recorrido entrelazado de las listas de raíces y una pasada de resolución de acarreos (*adjust*), análoga a la suma de dos enteros en base 2. Su costo es estrictamente $\mathcal{O}(\log n)$.

---

## Tabla Comparativa de Complejidades

A continuación se presenta el contraste analítico formal entre ambas estructuras de datos:

| Operación | Binary Heap (Peor Caso) | Binary Heap (Amortizado) | Binomial Heap (Peor Caso) | Binomial Heap (Amortizado) | Mecanismo Algorítmico Principal |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **`build_heap`** | $\mathcal{O}(n)$ | $\mathcal{O}(n)$ | $\mathcal{O}(n \log n)$ | $\mathcal{O}(n)$ | Floyd *sift-down* lineal vs inserciones repetidas |
| **`top` / `getMin`** | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | La raíz siempre está en `data[0]` vs búsqueda en lista de raíces |
| **`insert` / `push`** | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | *Sift-up* con duplicación amortizada vs adición de un $B_0$ y fusión |
| **`extract_min` / `pop`** | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | Reemplazar raíz por último y *sift-down* vs escindir hijos e invertir lista |
| **`meld` / `merge`** | $\mathcal{O}(n + m)$ | $\mathcal{O}(n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | Concatenación de vectores y `make_heap` vs suma binaria con acarreos |
| **`decrease_key`** | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | Propagación hacia arriba por puntero implícito/explícito |
| **`delete`** | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | $\mathcal{O}(\log n)$ | `decrease_key` al infinito negativo seguido de `extract_min` |

---

## Huella de Memoria e Impacto en la Jerarquía de Caché

La brecha de rendimiento observada empíricamente (donde el Heap Binario resulta hasta $15\times$ más rápido en operaciones individuales que el Binomial) se explica mediante la sobrecarga estructural y espacial:

| Característica | Binary Heap | Binomial Heap |
| :--- | :--- | :--- |
| **Memoria por elemento** | **8 bytes** (`int64_t` contiguo) | **32–48 bytes** (valor, grado + 3 punteros de 64 bits + relleno de alineación) |
| **Sobrecarga de punteros** | **0%** (navegación aritmética por índices) | **300% a 500%** de sobrecarga respecto a la clave pura |
| **Asignaciones dinámicas** | **1 sola reasignación geométrica** en vector | **1 invocación a `new`/`malloc` por cada nodo** insertado |
| **Patrón de acceso a caché** | **Secuencial/Predecible:** Excelente afinidad con L1/L2 | **Punteros dispersos:** Saltos aleatorios en memoria (*heap fragmentation*) |
| **Penalización TLB** | Casi nula (páginas contiguas de memoria) | Frecuentes fallos de TLB (*TLB misses*) para $N \ge 10^5$ |

---

## Correcciones de Memoria y RAII Implementadas

La implementación base provista originalmente por la cátedra presentaba (*memory leaks*) acumulativas detectadas mediante Valgrind. Para resolverlo, se incorporaron las siguientes mejoras en [binomial_heap.cpp](binomial_heap.cpp):

1. **Liberación Jerárquica Recursiva (`freeTree`):**
   ```cpp
   void freeTree(Node *node) {
       while (node != nullptr) {
           Node *sibling = node->sibling;
           if (node->child != nullptr) {
               freeTree(node->child);
           }
           delete node;
           node = sibling;
       }
   }
   ```
2. **Destructor RAII (`~binomial_heap`):** Asegura que al salir del ámbito (*scope*), todas las raíces y sus subárboles sean destruidos sin dejar memoria huérfana en el heap del sistema.
3. **Liberación de Nodos Desvinculados:** En `extractMin`, el nodo que contenía el mínimo global es liberado explícitamente mediante `delete temp` tras desvincularlo de la lista de raíces.
4. **Semántica de Movimiento:** Prevención de clonación accidental mediante constructores de copia profunda o transferencia de propiedad por movimiento.

---

## Referencias Bibliográficas

1. **Floyd, R. W.** (1964). *Algorithm 245: Treesort 3*. Communications of the ACM, 7(12), 701.
2. **Vuillemin, J.** (1978). *A data structure for manipulating priority queues*. Communications of the ACM, 21(4), 309–315.
3. **Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C.** (2009). *Introduction to Algorithms* (3rd ed., Chapter 19: Binomial Heaps). MIT Press.
4. **Repositorio EDAA UdeC:** [https://github.com/jfuentess/edaa](https://github.com/jfuentess/edaa) (Dr. José Fuentes & Leonardo Lovera).
5. **Perfil de Ayudantía:** [https://github.com/leonardlover](https://github.com/leonardlover) (Leonardo Lovera).
