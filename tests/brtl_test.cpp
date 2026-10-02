#include "brtl.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

int main() {
    constexpr std::array<int16_t, 8> expected{
        346, 130, 10982, 1090, 11656, 7117, 17595, 6415
    };

    brtl_srand(1);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto actual = brtl_rand();
        if (actual != expected[i]) {
            std::fprintf(stderr, "brtl_rand[%zu]: expected %d, got %d\n",
                         i, expected[i], actual);
            return 1;
        }
    }

    brtl_srand(1);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (brtl_rand() != expected[i]) {
            std::fputs("brtl_srand did not reproduce the sequence\n", stderr);
            return 1;
        }
    }

    brtl_srand(0);
    for (int i = 0; i < 1000; ++i) {
        const auto value = brtl_random(100);
        if (value < 0 || value >= 100) {
            std::fprintf(stderr, "brtl_random(100) out of range: %d\n", value);
            return 1;
        }
    }

    char text[] = "Noctis iv+";
    brtl_strupr(text);
    if (std::strcmp(text, "NOCTIS IV+") != 0) {
        std::fputs("brtl_strupr changed the wrong bytes\n", stderr);
        return 1;
    }

    return 0;
}
