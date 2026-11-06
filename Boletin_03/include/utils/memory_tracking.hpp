#ifndef BENCH_MEMORY_TRACKING_HPP
#define BENCH_MEMORY_TRACKING_HPP
#include <cstddef>

namespace bench::memory {
struct Usage {
    std::size_t retained = 0, peak = 0, temporary_peak = 0;
};
void begin();
Usage finish();
void cancel() noexcept;
}
#endif
