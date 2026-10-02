#pragma once

#include <cstddef>
#include <cstdint>

namespace noctis {

enum class UpscaleMode : std::uint8_t {
    crisp_pixel,     // 1x nearest-neighbor point sampling (authentic chunky pixels)
    edge_scale2x,    // Deterministic Scale2x / EPX edge-directed (smooth diagonals, palette-exact)
    smooth_bilinear  // Hardware bilinear filtering across viewport
};

// Cycle to next upscale mode: crisp_pixel -> edge_scale2x -> smooth_bilinear -> crisp_pixel
UpscaleMode cycle_upscale_mode(UpscaleMode current);

// User-facing name for HUD status notifications
const char *upscale_mode_name(UpscaleMode mode);

// Global upscale mode state
UpscaleMode get_upscale_mode();
void set_upscale_mode(UpscaleMode mode);

// Deterministic Scale2x / EPX algorithm for 32-bit RGBA pixels.
// Expands (width, height) to (2*width, 2*height).
// Preserves exact source colors (zero color bleed, zero intermediate blending).
// Smooths diagonal edges and corners deterministically.
void scale2x_rgba(const std::uint32_t *src, int width, int height, std::uint32_t *dst);

// Deterministic Scale2x / EPX algorithm for 8-bit indexed pixels.
// Expands (width, height) to (2*width, 2*height).
void scale2x_indexed(const std::uint8_t *src, int width, int height, std::uint8_t *dst);

} // namespace noctis
