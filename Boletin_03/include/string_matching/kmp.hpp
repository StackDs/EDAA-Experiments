// https://github.com/jfuentess/edaa/blob/main/strings/kmp.cpp

#ifndef KMP
#define KMP

#include <string>
#include <vector>

int64_t kmp(const std::string_view s, const std::string_view t)
{
    int64_t m, n, matches, i, j;
    m = s.length();
    n = t.length();
    matches = 0;

    if (m > n)
        return 0;

    std::vector<int64_t> p(m + 1);
    p[0] = -1;

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

    p[m] = j;

    i = 0;
    j = 0;
    while (i < n) {
        if (t[i] == s[j]) {
            i++;
            j++;
            if (j == m) {
                matches++;
                j = p[j];
            }
        } else {
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
