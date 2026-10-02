#include "legacy_prng.h"

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    constexpr std::array<std::int32_t, 8> seed_one{
        0x0009, 0x0090, 0x5f10, 0x1b90, 0x36fd, 0x50b7, 0x217a, 0x7b84,
    };
    constexpr std::array<std::int32_t, 8> wide_seed{
        0x5ff5, 0x7138, 0x1ec0, 0x0a71, 0x5708, 0x5381, 0x5122, 0x245a,
    };
    constexpr std::array<std::int32_t, 8> wrapping_low_byte{
        0x00ff, 0x7c04, 0x7404, 0x4ca6, 0x6bf6, 0x4a0f, 0x0002, 0x4fcd,
    };

    fast_srand(1);
    for (std::size_t i = 0; i < seed_one.size(); ++i) {
        const auto actual = fast_random(0x7fff);
        if (actual != seed_one[i]) {
            std::fprintf(stderr, "fast_random seed-one[%zu]: expected %d, got %d\n",
                         i, seed_one[i], actual);
            return 1;
        }
    }

    fast_srand(UINT32_C(0x12345678));
    for (std::size_t i = 0; i < wide_seed.size(); ++i) {
        const auto actual = fast_random(0x7fff);
        if (actual != wide_seed[i]) {
            std::fprintf(stderr, "fast_random wide-seed[%zu]: expected %d, got %d\n",
                         i, wide_seed[i], actual);
            return 1;
        }
    }

    fast_srand(UINT32_MAX);
    for (std::size_t i = 0; i < wrapping_low_byte.size(); ++i) {
        const auto actual = fast_random(0x7fff);
        if (actual != wrapping_low_byte[i]) {
            std::fprintf(stderr, "fast_random byte-wrap[%zu]: expected %d, got %d\n",
                         i, wrapping_low_byte[i], actual);
            return 1;
        }
    }

    fast_srand(0);
    if (fast_random(0x7fff) != seed_one[0]) {
        std::fputs("fast_srand did not preserve the original seed | 3 behavior\n", stderr);
        return 1;
    }
    if (ranged_fast_random(0) != 0) {
        std::fputs("ranged_fast_random did not clamp a non-positive range\n", stderr);
        return 1;
    }
    return 0;
}
