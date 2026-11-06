#pragma once

// Adaptado de los generadores de los boletines 01 y 02

#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace bench {
using Value = std::int64_t;
using Entry = std::pair<Value, std::uint64_t>;
using Array = std::vector<Entry>;
struct Query { std::uint64_t left, right; };
struct Update { std::uint64_t index; Value value; };
struct Range { std::string name; std::size_t length; };
constexpr Value max_value = 1000000000;

// El rechazo evita sesgo y dependencias del mapeo de la biblioteca
inline std::uint64_t uniform(std::mt19937_64 &rng, std::uint64_t upper) {
    const auto span = upper + 1;
    if (!span) return rng();
    const auto threshold = -span % span;
    std::uint64_t value;
    do { value = rng(); } while (value < threshold);
    return value % span;
}

inline std::uint64_t derive_seed(std::uint64_t seed, std::uint64_t tag) {
    auto x = seed + 0x9e3779b97f4a7c15ULL * (tag + 1);
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

inline Array generar_arreglo(std::size_t n, std::uint64_t seed) {
    if (!n) throw std::invalid_argument("El arreglo no puede estar vacio");
    std::mt19937_64 rng(seed);
    Array values;
    values.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        values.emplace_back(static_cast<Value>(uniform(rng, max_value)), i);
    return values;
}

// El extremo derecho se guarda excluido para llamar al codigo del curso
inline std::vector<Query> generar_consultas(std::size_t n, std::size_t k,
                                           std::size_t count, std::uint64_t seed) {
    if (!n || !k || k > n || !count) throw std::invalid_argument("Rangos invalidos");
    std::mt19937_64 rng(seed);
    std::vector<Query> queries;
    queries.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto left = uniform(rng, n - k);
        queries.push_back({left, left + k});
    }
    return queries;
}

inline std::vector<Update> generar_actualizaciones(const Array &initial,
                                                  std::size_t count, std::uint64_t seed) {
    if (initial.empty() || !count) throw std::invalid_argument("Actualizaciones invalidas");
    auto state = initial;
    std::mt19937_64 rng(seed);
    std::vector<Update> updates;
    updates.reserve(count);
    for (std::size_t j = 0; j < count; ++j) {
        const auto index = uniform(rng, state.size() - 1);
        Value value;
        do { value = static_cast<Value>(uniform(rng, max_value)); }
        while (value == state[index].first);
        state[index] = {value, index};
        updates.push_back({index, value});
    }
    return updates;
}

inline std::vector<Range> rangos(std::size_t n) {
    if (!n) throw std::invalid_argument("Tamano invalido");
    std::vector<Range> result;
    for (const auto &range : std::vector<Range>{{"1", 1}, {"8", 8}, {"63", 63},
                                               {"511", 511}, {"half", std::max(std::size_t{1}, n / 2)},
                                               {"full", n}}) {
        if (range.length > n) continue;
        if (std::none_of(result.begin(), result.end(), [&](const auto &r) {
                return r.length == range.length;
            })) result.push_back(range);
    }
    return result;
}

inline Entry reference_rmq(const Array &values, Query query) {
    if (query.left >= query.right || query.right > values.size())
        throw std::invalid_argument("Consulta de referencia invalida");
    auto result = values[query.left];
    for (auto i = query.left + 1; i < query.right; ++i)
        if (values[i].first < result.first ||
            (values[i].first == result.first && values[i].second < result.second)) result = values[i];
    return result;
}
}
