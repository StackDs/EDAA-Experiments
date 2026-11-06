#ifndef BENCH_UHR_HPP
#define BENCH_UHR_HPP

// Adaptamos el esquema uhr de LELE para medir lotes de consultas
// https://github.com/leonardlover

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace bench {
using Clock = std::chrono::steady_clock;
inline double elapsed_ns(Clock::time_point begin, Clock::time_point end) {
    return std::chrono::duration<double, std::nano>(end - begin).count();
}
struct Statistics { double mean, stdev; std::array<double, 5> q; };

inline Statistics summarize(std::vector<double> data) {
    if (data.size() < 4) throw std::runtime_error("uhr requiere al menos cuatro repeticiones");
    const auto n = data.size();
    Statistics result{};
    result.mean = std::accumulate(data.begin(), data.end(), 0.0) / n;
    for (auto value : data) result.stdev += (value - result.mean) * (value - result.mean);
    result.stdev = std::sqrt(result.stdev / (n - 1));
    std::sort(data.begin(), data.end());
    result.q[0] = data.front();
    result.q[4] = data.back();
    result.q[2] = n % 2 ? data[n / 2] : (data[n / 2 - 1] + data[n / 2]) / 2;
    // Conservamos la convencion de cuartiles de la plantilla original
    if (n % 4 >= 2) {
        result.q[1] = data[n / 4];
        result.q[3] = data[3 * n / 4];
    } else {
        result.q[1] = .25 * data[n / 4 - 1] + .75 * data[n / 4];
        result.q[3] = .75 * data[3 * n / 4 - 1] + .25 * data[3 * n / 4];
    }
    return result;
}

struct Sample { std::size_t batch_size; double time_ns; };

template<class Call>
double timed_batch(Call &call, std::uint64_t expected, std::size_t batch_size) {
    std::uint64_t total = 0;
    const auto begin = Clock::now();
    for (std::size_t j = 0; j < batch_size; ++j) {
        // Evitamos sacar las consultas repetidas fuera del lote medido
        asm volatile("" : : : "memory");
        const auto value = static_cast<std::uint64_t>(call());
        asm volatile("" : : "g"(value) : "memory");
        total += value;
    }
    const auto end = Clock::now();
    // Validamos el resultado fuera del reloj
    if (total != expected * batch_size) throw std::runtime_error("Conteo incorrecto en el lote medido");
    return elapsed_ns(begin, end);
}

template<class Call>
std::vector<Sample> measure_queries(Call call, std::uint64_t expected, std::size_t runs,
                                   std::size_t target_ns, std::size_t max_batch,
                                   void (*progress)(std::size_t, std::size_t) = nullptr) {
    if (runs < 4 || !target_ns || !max_batch) throw std::runtime_error("Parametros de medicion invalidos");
    // Calentamos y calibramos sin guardar estas ejecuciones como muestras
    timed_batch(call, expected, 1);
    std::size_t batch = 1;
    while (timed_batch(call, expected, batch) < target_ns && batch < max_batch)
        batch = std::min(max_batch, batch * 2);
    std::vector<Sample> samples;
    samples.reserve(runs);
    if (progress) progress(0, runs);
    for (std::size_t run = 0; run < runs; ++run) {
        samples.push_back({batch, timed_batch(call, expected, batch) / batch});
        // Actualizamos la terminal despues de cerrar el intervalo medido
        if (progress) progress(run + 1, runs);
    }
    return samples;
}
}
#endif
