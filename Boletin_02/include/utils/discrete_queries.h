#pragma once


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
std::vector<key>
generar_consultas_binomiales_negativas(const std::vector<key> &popularidad,
                                      std::size_t cantidad,
                                      std::uint64_t seed) {
  std::mt19937_64 gen(seed);
  const double claves_frecuentes =
      0.05 * static_cast<double>(popularidad.size());
  const double p = 3.0 / (claves_frecuentes + 3.0);
  std::negative_binomial_distribution<std::size_t> elegir_rango(2, p);
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
