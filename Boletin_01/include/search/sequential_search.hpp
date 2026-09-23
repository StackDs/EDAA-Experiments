#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

/*  ###################################################
    ###################################################
    ######                                       ######
    ######  Busqueda Secuencial (Linear Search)  ######
    ######                                       ######
    ###################################################
    ###################################################
    */

template <std::integral key = std::int64_t>

//Implementacion propia
std::int64_t sequential_search_custom(const std::vector<key>& vect, const key& target){
    
    std::size_t size = vect.size();

    //Buscamos coincidencia con nuestro target
    for(std::size_t i = 0; i < size; i++){

        // Devolvemos la posicion casteado a un entero de 64 bits
        if(vect[i] == target) {
            return static_cast<std::int64_t>(i);
        }
    }
    return -1;
}


template <std::integral key = std::int64_t>

//Implementacion haciendo uso de STL
std::int64_t sequential_search_STL(const std::vector<key>& vect, const key& target){
    //Obtenemos la referencia al elemento buscado mediante un iterador, haciendo uso de 'find'
    auto iterator = std::find(vect.begin(), vect.end(), target);
    //En caso de no haber alcanzado el final, significa que encontramos target
    if(iterator != vect.end()){
        //Devolvemos la distancia desde el inicio hasta la posicion del target
        return static_cast<std::int64_t>(std::distance(vect.begin(), iterator));
    }
    return -1;
}
