#ifndef BENCH_TEXT_DATA_HPP
#define BENCH_TEXT_DATA_HPP

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "sha256.hpp"

namespace bench {
inline constexpr const char *corpus_hash =
    "ab426bd5952843c190cc9705f94dfe42a1fbbc53dcb06f480ebd21facea6d9f7";

inline std::size_t number(std::string_view value) {
    std::size_t result = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc() || end != value.data() + value.size() || value.empty())
        throw std::runtime_error("Numero invalido");
    return result;
}

inline void validate_bytes(std::string_view text) {
    if (std::any_of(text.begin(), text.end(), [](unsigned char c) { return c <= 3; }))
        throw std::runtime_error("Los bytes de entrada deben ser mayores que el centinela ETX");
}

inline std::string load_text_prefix(const std::filesystem::path &path, std::size_t n) {
    if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::runtime_error("Texto fuera del rango admitido por Rabin Karp");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("No se pudo abrir el texto " + path.string());
    std::string text(n, '\0');
    // Cargamos exactamente los bytes solicitados fuera del reloj
    if (!input.read(text.data(), static_cast<std::streamsize>(n)))
        throw std::runtime_error("El texto tiene menos bytes que los solicitados");
    validate_bytes(text);
    return text;
}

inline std::string hex(std::string_view bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (unsigned char byte : bytes) {
        result += digits[byte >> 4];
        result += digits[byte & 15];
    }
    return result;
}

inline std::string unhex(std::string_view encoded) {
    if (encoded.empty() || encoded.size() % 2) throw std::runtime_error("Patron hexadecimal invalido");
    std::string result;
    for (std::size_t i = 0; i < encoded.size(); i += 2) {
        unsigned value = 0;
        auto [end, error] = std::from_chars(encoded.data() + i, encoded.data() + i + 2, value, 16);
        if (error != std::errc() || end != encoded.data() + i + 2)
            throw std::runtime_error("Patron hexadecimal invalido");
        result += static_cast<char>(value);
    }
    validate_bytes(result);
    return result;
}

struct Pattern {
    std::string id, bytes, group;
    std::size_t z = 0;
    std::string offset;
};

inline std::vector<std::string> fields(const std::string &line) {
    std::vector<std::string> result;
    std::size_t begin = 0;
    do {
        auto end = line.find(',', begin);
        result.push_back(line.substr(begin, end == std::string::npos ? end : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1;
    } while (true);
    return result;
}

inline std::vector<Pattern> load_patterns(const std::filesystem::path &path,
                                        const std::string &expected_hash) {
    std::ifstream input(path);
    std::string line;
    if (!std::getline(input, line) || line != "pattern_id,hex,m,z,group,offset,corpus_sha256")
        throw std::runtime_error("Cabecera de patrones invalida " + path.string());
    std::vector<Pattern> patterns;
    std::set<std::string> ids, contents;
    while (std::getline(input, line)) {
        auto row = fields(line);
        if (row.size() != 7 || row[6] != expected_hash || row[0].empty())
            throw std::runtime_error("Manifiesto o hash incompatible");
        auto bytes = unhex(row[1]);
        if (bytes.size() != number(row[2]) || !ids.insert(row[0]).second ||
            !contents.insert(bytes).second)
            throw std::runtime_error("Patrones duplicados o longitud incorrecta");
        if (!row[5].empty()) number(row[5]);
        patterns.push_back({row[0], bytes, row[4], number(row[3]), row[5]});
    }
    if (patterns.empty()) throw std::runtime_error("No hay patrones en el manifiesto");
    return patterns;
}

inline std::size_t reference_count(const std::string &pattern, const std::string &text) {
    if (pattern.empty()) throw std::runtime_error("El patron no puede estar vacio");
    std::size_t count = 0;
    for (auto pos = text.find(pattern); pos != std::string::npos; pos = text.find(pattern, pos + 1))
        ++count;
    return count;
}
}
#endif
