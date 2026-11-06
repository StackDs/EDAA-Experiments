#pragma once

#include "rmq_inputs.hpp"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace bench {
struct Options {
    std::string profile = "official";
    std::string session_id, config_id;
    std::filesystem::path output;
    std::vector<std::size_t> sizes{1000, 4000, 16000, 64000, 256000, 1000000};
    std::vector<std::uint64_t> seeds{1729, 2718, 31415};
    std::size_t runs = 32, cycles = 10, dynamic_queries = 1000, static_queries = 10000;
    std::size_t target_ns = 5000000, max_batch = 1000000;
};

inline std::uint64_t positive(const std::string &text) {
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !value)
        throw std::invalid_argument("Entero positivo invalido: " + text);
    return value;
}

inline std::vector<std::uint64_t> numbers(const std::string &text) {
    std::vector<std::uint64_t> result;
    std::size_t start = 0;
    do {
        const auto end = text.find(',', start);
        const auto value = positive(text.substr(start, end - start));
        for (auto old : result) if (old == value) throw std::invalid_argument("Valor duplicado");
        result.push_back(value);
        if (end == std::string::npos) break;
        start = end + 1;
    } while (true);
    return result;
}

inline Options parse_options(int argc, char **argv) {
    Options o;
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 == argc) throw std::invalid_argument("Falta el valor de una opcion");
        const std::string key = argv[i], value = argv[i + 1];
        if (key == "--profile") o.profile = value;
        else if (key == "--session-id") o.session_id = value;
        else if (key == "--config-id") o.config_id = value;
        else if (key == "--output") o.output = value;
        else if (key == "--sizes") {
            const auto values = numbers(value);
            o.sizes.assign(values.begin(), values.end());
        } else if (key == "--seeds") o.seeds = numbers(value);
        else if (key == "--runs") o.runs = positive(value);
        else if (key == "--cycles") o.cycles = positive(value);
        else if (key == "--dynamic-queries") o.dynamic_queries = positive(value);
        else if (key == "--static-queries") o.static_queries = positive(value);
        else if (key == "--target-ns") o.target_ns = positive(value);
        else if (key == "--max-batch") o.max_batch = positive(value);
        else throw std::invalid_argument("Opcion desconocida: " + key);
    }
    if (o.profile != "official" && o.profile != "pilot" && o.profile != "smoke")
        throw std::invalid_argument("Perfil invalido");
    if (o.output.empty() || !std::filesystem::is_directory(o.output))
        throw std::invalid_argument("La salida debe ser un directorio creado por el lanzador");
    for (const auto &id : {o.session_id, o.config_id})
        if (id.empty() || id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") != std::string::npos)
            throw std::invalid_argument("Identificador de sesion o configuracion invalido");
    if ((o.profile == "official" && o.runs < 32) || o.runs < 4)
        throw std::invalid_argument("Se requieren 32 repeticiones oficiales o cuatro de comprobacion");
    if (o.static_queries > o.max_batch || o.max_batch > 1000000 || o.cycles > 100 || o.dynamic_queries > 10000)
        throw std::invalid_argument("Carga fuera de los limites de seguridad");
    for (auto n : o.sizes) {
        if (n > 1000000 || o.dynamic_queries < rangos(n).size())
            throw std::invalid_argument("Tamano o cantidad de consultas no admitido");
    }
    return o;
}
}
