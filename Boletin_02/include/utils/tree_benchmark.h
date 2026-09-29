#pragma once

#include <cstddef>
#include <vector>

// Funciones auxiliares para poblar un árbol, ejecutar consultas y eliminar
// claves.

template <class Arbol, class key>
std::size_t poblar_arbol(Arbol &arbol, const std::vector<key> &claves) {
  std::size_t insertadas = 0;
  for (const key &clave : claves) {
    insertadas += arbol.insert(clave);
  }
  return insertadas;
}

template <class Arbol, class key>
std::size_t ejecutar_consultas(Arbol &arbol,
                               const std::vector<key> &consultas) {
  std::size_t encontradas = 0;
  for (const key &clave : consultas) {
    encontradas += arbol.search(clave);
  }
  return encontradas;
}

template <class Arbol, class key>
std::size_t eliminar_claves(Arbol &arbol, const std::vector<key> &claves) {
  std::size_t eliminadas = 0;
  for (const key &clave : claves) {
    eliminadas += arbol.erase(clave);
  }
  return eliminadas;
}
