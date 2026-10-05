#include "noctis-d.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {
std::array<std::uint8_t, sc_bytes> framebuffer{};
}

std::uint8_t *adapted = framebuffer.data();

#include "tdpolygs.h"

namespace {
std::uint64_t fnv1a(const std::uint8_t *bytes, std::size_t size) {
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

std::size_t nonzero_pixels() {
    return static_cast<std::size_t>(std::count_if(framebuffer.begin(),
        framebuffer.begin() + adapted_width * adapted_height,
        [](std::uint8_t value) { return value != 0; }));
}

void reset_renderer() {
    framebuffer.fill(0);
    cam_x = cam_y = cam_z = 0;
    alfa = beta = ngamma = 0;
    dpp = 200;
    flares = 0;
    entity = 1;
    culling_needed = 0;
    halfscan_needed = 0;
    H_MATRIXS = V_MATRIXS = 16;
    change_camera_lens();
    change_txm_repeating_mode();
}
}

int main() {
    const std::size_t visible_bytes = adapted_width * adapted_height;

    reset_renderer();
    const std::array<float, 4> flat_x{-120.0F, 120.0F, 120.0F, -120.0F};
    const std::array<float, 4> flat_y{-75.0F, -75.0F, 75.0F, 75.0F};
    const std::array<float, 4> flat_z{500.0F, 500.0F, 500.0F, 500.0F};
    poly3d(flat_x.data(), flat_y.data(), flat_z.data(), 4, 42);
    const auto flat_hash = fnv1a(framebuffer.data(), visible_bytes);
    const auto flat_pixels = nonzero_pixels();

    reset_renderer();
    std::array<std::uint8_t, TEXTURE_X_SIZE * TEXTURE_Y_SIZE> texture{};
    for (std::size_t y = 0; y < TEXTURE_Y_SIZE; ++y) {
        for (std::size_t x = 0; x < TEXTURE_X_SIZE; ++x) {
            texture[y * TEXTURE_X_SIZE + x] = static_cast<std::uint8_t>(64 + ((x / 16 + y / 16) & 31));
        }
    }
    txtr = texture.data();
    std::array<float, 4> textured_x{-120.0F, 120.0F, 120.0F, -120.0F};
    std::array<float, 4> textured_y{-75.0F, -75.0F, 75.0F, 75.0F};
    std::array<float, 4> textured_z{500.0F, 500.0F, 500.0F, 500.0F};
    polymap(textured_x.data(), textured_y.data(), textured_z.data(), 4, 0);
    const auto textured_hash = fnv1a(framebuffer.data(), visible_bytes);
    const auto textured_pixels = nonzero_pixels();

    constexpr std::uint64_t expected_flat_hash = UINT64_C(0xf59885b32c4eda4f);
    constexpr std::uint64_t expected_textured_hash = UINT64_C(0xf64a31f3c71b46e3);
    constexpr std::size_t expected_flat_pixels = 5917;
    constexpr std::size_t expected_textured_pixels = 5856;
    if (flat_hash != expected_flat_hash || textured_hash != expected_textured_hash
        || flat_pixels != expected_flat_pixels || textured_pixels != expected_textured_pixels) {
        std::fprintf(stderr,
                     "renderer fixture: flat=%016llx/%zu textured=%016llx/%zu\n",
                     static_cast<unsigned long long>(flat_hash), flat_pixels,
                     static_cast<unsigned long long>(textured_hash), textured_pixels);
        return 1;
    }

    reset_renderer();
    flares = 1;
    framebuffer.fill(10);
    draw_triangle_2d(glm::ivec2(10, 10), glm::ivec2(30, 10), glm::ivec2(20, 30), 20);
    const auto blended_pixel = framebuffer[adapted_width * 15 + 20];
    if (blended_pixel != 30) {
        std::fprintf(stderr, "draw_triangle_2d flares=1 expected 30, got %u\n", blended_pixel);
        return 1;
    }

    // Sub-pixel fidelity rasterization test
    reset_renderer();
    set_subpixel_fidelity(true);
    framebuffer.fill(0);
    poly3d(flat_x.data(), flat_y.data(), flat_z.data(), 4, 42);
    const auto fidelity_pixels = nonzero_pixels();
    if (fidelity_pixels == 0) {
        std::fprintf(stderr, "subpixel fidelity: poly3d rendered 0 pixels\n");
        return 1;
    }

    framebuffer.fill(0);
    polymap(textured_x.data(), textured_y.data(), textured_z.data(), 4, 0);
    const auto polymap_fidelity_pixels = nonzero_pixels();
    if (polymap_fidelity_pixels == 0) {
        std::fprintf(stderr, "subpixel fidelity: polymap rendered 0 pixels\n");
        return 1;
    }
    set_subpixel_fidelity(false);

    // Verify restore to legacy mode yields exact original flat & textured hashes
    reset_renderer();
    poly3d(flat_x.data(), flat_y.data(), flat_z.data(), 4, 42);
    const auto restored_flat_hash = fnv1a(framebuffer.data(), visible_bytes);
    if (restored_flat_hash != expected_flat_hash) {
        std::fprintf(stderr, "subpixel fidelity: flat legacy restore mismatch\n");
        return 1;
    }

    reset_renderer();
    polymap(textured_x.data(), textured_y.data(), textured_z.data(), 4, 0);
    const auto restored_textured_hash = fnv1a(framebuffer.data(), visible_bytes);
    if (restored_textured_hash != expected_textured_hash) {
        std::fprintf(stderr, "subpixel fidelity: textured legacy restore mismatch\n");
        return 1;
    }

    return 0;
}
