// Fuente https://github.com/jfuentess/edaa/blob/550e2199f82e74e38d253eba2e64f8c72af38296/rmq/sparse_table.cpp
// Autor original LELE
// Tabla dispersa generica

#ifndef SPARSE_TABLE
#define SPARSE_TABLE

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
#include <vector>

template<class T>
class sparse_table
{
    private:
        std::vector<std::vector<T> > ST;
        std::function<T(T, T)> f;

        // Calcula el logaritmo entero en base dos
        inline uint64_t _lg(uint64_t n)
        {
            if (n == 0) {
                std::cerr << "lg(0) is undefined." << std::endl;
                std::exit(EXIT_FAILURE);
            }

            return std::bit_width(n) - 1;
        }

    public:
        sparse_table(const std::vector<T> &A, std::function<T(T, T)> F)
        {
            // Rechaza entradas vacias
            if (A.size() == 0) {
                std::cerr << "Invalid input array: must be non-empty." << std::endl;
                std::exit(EXIT_FAILURE);
            }

            uint64_t i, j, m, n, p, q, one;
            n = A.size();
            m = _lg(n);
            one = 1;

            // Reserva los niveles y guarda la funcion de combinacion
            ST.resize(m + 1, std::vector<T>(n));
            f = F;

            // Guarda los bloques de un elemento
            for (j = 0; j < n; j++)
                ST[0][j] = A[j];

            // Construye bloques de longitud potencia de dos
            for (i = 0; i < m; i++) {
                p = one << i;
                q = p << 1;
                for (j = 0; j <= n - q; j++)
                    ST[i + 1][j] = f(ST[i][j], ST[i][j + p]);
            }
        }

        // Consulta desde i incluido hasta j excluido
        T query(uint64_t i, uint64_t j)
        {
            if (i >= j || j > ST[0].size()) {
                std::cerr << "Invalid query " << i << " " << j << std::endl;
                std::exit(EXIT_FAILURE);
            }

            uint64_t one, b;
            one = 1;

            b = _lg(j - i);
            T q = ST[b][i];
            i += (one << b);

            // Combina bloques disjuntos de izquierda a derecha
            while (i < j) {
                b = _lg(j - i);
                q = f(q, ST[b][i]);
                i += (one << b);
            }

            return q;
        }

        // Consulta con una operacion asociativa e idempotente
        T query_idempotent(uint64_t i, uint64_t j)
        {
            if (i >= j || j > ST[0].size()) {
                std::cerr << "Invalid query " << i << " " << j << std::endl;
                std::exit(EXIT_FAILURE);
            }

            uint64_t one, b;
            one = 1;
            b = _lg(j - i);

            // Combina dos bloques que pueden solaparse
            return f(ST[b][i], ST[b][j - (one << b)]);
        }
};

#endif
