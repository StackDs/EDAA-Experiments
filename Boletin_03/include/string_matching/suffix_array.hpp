// https://github.com/jfuentess/edaa/blob/main/strings/suffix_array.cpp

#ifndef SUFFIX_ARRAY
#define SUFFIX_ARRAY

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class suffix_array
{
private:
    // Conservamos una copia del texto y una vista para las comparaciones
    std::string _t;
    std::string_view t;
    // Guardamos las posiciones iniciales de los sufijos en orden
    std::vector<std::int64_t> SA;

public:
    suffix_array(const std::string &text)
    {
        // Agregamos un centinela menor que los bytes del texto admitido
        char ETX = 3;
        _t = text + ETX;
        t = _t;

        std::int64_t n, sigma, one, i, j, k;
        n = t.length();
        sigma = 256;
        one = 1;

        // Reservamos el indice y las frecuencias del ordenamiento por conteo
        SA.resize(n);
        std::vector<std::int64_t> count(std::max(sigma, n), 0);
        // Guardamos posiciones temporales y las clases nuevas y actuales
        std::vector<std::int64_t> p(n);
        std::vector<std::int64_t> q(n);
        std::vector<std::int64_t> r(n);

        // Ordenamos por el primer byte usando frecuencias e indices sin signo
        for (i = 0; i < n; i++)
            count[static_cast<unsigned char>(t[i])]++;
        for (i = 1; i < sigma; i++)
            count[i] += count[i - 1];
        for (i = n - 1; i >= 0; i--)
            SA[--count[static_cast<unsigned char>(t[i])]] = i;

        // Asignamos la misma clase a posiciones con el mismo byte
        r[SA[0]] = 0;
        j = 0;
        for (i = 1; i < n; i++) {
            if (t[SA[i - 1]] != t[SA[i]])
                j++;
            r[SA[i]] = j;
        }

        // Duplicamos el largo de los prefijos ordenados en cada ronda
        for (k = 0; (one << k) < n; k++) {
            // Preparamos las posiciones usando el orden de la ronda anterior
            for (i = 0; i < n; i++) {
                p[i] = SA[i] - (one << k);
                if (p[i] < 0)
                    p[i] += n;
            }

            // Ordenamos las posiciones por sus clases usando conteo
            for (i = 0; i <= j; i++)
                count[i] = 0;
            for (i = 0; i < n; i++)
                count[r[p[i]]]++;
            for (i = 1; i <= j; i++)
                count[i] += count[i - 1];
            for (i = n - 1; i >= 0; i--)
                SA[--count[r[p[i]]]] = p[i];

            // Comparamos pares de clases para formar las clases nuevas
            q[SA[0]] = 0;
            j = 0;
            for (i = 1; i < n; i++) {
                if (r[SA[i - 1]] != r[SA[i]] ||
                    r[(SA[i - 1] + (one << k)) % n] != r[(SA[i] + (one << k)) % n])
                    j++;

                q[SA[i]] = j;
            }

            // Conservamos las clases nuevas para la siguiente ronda
            for (i = 0; i < n; i++)
                r[i] = q[i];
        }
    }

    std::int64_t count(const std::string_view s)
    {
        if (s.length() > t.length())
            return 0;

        std::int64_t n, lo, mi, hi, matches;
        n = t.length();

        // Buscamos el primer sufijo que no sea menor que el patron
        lo = 0;
        hi = n;
        while (lo < hi) {
            mi = lo + (hi - lo) / 2;
            if (t.substr(SA[mi]) < s)
                lo = mi + 1;
            else
                hi = mi;
        }
        matches = lo;

        // Buscamos el final del intervalo que comienza con el patron
        hi = n;
        while (lo < hi) {
            mi = lo + (hi - lo) / 2;
            if (t.substr(SA[mi]).starts_with(s))
                lo = mi + 1;
            else
                hi = mi;
        }
        // Restamos los limites sin recorrer cada coincidencia
        matches = hi - matches;

        return matches;
    }

    // Permitimos consultar una posicion del arreglo de sufijos
    std::int64_t& operator[](std::int64_t i)
    {
        return SA[i];
    }
};

#endif
