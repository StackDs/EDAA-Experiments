#pragma once

// Adaptado del medidor uhr del boletin 03
// Autor original LELE
// Fuente https://github.com/leonardlover

#include "rmq_inputs.hpp"
#include <chrono>
#include <cstddef>

namespace bench {
using Clock = std::chrono::steady_clock;
inline double elapsed_ns(Clock::time_point begin, Clock::time_point end) {
    return std::chrono::duration<double, std::nano>(end - begin).count();
}
inline void consume(Entry value) {
    asm volatile("" : : "g"(value.first), "g"(value.second) : "memory");
}
struct Sample { double elapsed; std::size_t operations; std::uint64_t checksum; };

template<class QueryCall>
Sample timed_queries(QueryCall query, const std::vector<Query> &queries, std::size_t count) {
    std::uint64_t checksum = 1469598103934665603ULL;
    std::size_t index = 0;
    const auto begin = Clock::now();
    for (std::size_t i = 0; i < count; ++i) {
        asm volatile("" : : : "memory");
        const auto value = query(queries[index]);
        consume(value);
        checksum = (checksum ^ static_cast<std::uint64_t>(value.first)) * 1099511628211ULL;
        checksum = (checksum ^ value.second) * 1099511628211ULL;
        if (++index == queries.size()) index = 0;
    }
    const auto end = Clock::now();
    return {elapsed_ns(begin, end), count, checksum};
}

template<class Call>
double timed_operation(Call call) {
    asm volatile("" : : : "memory");
    const auto begin = Clock::now();
    call();
    asm volatile("" : : : "memory");
    const auto end = Clock::now();
    return elapsed_ns(begin, end);
}
}
