#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

/*  #####################################################
    #####################################################
    ######                                         ######
    ######   Busqueda galopante (galloping search) ######
    ######                                         ######
    #####################################################
    #####################################################
*/

template <std::integral key = std::int64_t>
std::int64_t galloping_search_custom(const std::vector<key>& vect, const key& target){

    // Comprobamos los casos al borde
    if (vect.empty()) return -1;
    if (vect[0] == target) return 0;
    if (vect[0] > target) return -1;

    // Guardamos el size
    const std::int64_t n = static_cast<std::int64_t>(vect.size());

    // Definimos el salto
    std::int64_t left = 1;
    
    // Mientras este en el rango y sea estrictamente menor, duplicamos
    while (left < n && vect[left] < target) {
        left *= 2;
    }
    
    // Para considera si el pivote se escapo del rango asignamos right como el min entre el pivote y el size del vector
    std::int64_t right = std::min(left, n - 1);
    left /= 2;
    
    // Busqueda binaria sobre el intervalo resultante
    while (left <= right) {
        std::int64_t mid = left + (right - left) / 2;
        if (vect[mid] == target) {
            return mid;
        } else if (vect[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

// Implementacion STL

// Busqueda de intervalo analoga a la anterior

template <std::integral key = std::int64_t>
std::int64_t galloping_search_STL(const std::vector<key>& vect, const key& target){

    if(vect.empty()) return -1;

    if(vect[0] == target) return 0;

    if(vect[0] > target) return -1;

    // Guardamos el size
    const std::int64_t n = static_cast<std::int64_t>(vect.size());
 
    std::int64_t left = 1;
    
    while(left < n && vect[left] < target){
        left *= 2;
    }
    
    std::int64_t right = std::min(left, n - 1);
    left /= 2;

    // Creamos variables automaticas con el intervalo sobre el cual hacer lower_bound
    auto first = vect.begin() + left;
    auto last = vect.begin() + right + 1;

    auto it = std::lower_bound(first, last, target);

    if (it != last && *it == target) {
        return static_cast<std::int64_t>(std::distance(vect.begin(), it));
    }
    return -1;
}
