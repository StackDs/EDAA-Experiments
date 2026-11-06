#ifndef BENCH_EXPERIMENT_OPTIONS_HPP
#define BENCH_EXPERIMENT_OPTIONS_HPP

#include <map>
#include <sstream>
#include "text_data.hpp"

namespace bench {
struct Options {
    std::filesystem::path text = "texts/english.bin", patterns = "texts/patterns-official", output;
    std::string profile = "official", session_id;
    std::size_t size = 10000000, count = 5, runs = 32, target_ns = 5000000, max_batch = 1000000;
    std::size_t pause_seconds = 2;
    std::vector<std::size_t> sizes = {1000000,2000000,4000000,8000000,10000000};
    std::vector<std::size_t> lengths = {8,16,32,64,128,256};
};

inline std::vector<std::size_t> numbers(const std::string &value) {
    auto result = std::vector<std::size_t>{};
    for (const auto &item : fields(value)) result.push_back(number(item));
    if (result.empty() || !std::is_sorted(result.begin(), result.end()) || result.front() == 0 ||
        std::adjacent_find(result.begin(), result.end()) != result.end())
        throw std::runtime_error("La lista debe ser positiva creciente y sin duplicados");
    return result;
}

inline Options parse_options(int argc, char **argv) {
    Options o;
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) throw std::runtime_error("Falta el valor de una opcion");
        std::string key = argv[i], value = argv[i + 1];
        if (key == "--text") o.text = value;
        else if (key == "--patterns") o.patterns = value;
        else if (key == "--output") o.output = value;
        else if (key == "--session-id") o.session_id = value;
        else if (key == "--profile") o.profile = value;
        else if (key == "--size") o.size = number(value);
        else if (key == "--count") o.count = number(value);
        else if (key == "--runs") o.runs = number(value);
        else if (key == "--target-ns") o.target_ns = number(value);
        else if (key == "--max-batch") o.max_batch = number(value);
        else if (key == "--pause-seconds") o.pause_seconds = number(value);
        else if (key == "--sizes") o.sizes = numbers(value);
        else if (key == "--lengths") o.lengths = numbers(value);
        else throw std::runtime_error("Opcion desconocida " + key);
    }
    if (o.runs < 32) throw std::runtime_error("Se requieren al menos 32 repeticiones");
    if (o.profile != "official" ||
        o.runs > 100000 || o.count == 0 || o.count > 100 ||
        o.target_ns == 0 || o.max_batch == 0 || o.max_batch > 1000000 ||
        o.sizes.back() != o.size || o.lengths.back() > o.sizes.front() ||
        o.sizes.front() < 32)
        throw std::runtime_error("Configuracion fuera de rango");
    const Options defaults;
    if (o.profile == "official" && (o.size != defaults.size || o.count != defaults.count ||
        o.sizes != defaults.sizes || o.lengths != defaults.lengths))
        throw std::runtime_error("El protocolo debe coincidir con el definido en el informe");
    return o;
}

inline std::string preparation_config(const Options &o, const std::string &hash) {
    std::ostringstream out;
    out << "corpus_sha256=" << hash << "\nprofile=" << o.profile << "\nsize=" << o.size
        << "\ncount=" << o.count << "\nsizes=";
    for (auto n : o.sizes) out << n << ',';
    out << "\nlengths=";
    for (auto m : o.lengths) out << m << ',';
    return out.str() + "\n";
}

inline std::string read_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("No se pudo leer " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}
#endif
