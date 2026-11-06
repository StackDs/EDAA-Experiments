#include "utils/memory_tracking.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <new>
#include <stdexcept>

namespace {
struct Allocation { void *pointer; std::size_t size, start, end; };
// Guardamos el seguimiento en memoria estatica para no medir al medidor
std::array<Allocation, 4096> records;
std::size_t count = 0, event = 0, live = 0, peak = 0;
bool active = false, overflow = false;
void allocated(void *pointer, std::size_t size) noexcept {
    if (!active) return;
    if (count == records.size()) { overflow = true; return; }
    records[count++] = {pointer, size, ++event, 0};
    live += size;
    peak = std::max(peak, live);
}
void released(void *pointer) noexcept {
    if (!active) return;
    for (std::size_t i = count; i > 0; --i) {
        auto &record = records[i - 1];
        if (record.pointer == pointer && record.end == 0) {
            record.end = ++event;
            live -= record.size;
            return;
        }
    }
}
}

void *operator new(std::size_t size) {
    auto *pointer = std::malloc(size ? size : 1);
    if (!pointer) throw std::bad_alloc();
    allocated(pointer, size);
    return pointer;
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { released(p); std::free(p); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete(void *p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }
void *operator new(std::size_t size, std::align_val_t alignment) {
    void *pointer = nullptr;
    if (posix_memalign(&pointer, static_cast<std::size_t>(alignment), size ? size : 1))
        throw std::bad_alloc();
    allocated(pointer, size);
    return pointer;
}
void *operator new[](std::size_t size, std::align_val_t a) { return ::operator new(size, a); }
void operator delete(void *p, std::align_val_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::align_val_t) noexcept { ::operator delete(p); }
void operator delete(void *p, std::size_t, std::align_val_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept { ::operator delete(p); }

namespace bench::memory {
void begin() {
    if (active) throw std::runtime_error("Seguimiento de memoria anidado");
    count = event = live = peak = 0;
    overflow = false;
    active = true;
}
void cancel() noexcept { active = false; }
Usage finish() {
    if (!active) throw std::runtime_error("No hay seguimiento de memoria activo");
    active = false;
    if (overflow) throw std::runtime_error("Demasiadas asignaciones para el medidor de memoria");
    Usage result{live, peak, 0};
    // Separamos las asignaciones liberadas durante la construccion
    for (std::size_t instant = 1; instant <= event; ++instant) {
        std::size_t temporary = 0;
        for (std::size_t i = 0; i < count; ++i) {
            const auto &r = records[i];
            if (r.end && r.start <= instant && instant < r.end) temporary += r.size;
        }
        result.temporary_peak = std::max(result.temporary_peak, temporary);
    }
    return result;
}
}
