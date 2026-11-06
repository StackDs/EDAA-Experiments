#ifndef BENCH_INDEX_STORAGE_HPP
#define BENCH_INDEX_STORAGE_HPP
#include <cstddef>
#include <memory>
#include "string_matching/suffix_array.hpp"

namespace bench {
// Conservamos el indice en una ubicacion fija para mantener su vista interna
class IndexStorage {
    alignas(suffix_array) std::byte storage[sizeof(suffix_array)];
    suffix_array *value = nullptr;
public:
    IndexStorage() = default;
    IndexStorage(const IndexStorage &) = delete;
    IndexStorage &operator=(const IndexStorage &) = delete;
    ~IndexStorage() { reset(); }
    void reset() { if (value) { std::destroy_at(value); value = nullptr; } }
    void build(const std::string &text) {
        reset();
        value = std::construct_at(reinterpret_cast<suffix_array *>(storage), text);
    }
    suffix_array &get() { return *value; }
};
}
#endif
