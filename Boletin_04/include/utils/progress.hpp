#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace bench {
// Adaptado de la barra uhr de los boletines 02 y 03
// https://github.com/leonardlover
class Progress {
    using Clock = std::chrono::steady_clock;
    std::size_t actual_ = 0, total_, previous_width_ = 0;
    std::string label_, context_;
    Clock::time_point start_ = Clock::now(), last_ = start_;
    bool terminal_ = isatty(STDOUT_FILENO), open_line_ = false;

public:
    Progress(std::size_t total, const std::string &label) : total_(total), label_(label) {
        show("Inicio", true);
    }
    ~Progress() { if (open_line_) std::cout << '\n' << std::flush; }
    void context(const std::string &text) { context_ = text; }
    void show(const std::string &detail, bool force = false) {
        const auto now = Clock::now();
        // Limita la salida fuera de los intervalos medidos
        if (!force && actual_ != total_ && now - last_ < std::chrono::milliseconds(terminal_ ? 200 : 1000)) return;
        last_ = now;
        constexpr std::size_t width = 30;
        const auto filled = total_ ? width * actual_ / total_ : width;
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start_).count();
        std::ostringstream line;
        line << '[' << std::string(filled, '=') << std::string(width - filled, ' ')
             << "] " << (total_ ? 100 * actual_ / total_ : 100) << "% " << actual_ << '/' << total_
             << " | " << seconds / 60 << "m " << seconds % 60 << "s | " << label_;
        if (!context_.empty()) line << " | " << context_;
        line << " | " << detail;
        const auto text = line.str();
        if (terminal_) std::cout << '\r';
        std::cout << text;
        if (terminal_ && previous_width_ > text.size()) std::cout << std::string(previous_width_ - text.size(), ' ');
        previous_width_ = text.size();
        open_line_ = terminal_ && actual_ != total_;
        if (!open_line_) std::cout << '\n';
        std::cout << std::flush;
    }
    void advance(const std::string &detail) {
        actual_ = std::min(actual_ + 1, total_);
        show(detail);
    }
};
}
