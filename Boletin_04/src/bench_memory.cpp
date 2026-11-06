// El seguimiento se enlaza solo con este ejecutable

#include "range_queries/sparse_table.hpp"
#include "range_queries/segment_tree.hpp"
#include "utils/experiment_options.hpp"
#include "utils/memory_tracking.hpp"
#include "utils/progress.hpp"
#include <fstream>
#include <iostream>
#include <optional>

namespace {
using namespace bench;
using Sparse = sparse_table<Entry>;
using Segment = segment_tree<Entry>;
Entry minimum(Entry a, Entry b) { return std::min(a, b); }

template<class Call>
memory::Usage track(Call call) {
    memory::begin();
    try { call(); return memory::finish(); }
    catch (...) { memory::cancel(); throw; }
}

void run(const Options &o) {
    Progress progress(o.sizes.size() * o.seeds.size() * 4 + 1, "Memoria " + o.profile);
    if (std::filesystem::exists(o.output / "memory.csv"))
        throw std::runtime_error("No se sobrescriben mediciones de memoria");
    std::ofstream out(o.output / "memory.csv");
    out.exceptions(std::ios::failbit | std::ios::badbit);
    out << "method,phase,n,seed,input_bytes,baseline_bytes,new_retained_bytes,allocation_peak_bytes,temporary_peak_bytes,total_peak_bytes,index_bytes,session_id,config_id\n";
    for (auto n : o.sizes) for (auto seed : o.seeds) {
        progress.context("n=" + std::to_string(n) + " semilla=" + std::to_string(seed));
        progress.show("Preparacion de memoria", true);
        auto values = generar_arreglo(n, seed);
        const auto update = generar_actualizaciones(values, 1, derive_seed(seed, n + 100)).front();
        auto row = [&](const std::string &method, const std::string &phase, memory::Usage usage,
                       std::size_t baseline, std::size_t index) {
            out << method << ',' << phase << ',' << n << ',' << seed << ',' << values.size() * sizeof(Entry)
                << ',' << baseline << ',' << usage.retained << ',' << usage.peak << ','
                << usage.temporary_peak << ',' << baseline + usage.peak << ',' << index << ','
                << o.session_id << ',' << o.config_id << '\n';
        };
        {
            std::optional<Sparse> table;
            progress.show("sparse_table construccion");
            const auto usage = track([&] { table.emplace(values, minimum); });
            row("sparse_table", "construction", usage, 0, usage.retained);
            progress.advance("sparse_table construccion completa");
            values[update.index] = {update.value, update.index};
            std::optional<Sparse> fresh;
            progress.show("sparse_table reconstruccion");
            const auto rebuilt = track([&] { fresh.emplace(values, minimum); });
            row("sparse_table", "reconstruction", rebuilt, usage.retained, rebuilt.retained);
            progress.advance("sparse_table reconstruccion completa");
        }
        values = generar_arreglo(n, seed);
        {
            std::optional<Segment> tree;
            progress.show("segment_tree construccion");
            const auto usage = track([&] { tree.emplace(values); });
            row("segment_tree", "construction", usage, 0, usage.retained);
            progress.advance("segment_tree construccion completa");
            progress.show("segment_tree update");
            const auto changed = track([&] { tree->update(update.index, {update.value, update.index}); });
            row("segment_tree", "update", changed, usage.retained, usage.retained);
            progress.advance("segment_tree update completo");
        }
    }
    out.flush();
    out.close();
    std::ofstream marker(o.output / "MEMORY_COMPLETE");
    marker.exceptions(std::ios::failbit | std::ios::badbit);
    marker << "rmq-v1\n";
    marker.close();
    progress.context("");
    progress.advance("CSV y memoria completos");
}
}

int main(int argc, char **argv) {
    try { run(bench::parse_options(argc, argv)); }
    catch (const std::exception &error) { std::cerr << "Error: " << error.what() << '\n'; return 1; }
}
