#include "indexed_framebuffer.h"

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    std::array<std::uint8_t, noctis::palette_bytes> working{};
    std::array<std::uint8_t, noctis::palette_bytes> current{};
    working[0] = 1;
    working[1] = 2;
    working[2] = 3;
    const std::array<std::uint8_t, 6> source{63, 32, 1, 10, 20, 30};

    noctis::apply_vga_palette(source.data(), working.data(), current.data(), 1, 2, 63, 31, 0);

    bool ok = true;
    const std::array<std::uint8_t, 9> expected_palette{1, 2, 3, 63, 15, 0, 10, 9, 0};
    for (std::size_t i = 0; i < expected_palette.size(); ++i) {
        if (current[i] != expected_palette[i]) {
            std::fprintf(stderr, "palette[%zu]: expected %u, got %u\n", i,
                         expected_palette[i], current[i]);
            ok = false;
        }
    }

    const std::array<std::uint8_t, 3> indices{1, 2, 0};
    std::array<std::uint8_t, 12> rgba{};
    noctis::expand_indexed_rgba(indices.data(), indices.size(), current.data(), rgba.data());
    const std::array<std::uint8_t, 12> expected_rgba{
        252, 60, 0, 255,
        40, 36, 0, 255,
        4, 8, 12, 255,
    };
    for (std::size_t i = 0; i < expected_rgba.size(); ++i) {
        if (rgba[i] != expected_rgba[i]) {
            std::fprintf(stderr, "rgba[%zu]: expected %u, got %u\n", i,
                         expected_rgba[i], rgba[i]);
            ok = false;
        }
    }

    const std::array<std::uint8_t, 3> overbright{70, 70, 70};
    noctis::apply_vga_palette(overbright.data(), working.data(), current.data(), 255, 1, 63, 63, 63);
    ok &= current[255 * 3 + 0] == 63;
    ok &= current[255 * 3 + 1] == 63;
    ok &= current[255 * 3 + 2] == 63;
    return ok ? 0 : 1;
}
