#ifndef BENCH_SHA256_HPP
#define BENCH_SHA256_HPP

#include <array>
#include <bit>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace bench {
inline std::string sha256(std::string_view input) {
    constexpr std::array<std::uint32_t, 64> constants = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::array<std::uint32_t, 8> hash = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    const std::size_t padded = ((input.size() + 9 + 63) / 64) * 64;
    // Procesamos bloques sin copiar el corpus completo
    for (std::size_t offset = 0; offset < padded; offset += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t j = 0; j < 64; ++j) {
            const auto pos = offset + j;
            unsigned byte = 0;
            if (pos < input.size()) byte = static_cast<unsigned char>(input[pos]);
            else if (pos == input.size()) byte = 128;
            else if (pos >= padded - 8)
                byte = static_cast<unsigned>((std::uint64_t(input.size()) * 8)
                       >> ((padded - 1 - pos) * 8)) & 255;
            words[j / 4] |= std::uint32_t(byte) << ((3 - j % 4) * 8);
        }
        for (int j = 16; j < 64; ++j) {
            const auto x = words[j - 15], y = words[j - 2];
            words[j] = words[j - 16] + (std::rotr(x,7) ^ std::rotr(x,18) ^ (x >> 3))
                     + words[j - 7] + (std::rotr(y,17) ^ std::rotr(y,19) ^ (y >> 10));
        }
        auto state = hash;
        for (int j = 0; j < 64; ++j) {
            const auto [a,b,c,d,e,f,g,h] = state;
            const auto t1 = h + (std::rotr(e,6) ^ std::rotr(e,11) ^ std::rotr(e,25))
                          + ((e & f) ^ (~e & g)) + constants[j] + words[j];
            const auto t2 = (std::rotr(a,2) ^ std::rotr(a,13) ^ std::rotr(a,22))
                          + ((a & b) ^ (a & c) ^ (b & c));
            state = {t1+t2,a,b,c,d+t1,e,f,g};
        }
        for (int j = 0; j < 8; ++j) hash[j] += state[j];
    }
    std::ostringstream output;
    for (auto value : hash) output << std::hex << std::setfill('0') << std::setw(8) << value;
    return output.str();
}
}
#endif
