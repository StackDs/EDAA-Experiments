#ifndef BENCH_OUTPUT_HPP
#define BENCH_OUTPUT_HPP
#include <iomanip>
#include "experiment_options.hpp"
#include "uhr.hpp"
#include "memory_tracking.hpp"

namespace bench {
struct Context {
    std::string dataset, hash, pattern_id, method, phase;
    std::size_t n = 0, m = 0, z = 0;
};

class Output {
    std::filesystem::path directory;
    std::string experiment, session, config_id;
    std::ofstream raw, summary, memory_file;

    std::string prefix(const Context &c) const {
        std::ostringstream out;
        out << experiment << ',' << c.dataset << ',' << c.n << ',' << c.m << ',' << c.z << ','
            << c.pattern_id << ',' << c.method << ',' << c.phase << ',' << c.hash << ','
            << session << ',' << config_id;
        return out.str();
    }

public:
    Output(const Options &o, const std::string &name, const std::string &hash)
        : directory(o.output), experiment(name), session(o.output.parent_path().filename().string()) {
        if (session.find_first_of(",\r\n\"") != std::string::npos)
            throw std::runtime_error("Nombre de sesion no valido para CSV");
        if (directory.empty() || session.empty() || !std::filesystem::create_directory(directory))
            throw std::runtime_error("La salida debe ser un directorio nuevo dentro de una sesion");
        const auto manifest = read_file(o.patterns / (name + ".csv"));
        const auto config = preparation_config(o, hash) + "experiment=" + name +
            "\nruns=" + std::to_string(o.runs) + "\ntarget_ns=" + std::to_string(o.target_ns) +
            "\nmax_batch=" + std::to_string(o.max_batch) + "\nq=101\nmanifest_sha256=" + sha256(manifest) + "\n";
        config_id = sha256(config);
        std::ofstream meta(directory / "config.txt");
        meta << config << "config_id=" << config_id << "\n";
        if (!meta) throw std::runtime_error("No se pudieron guardar los metadatos");
        std::ofstream saved_manifest(directory / "patterns.csv");
        saved_manifest << manifest;
        if (!saved_manifest) throw std::runtime_error("No se pudieron conservar los patrones usados");
        raw.open(directory / "samples.csv");
        summary.open(directory / "summary.csv");
        memory_file.open(directory / "memory.csv");
        const std::string common = "experiment,dataset,n,m,z,pattern_id,method,phase,corpus_sha256,session_id,config_id";
        raw << common << ",run,batch_size,time_ns\n";
        summary << common << ",runs,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4\n";
        memory_file << common << ",auxiliary_peak_bytes,index_bytes,construction_peak_bytes,construction_temporary_bytes\n";
        raw << std::setprecision(17);
        summary << std::setprecision(17);
    }

    void times(const Context &c, const std::vector<Sample> &samples) {
        std::vector<double> values;
        for (std::size_t run = 0; run < samples.size(); ++run) {
            const auto &s = samples[run];
            raw << prefix(c) << ',' << run << ',' << s.batch_size << ',' << s.time_ns << '\n';
            values.push_back(s.time_ns);
        }
        const auto stats = summarize(values);
        summary << prefix(c) << ',' << samples.size() << ',' << stats.mean << ',' << stats.stdev;
        for (auto q : stats.q) summary << ',' << q;
        summary << '\n';
    }

    void memory(const Context &c, memory::Usage usage) {
        memory_file << prefix(c);
        if (c.phase == "construction")
            memory_file << ",0," << usage.retained << ',' << usage.peak << ',' << usage.temporary_peak;
        else memory_file << ',' << usage.peak << ",0,0,0";
        memory_file << '\n';
    }

    void complete() {
        raw.flush(); summary.flush(); memory_file.flush();
        if (!raw || !summary || !memory_file) throw std::runtime_error("Fallo al guardar los CSV");
        std::ofstream marker(directory / "COMPLETE");
        marker << config_id << '\n';
        if (!marker) throw std::runtime_error("No se pudo cerrar la sesion");
    }
};
}
#endif
