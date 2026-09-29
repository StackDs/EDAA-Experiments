#pragma once


#include "tree_sequences.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>


template <std::integral key>
std::vector<key> generar_consultas_uniformes(const std::vector<key> &claves,
                                             std::size_t cantidad,
                                             std::uint64_t seed) {
  std::mt19937_64 gen(seed);
  std::uniform_int_distribution<std::size_t> elegir(0, claves.size() - 1);
  std::vector<key> consultas;
  consultas.reserve(cantidad);

  for (std::size_t i = 0; i < cantidad; ++i) {
    consultas.push_back(claves[elegir(gen)]);
  }
  return consultas;
}


template <std::integral key>
std::vector<key> generar_consultas_sesgadas(const GruposClaves<key> &grupos,
                                            std::size_t cantidad,
                                            double probabilidad_frecuentes,
                                            std::uint64_t seed) {
  std::mt19937_64 gen(seed);
  std::bernoulli_distribution elegir_grupo(probabilidad_frecuentes);
  std::uniform_int_distribution<std::size_t> elegir_frecuente(
      0, grupos.frecuentes.size() - 1);
  std::uniform_int_distribution<std::size_t> elegir_otra(
      0, grupos.otras.size() - 1);
  std::vector<key> consultas;
  consultas.reserve(cantidad);

  for (std::size_t i = 0; i < cantidad; ++i) {
    if (elegir_grupo(gen)) {
      consultas.push_back(grupos.frecuentes[elegir_frecuente(gen)]);
    } else {
      consultas.push_back(grupos.otras[elegir_otra(gen)]);
    }
  }
  return consultas;
}

template <std::integral key>
std::vector<key>
generar_consultas_geometricas(const std::vector<key> &popularidad,
                              std::size_t cantidad, double p,
                              std::uint64_t seed) {
  std::mt19937_64 gen(seed);
  std::geometric_distribution<std::size_t> elegir_rango(p);
  std::vector<key> consultas;
  consultas.reserve(cantidad);

  while (consultas.size() < cantidad) {
    const std::size_t rango = elegir_rango(gen);
    if (rango < popularidad.size()) {
      consultas.push_back(popularidad[rango]);
    }
  }
  return consultas;
}
