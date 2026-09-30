#pragma once

#include "../../../Boletin_01/include/generator.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

// Genera un vector de claves enteras de tamaño `size` con valores
// pseudoaleatorios
template <std::integral key = std::int64_t>
std::vector<key> generar_claves(std::size_t size) {
  return generar_vector<key>(size, 123456789ULL, 1, 1);
}

// Desordena un vector de claves enteras de manera pseudoaleatoria
template <std::integral key>
void barajar_claves(std::vector<key> &claves, std::uint64_t seed) {
  std::mt19937_64 gen(seed);
  std::shuffle(claves.begin(), claves.end(), gen);
}

// Copia y baraja el vector de claves, devolviendo un nuevo vector con el orden
// de eliminación
template <std::integral key>
std::vector<key> generar_orden_eliminacion(const std::vector<key> &claves,
                                           std::uint64_t seed) {
  std::vector<key> orden = claves;
  barajar_claves(orden, seed);
  return orden;
}
