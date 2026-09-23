#pragma once

#include <concepts> // para std::integral key, asi evitamos el uso constexpr para validar tipos numericos
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>


//Declaramos tipo generico 'key' necesariamente numerico, entero por default
template <std::integral key = std::int64_t>

//Creamos un vector creciente con elementos unicos mediante Random Cumulative Steps
std:: vector<key> generar_vector(std::size_t size, std::uint64_t seed = 123456789ULL, key step_min = 1, key step_max = 5){

    if (step_min < 1){
        throw std::invalid_argument("El salto minimo debe ser al menos 1");
    }

    if (step_min > step_max){
        throw std::invalid_argument("Step min no puede ser mayor que Step Max");
    }

    std::vector<key> data;

    // Reservamos la memoria necesaria para no realocar memoria mientras el vector crece
    data.reserve(size);

    if( size == 0) return data;


    // Creamos un generador pseudoaleatorio con la semilla proporcionada usando el algoritmo de Mersenne Twister para generar numeros de 64 bits
    std:: mt19937_64 gen(seed);
    // Distribucion uniforme de enteros para los saltos
    std::uniform_int_distribution<key> distrib(step_min, step_max);

    // Primer elemento
    key acumulador = distrib(gen);
    data.push_back(acumulador);

    // Generacion del resto del vector en tiempo lineal O(n)
    for(std::size_t i = 1; i < size; i++){
        acumulador += distrib(gen);
        data.push_back(acumulador);
    }

    return data;
}