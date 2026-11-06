// Motor compartido para no duplicar el mantenimiento dinamico

#include "range_queries/sparse_table.hpp"
#include "range_queries/segment_tree.hpp"
#include "utils/measurement.hpp"
#include "utils/rmq_workload.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <type_traits>

namespace {
using namespace bench;
using Sparse = sparse_table<Entry>;
using Segment = segment_tree<Entry>;
Entry minimum(Entry a, Entry b) { return std::min(a, b); }

struct Output {
    std::string session_id, config_id;
    std::array<std::ofstream, 4> files;
    const std::array<std::string, 4> names{"construction", "static_queries", "maintenance", "dynamic_queries"};
    explicit Output(const Options &o) : session_id(o.session_id), config_id(o.config_id) {
        for (std::size_t i = 0; i < files.size(); ++i) {
            const auto path = o.output / (names[i] + ".csv");
            if (std::filesystem::exists(path)) throw std::runtime_error("No se sobrescriben mediciones");
            files[i].exceptions(std::ios::failbit | std::ios::badbit);
            files[i].open(path);
            files[i] << "experiment,method,n,k,range,seed,run,cycle,operations,elapsed_ns,time_ns,checksum,input_sha256,session_id,config_id\n"
                     << std::setprecision(17);
        }
    }
    void row(std::size_t experiment, int method, std::size_t n, const Range &range,
             std::uint64_t seed, std::size_t run, int cycle, Sample sample, const std::string &hash) {
        files[experiment] << names[experiment] << ',' << (method ? "segment_tree" : "sparse_table")
            << ',' << n << ',' << range.length << ',' << range.name << ',' << seed << ',' << run
            << ',' << cycle << ',' << sample.operations << ',' << sample.elapsed << ','
            << sample.elapsed / sample.operations << ',' << sample.checksum << ',' << hash << ','
            << session_id << ',' << config_id << '\n';
    }
    void close() { for (auto &file : files) { file.flush(); file.close(); } }
};

template<class Call>
std::size_t calibrate(Call query, const Batch &batch, const Options &o, std::ofstream &output,
                      int method, std::size_t n, std::uint64_t seed) {
    auto count = o.static_queries;
    Sample sample{};
    while (true) {
        sample = timed_queries(query, batch.queries, count);
        if (sample.elapsed >= o.target_ns || count == o.max_batch) break;
        count = count > o.max_batch / 2 ? o.max_batch : count * 2;
    }
    output << (method ? "segment_tree" : "sparse_table") << ',' << n << ',' << seed << ','
           << batch.range.name << ',' << batch.range.length << ',' << count << ',' << sample.elapsed
           << ',' << (sample.elapsed >= o.target_ns) << '\n';
    return count;
}

template<bool is_sparse>
void measure_method(const Workload &work,
                    const std::vector<std::array<std::size_t, 2>> &batches,
                    std::size_t n, std::uint64_t seed, std::size_t repetition,
                    bool compare, std::vector<std::uint64_t> &expected, Output &output, Progress &progress) {
    constexpr int method = is_sparse ? 0 : 1;
    using Tree = std::conditional_t<is_sparse, Sparse, Segment>;
    std::optional<Tree> tree;
    auto query = [&](Query q) {
        if constexpr (is_sparse) return tree->query_idempotent(q.left, q.right);
        else return tree->query(q.left, q.right);
    };
    progress.show("Experimento 1 construccion");
    const auto constructed = timed_operation([&] {
        if constexpr (is_sparse) tree.emplace(work.initial, minimum);
        else tree.emplace(work.initial);
    });
    output.row(0, method, n, {"none", 0}, seed, repetition, -1, {constructed, 1, 0}, work.hash);
    progress.advance("Experimento 1 construccion completa");
    progress.show("Experimento 2 consultas estaticas");
    for (std::size_t j = 0; j < work.batches.size(); ++j) {
        const auto &batch = work.batches[j];
        timed_queries(query, batch.queries, std::min(std::size_t{256}, batch.queries.size()));
        validate_batch(query, batch);
        const auto sample = timed_queries(query, batch.queries, batches[j][method]);
        output.row(1, method, n, batch.range, seed, repetition, -1, sample, work.hash);
    }
    progress.advance("Experimento 2 consultas estaticas completas");
    auto state = work.initial;
    std::size_t checksum_index = 0;
    for (std::size_t c = 0; c < work.cycles.size(); ++c) {
        const auto &cycle = work.cycles[c];
        const auto update = cycle.update;
        state[update.index] = {update.value, update.index};
        double elapsed;
        const auto cycle_label = " ciclo=" + std::to_string(c + 1) + '/' + std::to_string(work.cycles.size());
        progress.show("Experimento 3 mantenimiento" + cycle_label);
        if constexpr (is_sparse) {
            std::optional<Sparse> fresh;
            elapsed = timed_operation([&] { fresh.emplace(state, minimum); });
            // Sustituye y libera la tabla anterior fuera del reloj
            tree.swap(fresh);
        } else {
            elapsed = timed_operation([&] { tree->update(update.index, state[update.index]); });
        }
        output.row(2, method, n, {"none", 0}, seed, repetition, static_cast<int>(c),
                   {elapsed, 1, 0}, work.hash);
        progress.advance("Experimento 3 mantenimiento completo" + cycle_label);
        progress.show("Experimento 4 consultas dinamicas" + cycle_label);
        for (const auto &batch : cycle.batches) {
            const auto sample = timed_queries(query, batch.queries, batch.queries.size());
            output.row(3, method, n, batch.range, seed, repetition, static_cast<int>(c), sample, work.hash);
            if (compare) {
                if (sample.checksum != expected.at(checksum_index))
                    throw std::runtime_error("Resultados dinamicos diferentes");
            } else expected.push_back(sample.checksum);
            ++checksum_index;
        }
        // Verifica despues de medir todos los lotes del estado vigente
        for (const auto &batch : cycle.batches) validate_batch(query, batch);
        if (query({update.index, update.index + 1}) != state[update.index])
            throw std::runtime_error("Actualizacion incorrecta");
        progress.advance("Experimento 4 consultas y validacion completas" + cycle_label);
    }
    if (checksum_index != expected.size()) throw std::runtime_error("Recorridos incompatibles");
}

void run(const Options &o) {
    std::size_t total = 1;
    for (const auto n : o.sizes)
        total += o.seeds.size() * (3 + 3 * rangos(n).size() + o.cycles + o.runs * 2 * (2 + 2 * o.cycles));
    Progress progress(total, "Mediciones " + o.profile);
    Output output(o);
    std::ofstream calibration(o.output / "calibration.csv");
    calibration.exceptions(std::ios::failbit | std::ios::badbit);
    calibration << "method,n,seed,range,k,operations,elapsed_ns,target_reached\n" << std::setprecision(17);
    for (const auto n : o.sizes) for (const auto seed : o.seeds) {
        const auto context = "n=" + std::to_string(n) + " semilla=" + std::to_string(seed);
        progress.context(context);
        progress.show("Preparacion de datos y referencias", true);
        const auto work = prepare_workload(o, n, seed, &progress);
        std::vector<std::array<std::size_t, 2>> batches;
        // El calentamiento y la calibracion no se guardan como muestras
        {
            progress.show("Calentamiento y calibracion", true);
            Sparse sparse(work.initial, minimum);
            Segment segment(work.initial);
            for (const auto &batch : work.batches) {
                auto qs = [&](Query q) { return sparse.query_idempotent(q.left, q.right); };
                auto qt = [&](Query q) { return segment.query(q.left, q.right); };
                validate_batch(qs, batch);
                validate_batch(qt, batch);
                const auto sparse_count = calibrate(qs, batch, o, calibration, 0, n, seed);
                progress.advance("Calibracion sparse_table rango=" + batch.range.name);
                const auto segment_count = calibrate(qt, batch, o, calibration, 1, n, seed);
                progress.advance("Calibracion segment_tree rango=" + batch.range.name);
                batches.push_back({sparse_count, segment_count});
            }
            // Calienta tambien las rutas de actualizacion y reconstruccion
            const auto update = work.cycles.front().update;
            auto state = work.initial;
            state[update.index] = {update.value, update.index};
            segment.update(update.index, state[update.index]);
            Sparse fresh(state, minimum);
            consume(fresh.query_idempotent(update.index, update.index + 1));
        }
        progress.advance("Calentamiento completo");
        for (std::size_t repetition = 0; repetition < o.runs; ++repetition) {
            std::vector<std::uint64_t> expected;
            expected.reserve(o.cycles * work.batches.size());
            // Cada metodo recorre su carga sin mantener viva la otra estructura
            for (int order = 0; order < 2; ++order) {
                const int method = static_cast<int>((repetition + order) % 2);
                progress.context(context + " repeticion=" + std::to_string(repetition + 1) + '/' + std::to_string(o.runs)
                                 + (method ? " segment_tree" : " sparse_table"));
                progress.show("Experimentos 1 a 4", repetition == 0 && order == 0);
                if (method) measure_method<false>(work, batches, n, seed, repetition, order != 0, expected, output, progress);
                else measure_method<true>(work, batches, n, seed, repetition, order != 0, expected, output, progress);
            }
        }
    }
    output.close();
    calibration.close();
    std::ofstream marker(o.output / "MEASURE_COMPLETE");
    marker.exceptions(std::ios::failbit | std::ios::badbit);
    marker << "rmq-v1\n";
    marker.close();
    progress.context("");
    progress.advance("CSV y mediciones completos");
}
}

int main(int argc, char **argv) {
    try { run(bench::parse_options(argc, argv)); }
    catch (const std::exception &error) { std::cerr << "Error: " << error.what() << '\n'; return 1; }
}
