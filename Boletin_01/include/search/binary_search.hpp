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
    ######   Busqueda Binaria (Binary Search)    ######
    ######                                       ######
    ###################################################
    ###################################################
    */

template <std::integral key = std::int64_t>

// Implementacion propia
std::int64_t binary_search_custom(const std::vector<key>& vect, const key& target){
    if(vect.empty()) return -1;

    //Iniciamos los extremos, casteamos al tipo de retorno la variable right
    std::int64_t left = 0;
    std::int64_t right = static_cast<std::int64_t>(vect.size() - 1);

    while(left <= right){

        //Calculamos 'mid' mediante una resta para evitar overflow, int64_t redondea hacia abajo
        std::int64_t mid = left + (right - left)/2;

        //Retornamos o actualizamos left y right segun corresponda
        if(vect[mid] == target){
            return mid;
        }
        else if (vect[mid] < target){
            left = mid + 1;
        }
        else{
            right = mid - 1;
        }
    }

    return -1;
}


template <std::integral key = std::int64_t>

// Implementacion STL
/* 
NOTA: No usamos directamente el binary_search de la stl debido a que esa funcion
se desliga de la posicion, solo devuelve True O False, referenciando si encontro el
elemento o no, por debajo binary search usa lower bound para funcionar, como para los 
experimentos de mas adelante necesitaremos la posicion, la implementamos de esta forma
para asegurar guardar la posicion y a la vez la mejor complejidad
*/

std::int64_t binary_search_STL(const std::vector<key>& vect, const key& target){

    /* La funcion lower bound devuelve un iterador al primer elemento en un 
    rango ordenado que es mayor o igual al target, por debajo usa busqueda binaria 
    lo que asegura O(log n) */

    auto iterador = std::lower_bound(vect.begin(), vect.end(), target);

    //Si el iterador no es el final y su valor asociado es igual al target, devolvemos la posicion
    if(iterador != vect.end() && *iterador == target){
        return static_cast<std::int64_t>(std::distance(vect.begin(), iterador));
    }

    return -1;
}
