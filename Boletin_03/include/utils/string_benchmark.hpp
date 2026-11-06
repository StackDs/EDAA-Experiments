#ifndef BENCH_STRING_BENCHMARK_HPP
#define BENCH_STRING_BENCHMARK_HPP

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "benchmark_output.hpp"
#include "index_storage.hpp"
#include "progress.hpp"
#include "string_matching/brute_force.hpp"
#include "string_matching/kmp.hpp"
#include "string_matching/rabin_karp.hpp"
#include "string_matching/suffix_array.hpp"

namespace bench {
inline void construct_index(const std::string &text, const std::string &dataset,
                            const Options &o, Output &output, IndexStorage &index,
                            bool measure_construction) {
    Context context{dataset, sha256(text), "", "suffix_array", "construction", text.size(), 0, 0};
    std::cout << dataset << " n=" << text.size()
              << " method=suffix_array | Preparacion y memoria del indice" << std::endl;
    // Medimos las reservas en una pasada distinta del tiempo
    memory::begin();
    try { index.build(text); } catch (...) { memory::cancel(); throw; }
    const auto usage = memory::finish();
    output.memory(context, usage);
    // Fuera del experimento de texto usamos esta unica construccion como preparacion
    if (!measure_construction) return;
    index.reset();
    // Calentamos una construccion antes de guardar muestras
    std::cout << "Calentamiento de construccion" << std::endl;
    index.build(text);
    index.reset();
    std::vector<Sample> samples;
    std::cout << "Construccion del Suffix Array | Repeticiones" << std::endl;
    display_progress(0, o.runs);
    for (std::size_t run = 0; run < o.runs; ++run) {
        const auto begin = Clock::now();
        index.build(text);
        asm volatile("" : : : "memory");
        const auto end = Clock::now();
        samples.push_back({1, elapsed_ns(begin, end)});
        if (run + 1 != o.runs) index.reset();
        display_progress(run + 1, o.runs);
    }
    output.times(context, samples);
}

template<class Call>
void measure_pattern(Call call, const Pattern &p, const Context &c, const Options &o, Output &output) {
    std::cout << "Patron=" << p.id << " m=" << p.bytes.size() << " z=" << p.z
              << " | Calentamiento y calibracion" << std::endl;
    const auto samples = measure_queries(call, p.z, o.runs, o.target_ns, o.max_batch, display_progress);
    output.times(c, samples);
    memory::begin();
    std::int64_t result;
    try { result = call(); } catch (...) { memory::cancel(); throw; }
    const auto usage = memory::finish();
    if (result < 0 || static_cast<std::size_t>(result) != p.z)
        throw std::runtime_error("Conteo incorrecto en la pasada de memoria");
    output.memory(c, usage);
}

inline void measure_text(const std::string &text, const std::string &dataset,
                         const std::vector<Pattern> &patterns, const Options &o,
                         Output &output, IndexStorage &index) {
    const auto hash = sha256(text);
    // Verificamos los controles antes de iniciar las mediciones
    for (const auto &p : patterns)
        if (reference_count(p.bytes, text) != p.z)
            throw std::runtime_error("Frecuencia incompatible con el protocolo " + p.id);
    for (const auto &method : {"brute_force", "kmp", "rabin_karp", "suffix_array"}) {
        std::cout << dataset << " n=" << text.size() << " method=" << method << std::endl;
        for (const auto &p : patterns) {
            Context c{dataset, hash, p.id, method, "query", text.size(), p.bytes.size(), p.z};
            // Elegimos el metodo antes del intervalo medido
            if (std::string_view(method) == "brute_force")
                measure_pattern([&] { return brute_force(p.bytes, text); }, p, c, o, output);
            else if (std::string_view(method) == "kmp")
                measure_pattern([&] { return kmp(p.bytes, text); }, p, c, o, output);
            else if (std::string_view(method) == "rabin_karp")
                measure_pattern([&] { return rk(p.bytes, text, 101); }, p, c, o, output);
            else measure_pattern([&] { return index.get().count(p.bytes); }, p, c, o, output);
        }
        if (o.pause_seconds) std::this_thread::sleep_for(std::chrono::seconds(o.pause_seconds));
    }
}

inline int run_experiment(int argc, char **argv, const std::string &experiment) {
    try {
        const auto o = parse_options(argc, argv);
        const auto corpus = load_text_prefix(o.text, o.size);
        const auto hash = sha256(corpus);
        if (o.profile == "official" && hash != corpus_hash)
            throw std::runtime_error("SHA256 del corpus oficial incorrecto");
        if (read_file(o.patterns / "config.txt") != preparation_config(o, hash))
            throw std::runtime_error("Configuracion de patrones incompatible");
        auto patterns = load_patterns(o.patterns / (experiment + ".csv"), hash);
        if (experiment == "text_size") {
            if (patterns.size() != o.count) throw std::runtime_error("Cantidad de patrones incorrecta");
            for (const auto &p : patterns)
                if (p.bytes.size() != 32 || p.z != 1) throw std::runtime_error("Control de n incorrecto");
        } else if (experiment == "pattern_size") {
            if (patterns.size() != o.count * o.lengths.size()) throw std::runtime_error("Cantidad de patrones incorrecta");
            for (auto m : o.lengths)
                if (std::count_if(patterns.begin(), patterns.end(), [m](const auto &p) {
                    return p.bytes.size() == m && p.z == 1;
                }) != static_cast<std::ptrdiff_t>(o.count)) throw std::runtime_error("Control de m incorrecto");
        } else {
            for (const auto &p : patterns)
                if (p.bytes.size() != 4) throw std::runtime_error("Control de ocurrencias incorrecto");
        }
        Output output(o, experiment, hash);
        IndexStorage index;
        if (experiment == "text_size") {
            for (auto n : o.sizes) {
                std::string text = corpus.substr(0, n);
                construct_index(text, "english", o, output, index, true);
                measure_text(text, "english", patterns, o, output, index);
                index.reset();
            }
        } else {
            construct_index(corpus, "english", o, output, index, false);
            measure_text(corpus, "english", patterns, o, output, index);
            index.reset();
        }
        if (experiment == "occurrences") {
            for (auto n : o.sizes) {
                std::string text(n, 'A');
                std::vector<Pattern> extremes = {{"A", "A", "unary_present", n, "0"},
                                                 {"B", "B", "unary_absent", 0, ""}};
                construct_index(text, "unary", o, output, index, false);
                measure_text(text, "unary", extremes, o, output, index);
                index.reset();
            }
        }
        output.complete();
        std::cout << "Experimento completo " << o.output << '\n';
        return 0;
    } catch (const std::exception &error) {
        memory::cancel();
        std::cerr << "\nError: " << error.what() << '\n';
        return 1;
    }
}
}
#endif
