// https://github.com/jfuentess/edaa/blob/main/strings/rk.cpp

#ifndef RK
#define RK

#include <iostream>
#include <string>

int64_t rk(const std::string &pat, const std::string &txt, int q) {
    const int d = 256;
    int64_t count = 0;

    int txt_length = txt.length();
    int pat_length = pat.length();

    if (pat_length > txt_length) return 0;

    int p = 0;
    int t = 0;
    int h = 1;

    for (int i = 0; i < pat_length - 1; i++) {
        h = (h * d) % q;
    }

    for (int i = 0; i < pat_length; i++) {
        p = (d * p + pat[i]) % q;
        t = (d * t + txt[i]) % q;
    }

    for (int i = 0; i <= txt_length - pat_length; i++) {
        if (p == t) {
            int j = 0;
            while (j < pat_length && txt[i + j] == pat[j]) {
                j++;
            }
            if (j == pat_length) {
                count++;
            }
        }

        if (i < txt_length - pat_length) {
            t = (d * (t - txt[i] * h) + txt[i + pat_length]) % q;
            if (t < 0) {
                t += q;
            }
        }
    }

    return count;
}

#endif
