#include <iostream>
#include "utils/experiment_options.hpp"
#include "utils/pattern_selection.hpp"

int main(int argc, char **argv) {
    try {
        const auto o = bench::parse_options(argc, argv);
        const auto text = bench::load_text_prefix(o.text, o.size);
        const auto hash = bench::sha256(text);
        if (o.profile == "official" && hash != bench::corpus_hash)
            throw std::runtime_error("El corpus oficial no coincide con su SHA256");
        const auto config = bench::preparation_config(o, hash);
        if (std::filesystem::exists(o.patterns)) {
            if (bench::read_file(o.patterns / "config.txt") != config)
                throw std::runtime_error("Ya existen patrones con otra configuracion");
            for (const auto &name : {"text_size", "pattern_size", "occurrences"})
                for (const auto &p : bench::load_patterns(o.patterns / (std::string(name) + ".csv"), hash))
                    if (bench::reference_count(p.bytes, text) != p.z)
                        throw std::runtime_error("El manifiesto existente tiene un conteo incorrecto");
            std::cout << "Patrones existentes verificados\n";
            return 0;
        }
        // Preparamos todos los controles antes de publicar los manifiestos
        auto n_patterns = bench::select_unique(text, 32, o.count, o.sizes.front());
        std::vector<bench::Pattern> m_patterns;
        for (auto m : o.lengths) {
            auto selected = bench::select_unique(text, m, o.count, text.size());
            m_patterns.insert(m_patterns.end(), selected.begin(), selected.end());
        }
        auto z_patterns = bench::select_occurrences(text, o.count);
        auto temporary = o.patterns;
        temporary += ".partial";
        if (!std::filesystem::create_directories(temporary))
            throw std::runtime_error("Ya existe una preparacion parcial revise su directorio");
        bench::write_patterns(temporary / "text_size.csv", n_patterns, hash);
        bench::write_patterns(temporary / "pattern_size.csv", m_patterns, hash);
        bench::write_patterns(temporary / "occurrences.csv", z_patterns, hash);
        std::ofstream config_file(temporary / "config.txt");
        config_file << config;
        config_file.close();
        if (!config_file) throw std::runtime_error("No se pudo guardar la configuracion");
        std::filesystem::rename(temporary, o.patterns);
        std::cout << "Patrones preparados en " << o.patterns << '\n';
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
