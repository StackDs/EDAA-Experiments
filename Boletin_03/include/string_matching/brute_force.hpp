#ifndef BRUTE_FORCE
#define BRUTE_FORCE

#include <cstdint>
#include <string_view>

std::int64_t brute_force(const std::string_view pattern,
                         const std::string_view text) {
  std::int64_t matches = 0;

  // Si el patron es mas largo que el texto no puede aparecer
  if (pattern.size() > text.size())
    return matches;

  // Probamos cada posicion donde cabe el patron
  for (std::size_t i = 0; i <= text.size() - pattern.size(); i++) {
    // Comparamos los caracteres hasta encontrar una diferencia
    std::size_t j = 0;
    while (j < pattern.size() && text[i + j] == pattern[j])
      j++;

    // Contamos la coincidencia completa y avanzamos solo una posicion
    if (j == pattern.size())
      matches++;
  }

  return matches;
}

#endif
