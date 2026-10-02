#include "brtl.h"

#include <cstdint>
#include <cstdlib>
#include <ctype.h>

namespace {
uint32_t brtl_seed = 1;
}

void brtl_strupr(char *str) {
    while (*str) {
        *str = toupper(*str);
        str++;
    }
}

void brtl_srand(uint16_t seed) { brtl_seed = seed; }

int16_t brtl_rand() {
    // Borland's 32-bit state wraps modulo 2^32; signed overflow is undefined in C++.
    brtl_seed = brtl_seed * UINT32_C(0x015a4e35) + 1u;
    return static_cast<int16_t>((brtl_seed >> 16u) & 0x7fffu);
}

int16_t brtl_random(int16_t num) { return (int16_t) (((int32_t) brtl_rand() * num) / (((uint16_t) 0x7FFF) + 1)); }
