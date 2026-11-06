#ifndef BRUTE_FORCE
#define BRUTE_FORCE

#include <cstdint>
#include <string_view>

std::int64_t brute_force(const std::string_view pattern,
                         const std::string_view text) {
  std::int64_t matches = 0;

  // S m > n devolvemos 0
  if (pattern.size() > text.size())
    return matches;

  // Recorremos el texto y comparamos con el patrón
  for (std::size_t i = 0; i <= text.size() - pattern.size(); i++) {
    std::size_t j = 0;
    while (j < pattern.size() && text[i + j] == pattern[j])
      j++;

    if (j == pattern.size())
      matches++;
  }

  return matches;
}

#endif
