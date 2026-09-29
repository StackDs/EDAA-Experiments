# Funciones auxiliares para los experimentos de arboles

- `tree_sequences.h` crea las claves unicas, baraja el orden de insercion, separa las claves frecuentes y genera el orden de eliminacion.
- `discrete_queries.h` genera vectores de consultas uniformes, sesgadas o geometricas.
- `tree_benchmark.h` recorre los vectores y llama a `insert`, `search` o `erase` del arbol. Cada funcion devuelve la cantidad de operaciones exitosas.

Los encabezados no miden tiempos ni guardan resultados. La medicion, las repeticiones, los cuartiles y los CSV se implementaran en un `.cpp` basado en `utils/uhr.cpp`.
