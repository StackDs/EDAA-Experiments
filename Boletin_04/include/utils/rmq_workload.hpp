#pragma once

#include "experiment_options.hpp"
#include "progress.hpp"
#include "rmq_inputs.hpp"
#include "sha256.hpp"
#include <set>

namespace bench {
struct Check { Query query; Entry expected; };
struct Batch {
    Range range;
    std::vector<Query> queries;
    std::vector<Check> checks;
};
struct Cycle { Update update; std::vector<Batch> batches; };
struct Workload {
    Array initial;
    std::vector<Batch> batches;
    std::vector<Cycle> cycles;
    std::string hash;
};

inline Batch make_batch(const Array &state, Range range, std::size_t count,
                        std::uint64_t seed, bool exhaustive) {
    Batch batch{range, generar_consultas(state.size(), range.length, count, seed), {}};
    const auto samples = exhaustive ? count : std::min(std::size_t{8}, count);
    std::set<std::pair<std::uint64_t, std::uint64_t>> seen;
    for (std::size_t i = 0; i < samples; ++i) {
        const auto index = samples == 1 ? 0 : i * (count - 1) / (samples - 1);
        const auto q = batch.queries[index];
        if (seen.emplace(q.left, q.right).second)
            batch.checks.push_back({q, reference_rmq(state, q)});
    }
    return batch;
}

inline void encode(std::string &bytes, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) bytes.push_back(static_cast<char>((value >> (i * 8)) & 255));
}

inline Workload prepare_workload(const Options &o, std::size_t n, std::uint64_t seed, Progress *progress = nullptr) {
    Workload w;
    w.initial = generar_arreglo(n, seed);
    if (progress) progress->advance("Preparacion arreglo numerico");
    const auto ranges = rangos(n);
    const bool exhaustive = o.profile == "smoke";
    for (std::size_t j = 0; j < ranges.size(); ++j) {
        w.batches.push_back(make_batch(w.initial, ranges[j], o.static_queries,
                                      derive_seed(seed, n + j), exhaustive));
        if (progress) progress->advance("Preparacion consultas estaticas rango=" + ranges[j].name);
    }
    const auto updates = generar_actualizaciones(w.initial, o.cycles, derive_seed(seed, n + 100));
    auto state = w.initial;
    for (std::size_t c = 0; c < o.cycles; ++c) {
        const auto update = updates[c];
        state[update.index] = {update.value, update.index};
        Cycle cycle{update, {}};
        for (std::size_t j = 0; j < ranges.size(); ++j) {
            const auto extra = ((j + c) % ranges.size()) < o.dynamic_queries % ranges.size();
            const auto count = o.dynamic_queries / ranges.size() + extra;
            cycle.batches.push_back(make_batch(state, ranges[j], count,
                derive_seed(seed, n + 1000 + c * 16 + j), exhaustive));
        }
        w.cycles.push_back(std::move(cycle));
        if (progress) progress->advance("Preparacion consultas dinamicas ciclo=" + std::to_string(c + 1));
    }
    // Serializacion canonica independiente del relleno de los pares
    std::string bytes;
    encode(bytes, n);
    encode(bytes, seed);
    for (auto [value, index] : w.initial) {
        encode(bytes, static_cast<std::uint64_t>(value));
        encode(bytes, index);
    }
    auto append_batches = [&](const auto &batches) {
        encode(bytes, batches.size());
        for (const auto &batch : batches) {
            encode(bytes, batch.range.length);
            encode(bytes, batch.queries.size());
            for (auto q : batch.queries) { encode(bytes, q.left); encode(bytes, q.right); }
        }
    };
    append_batches(w.batches);
    encode(bytes, w.cycles.size());
    for (const auto &cycle : w.cycles) {
        encode(bytes, cycle.update.index);
        encode(bytes, static_cast<std::uint64_t>(cycle.update.value));
        append_batches(cycle.batches);
    }
    w.hash = sha256(bytes);
    if (progress) progress->advance("Preparacion y huella completas");
    return w;
}

template<class Call>
void validate_batch(Call query, const Batch &batch) {
    for (const auto &check : batch.checks)
        if (query(check.query) != check.expected) throw std::runtime_error("Respuesta RMQ incorrecta");
}
}
