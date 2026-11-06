// https://github.com/jfuentess/edaa/blob/main/strings/rk.cpp

#ifndef RK
#define RK

#include <iostream>
#include <cstdint>
#include <string>

int64_t rk(const std::string &pat, const std::string &txt, int q) {
    const int d = 256;
    int64_t count = 0;

    int txt_length = txt.length();
    int pat_length = pat.length();

    // Un patron mas largo que el texto no puede aparecer
    if (pat_length > txt_length) return 0;

    // Guardamos los hashes del patron y de la ventana actual
    int p = 0;
    int t = 0;
    int h = 1;

    // Preparamos el factor para quitar el primer byte de la ventana
    for (int i = 0; i < pat_length - 1; i++) {
        h = (h * d) % q;
    }

    // Calculamos los hashes iniciales usando bytes sin signo
    for (int i = 0; i < pat_length; i++) {
        p = (d * p + static_cast<unsigned char>(pat[i])) % q;
        t = (d * t + static_cast<unsigned char>(txt[i])) % q;
    }

    // Recorremos todas las ventanas incluyendo las que se solapan
    for (int i = 0; i <= txt_length - pat_length; i++) {
        if (p == t) {
            // Confirmamos los caracteres porque dos hashes pueden coincidir
            int j = 0;
            while (j < pat_length && txt[i + j] == pat[j]) {
                j++;
            }
            if (j == pat_length) {
                // Aumentamos el contador solo si coincide todo el patron
                count++;
            }
        }

        // Quitamos el byte que sale y agregamos el byte que entra
        if (i < txt_length - pat_length) {
            t = (d * (t - static_cast<unsigned char>(txt[i]) * h)
                 + static_cast<unsigned char>(txt[i + pat_length])) % q;
            if (t < 0) {
                t += q;
            }
        }
    }

    return count;
}

#endif
