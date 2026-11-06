#ifndef BENCH_PROGRESS_HPP
#define BENCH_PROGRESS_HPP

#include <cstddef>
#include <iostream>

namespace bench {
// Conservamos la barra de la plantilla uhr usada en el Boletin 02
// https://github.com/leonardlover
inline void display_progress(std::size_t actual, std::size_t total) {
    constexpr std::size_t width = 60;
    const auto filled = width * actual / total;
    std::cout << '\r' << '[';
    for (std::size_t i = 0; i < width; ++i)
        std::cout << (i < filled ? '=' : ' ');
    std::cout << "] " << 100 * actual / total << "%  " << actual << '/' << total;
    if (actual == total) std::cout << '\n';
    std::cout << std::flush;
}
}
#endif
