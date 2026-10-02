#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace noctis {

struct PanoramaResult {
    bool ok{};
    std::string message;
};

inline constexpr std::size_t indexed_bmp_header_size = 54;
inline constexpr std::size_t indexed_bmp_pixel_offset = indexed_bmp_header_size + 1024;

// Rewrites the size-dependent fields of a 54-byte, 8-bit BMP header (bfSize,
// bfOffBits, biWidth, biHeight, biSizeImage) and leaves every other byte alone.
void normalize_indexed_bmp_header(std::uint8_t *header, std::uint32_t width, std::uint32_t height);

PanoramaResult compose_panorama(const std::array<std::filesystem::path, 3> &frames,
                                const std::filesystem::path &destination);

} // namespace noctis
