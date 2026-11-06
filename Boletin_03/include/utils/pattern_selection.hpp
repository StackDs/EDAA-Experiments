#ifndef BENCH_PATTERN_SELECTION_HPP
#define BENCH_PATTERN_SELECTION_HPP

#include <array>
#include <cmath>
#include <map>
#include <unordered_map>
#include "text_data.hpp"

namespace bench {
inline std::vector<Pattern> select_unique(const std::string &text, std::size_t m,
                                         std::size_t count, std::size_t limit) {
    std::vector<Pattern> result;
    std::set<std::string> seen;
    limit = std::min(limit, text.size());
    // Elegimos patrones sin modificar el texto ni sus frecuencias
    for (std::size_t i = 0; i + m <= limit && result.size() < count; ++i) {
        auto pattern = text.substr(i, m);
        if (!seen.insert(pattern).second) continue;
        const auto first = text.find(pattern);
        if (first == i && text.find(pattern, first + 1) == std::string::npos)
            result.push_back({"u" + std::to_string(m) + "_" + std::to_string(result.size()),
                              pattern, "unique", 1, std::to_string(i)});
    }
    if (result.size() != count) throw std::runtime_error("No se encontraron suficientes patrones unicos");
    return result;
}

inline std::uint32_t key4(const std::string &text, std::size_t offset) {
    std::uint32_t key = 0;
    for (int j = 0; j < 4; ++j) key = (key << 8) | static_cast<unsigned char>(text[offset + j]);
    return key;
}

inline std::string bytes4(std::uint32_t key) {
    std::string bytes(4, ' ');
    for (int j = 3; j >= 0; --j) {
        bytes[j] = static_cast<char>(key & 255);
        key >>= 8;
    }
    return bytes;
}

inline std::vector<Pattern> select_occurrences(const std::string &text, std::size_t count) {
    struct Frequency { std::size_t z = 0, offset = 0; };
    std::unordered_map<std::uint32_t, Frequency> frequencies;
    for (std::size_t i = 0; i + 4 <= text.size(); ++i) {
        auto &entry = frequencies[key4(text, i)];
        if (entry.z++ == 0) entry.offset = i;
    }
    if (frequencies.empty()) throw std::runtime_error("Texto insuficiente para patrones de cuatro bytes");
    std::vector<std::uint32_t> keys;
    std::size_t low = text.size(), high = 0;
    for (const auto &[key, f] : frequencies) {
        keys.push_back(key);
        low = std::min(low, f.z);
        high = std::max(high, f.z);
    }
    std::sort(keys.begin(), keys.end());
    std::array<std::size_t, 3> selected{};
    std::vector<Pattern> result;
    for (auto key : keys) {
        const auto f = frequencies.at(key);
        const auto group = high == low ? 0 : std::min(2, static_cast<int>(3 *
            (std::log(double(f.z)) - std::log(double(low))) /
            (std::log(double(high)) - std::log(double(low)))));
        if (selected[group] >= count) continue;
        result.push_back({"f" + std::to_string(key), bytes4(key), "frequency" + std::to_string(group),
                          f.z, std::to_string(f.offset)});
        ++selected[group];
    }
    // Construimos consultas ausentes con el mismo alfabeto observado
    std::set<unsigned char> alphabet(text.begin(), text.end());
    std::size_t absent = 0;
    for (auto a : alphabet) for (auto b : alphabet) for (auto c : alphabet) for (auto d : alphabet) {
        if (absent == count) goto complete;
        const auto key = (std::uint32_t(a) << 24) | (std::uint32_t(b) << 16) |
                         (std::uint32_t(c) << 8) | d;
        if (!frequencies.contains(key)) {
            result.push_back({"a" + std::to_string(absent++), bytes4(key), "absent", 0, ""});
        }
    }
complete:
    if (absent != count) throw std::runtime_error("No se encontraron suficientes patrones ausentes");
    for (const auto &p : result)
        if (reference_count(p.bytes, text) != p.z)
            throw std::runtime_error("Fallo al verificar la frecuencia del patron");
    return result;
}

inline void write_patterns(const std::filesystem::path &path, const std::vector<Pattern> &patterns,
                           const std::string &hash) {
    std::ofstream output(path);
    output << "pattern_id,hex,m,z,group,offset,corpus_sha256\n";
    for (const auto &p : patterns)
        output << p.id << ',' << hex(p.bytes) << ',' << p.bytes.size() << ',' << p.z
               << ',' << p.group << ',' << p.offset << ',' << hash << '\n';
    if (!output) throw std::runtime_error("No se pudo guardar el manifiesto");
}
}
#endif
