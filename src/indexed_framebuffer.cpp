#include "indexed_framebuffer.h"

#include <algorithm>

namespace noctis {

void apply_vga_palette(const std::uint8_t *source, std::uint8_t *working_palette,
                       std::uint8_t *current_palette, std::uint16_t first_color,
                       std::uint16_t color_count, std::int8_t red_filter,
                       std::int8_t green_filter, std::int8_t blue_filter) {
    const std::size_t first = static_cast<std::size_t>(first_color) * palette_channels;
    const std::size_t count = static_cast<std::size_t>(color_count) * palette_channels;
    std::copy_n(source, count, working_palette + first);

    const std::int16_t filters[palette_channels] = {red_filter, green_filter, blue_filter};
    for (std::size_t offset = 0; offset < count; ++offset) {
        const auto channel = offset % palette_channels;
        std::uint16_t value = working_palette[first + offset];
        value = static_cast<std::uint16_t>(value * filters[channel]);
        value /= 63;
        working_palette[first + offset] = static_cast<std::uint8_t>(std::min<std::uint16_t>(value, 63));
    }

    // The original VGA routine rewrote DAC entries zero through the end of
    // the selected range, not only the selected entries.
    std::copy_n(working_palette, first + count, current_palette);
}

void expand_indexed_rgba(const std::uint8_t *indices, std::size_t pixel_count,
                         const std::uint8_t *palette, std::uint8_t *rgba) {
    for (std::size_t pixel = 0; pixel < pixel_count; ++pixel) {
        const std::size_t palette_offset = static_cast<std::size_t>(indices[pixel]) * palette_channels;
        const std::size_t rgba_offset = pixel * 4;
        rgba[rgba_offset + 0] = static_cast<std::uint8_t>(palette[palette_offset + 0] * 4u);
        rgba[rgba_offset + 1] = static_cast<std::uint8_t>(palette[palette_offset + 1] * 4u);
        rgba[rgba_offset + 2] = static_cast<std::uint8_t>(palette[palette_offset + 2] * 4u);
        rgba[rgba_offset + 3] = 255;
    }
}

} // namespace noctis
