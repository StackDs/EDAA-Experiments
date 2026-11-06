// Fuente https://github.com/jfuentess/edaa/blob/550e2199f82e74e38d253eba2e64f8c72af38296/rmq/segment_tree.cpp
// Autor original LELE
// Arbol de segmentos para consultas de minimo

#ifndef SEGMENT_TREE
#define SEGMENT_TREE

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <bit>

template<class T>
class segment_tree
{
    private:
        std::vector<T> ST;

        // Selecciona el menor de dos elementos
        inline T _min(const T &x, const T &y)
        {
            return x < y ? x : y;
        }

    public:
        segment_tree(const std::vector<T> &A)
        {
            // Rechaza entradas vacias
            if (A.size() == 0) {
                std::cerr << "Invalid input array: must be non-empty." << std::endl;
                std::exit(EXIT_FAILURE);
            }

            uint64_t n, i, j;
            n = A.size();

            // Reserva espacio para hojas y nodos internos
            ST.resize(n << 1);

            // Copia los elementos en las hojas
            for (i = 0; i < n; i++)
                ST[i + n] = A[i];

            // Construye los minimos desde las hojas hasta la raiz
            for (i = n - 1; i > 0; i--) {
                j = i << 1;
                ST[i] = _min(ST[j], ST[j | 1]);
            }
        }

        // Consulta desde i incluido hasta j excluido
        T query(uint64_t i, uint64_t j)
        {
            uint64_t n = ST.size() >> 1;

            if (i >= j || j > n) {
                std::cerr << "Invalid query " << i << " " << j << std::endl;
                std::exit(EXIT_FAILURE);
            }

            // Inicia el minimo con un elemento del intervalo
            T q = ST[i + n];

            // Combina los nodos que cubren el intervalo
            for (i += n, j += n; i < j; i >>= 1, j >>= 1) {
                if (i & 1)
                    q = _min(q, ST[i++]);
                if (j & 1)
                    q = _min(q, ST[--j]);
            }

            return q;
        }

        // Cambia el valor del elemento en la posicion i
        void update(uint64_t i, const T x)
        {
            uint64_t n = ST.size() >> 1;

            if (i >= n) {
                std::cerr << "Invalid update " << i << std::endl;
                std::exit(EXIT_FAILURE);
            }

            // Actualiza la hoja y recalcula sus ancestros
            for (ST[i += n] = x; i > 1; i >>= 1)
                ST[i >> 1] = _min(ST[i], ST[i ^ 1]);
        }
};

#endif
