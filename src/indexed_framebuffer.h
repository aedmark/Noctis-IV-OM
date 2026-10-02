#pragma once

#include <cstddef>
#include <cstdint>

namespace noctis {

inline constexpr std::size_t palette_entries = 256;
inline constexpr std::size_t palette_channels = 3;
inline constexpr std::size_t palette_bytes = palette_entries * palette_channels;

void apply_vga_palette(const std::uint8_t *source, std::uint8_t *working_palette,
                       std::uint8_t *current_palette, std::uint16_t first_color,
                       std::uint16_t color_count, std::int8_t red_filter,
                       std::int8_t green_filter, std::int8_t blue_filter);

void expand_indexed_rgba(const std::uint8_t *indices, std::size_t pixel_count,
                         const std::uint8_t *palette, std::uint8_t *rgba);

} // namespace noctis
