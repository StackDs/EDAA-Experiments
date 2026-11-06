// https://github.com/jfuentess/edaa/blob/main/strings/kmp.cpp

#ifndef KMP
#define KMP

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

int64_t kmp(const std::string_view s, const std::string_view t)
{
    // Guardamos los largos del patron y del texto y preparamos el contador
    int64_t m, n, matches, i, j;
    m = s.length();
    n = t.length();
    matches = 0;

    // Un patron mas largo que el texto no tiene coincidencias
    if (m > n)
        return 0;

    // La tabla guarda como retomar el patron despues de una diferencia
    std::vector<int64_t> p(m + 1);
    p[0] = -1;

    // Construimos la tabla aprovechando los prefijos que se repiten
    j = 0;
    for (i = 1; i < m; i++) {
        if (s[i] == s[j]) {
            p[i] = p[j];
        } else {
            p[i] = j;
            while (j >= 0) {
                if (s[i] == s[j])
                    break;

                j = p[j];
            }
        }

        j++;
    }

    // Guardamos como continuar despues de una coincidencia completa
    p[m] = j;

    // Recorremos el texto sin retroceder su indice
    i = 0;
    j = 0;
    while (i < n) {
        if (t[i] == s[j]) {
            i++;
            j++;
            if (j == m) {
                // Contamos y conservamos el prefijo que permite solapamientos
                matches++;
                j = p[j];
            }
        } else {
            // Retomamos otra posicion del patron usando la tabla
            j = p[j];
            if (j < 0) {
                i++;
                j++;
            }
        }
    }

    return matches;
}

#endif
