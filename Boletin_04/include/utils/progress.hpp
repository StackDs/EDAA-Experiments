#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>

namespace bench {
// Adaptado de la barra uhr de los boletines 02 y 03
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

class Progress {
    using Clock = std::chrono::steady_clock;
    std::size_t actual_ = 0, total_;
    std::string label_, context_;
    Clock::time_point start_ = Clock::now(), last_ = start_;
    bool open_line_ = false;

public:
    Progress(std::size_t total, const std::string &label) : total_(total), label_(label) {
        show("Inicio", true);
    }
    ~Progress() { if (open_line_) std::cout << '\n' << std::flush; }
    void context(const std::string &text) { context_ = text; }
    void show(const std::string &detail, bool force = false) {
        const auto now = Clock::now();
        // Limita la salida fuera de los intervalos medidos
        if (!force && actual_ != total_ && now - last_ < std::chrono::milliseconds(200)) return;
        last_ = now;
        if (force) {
            if (open_line_) std::cout << '\n';
            std::cout << label_;
            if (!context_.empty()) std::cout << " | " << context_;
            std::cout << " | " << detail << '\n';
        }
        display_progress(actual_, total_);
        open_line_ = actual_ != total_;
        if (!open_line_) {
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start_).count();
            std::cout << label_ << " | " << detail << " | " << seconds / 60 << "m " << seconds % 60 << "s\n" << std::flush;
        }
    }
    void advance(const std::string &detail) {
        actual_ = std::min(actual_ + 1, total_);
        show(detail);
    }
};
}
