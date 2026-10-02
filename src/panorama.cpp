#include "panorama.h"

#include "atomic_file.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

namespace noctis {
namespace {
constexpr std::size_t source_width = 320;
constexpr std::size_t output_width = 916;
constexpr std::size_t height = 200;
constexpr std::size_t pixel_offset = indexed_bmp_pixel_offset;
constexpr std::size_t source_size = pixel_offset + source_width * height;
constexpr std::size_t output_size = pixel_offset + output_width * height;

void write_u32(std::uint8_t *bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
}

bool read_frame(const std::filesystem::path &path, std::vector<std::uint8_t> &bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    bytes.assign(std::istreambuf_iterator<char>(input), {});
    return !input.bad() && bytes.size() == source_size && bytes[0] == 'B' && bytes[1] == 'M';
}
} // namespace

void normalize_indexed_bmp_header(std::uint8_t *header, std::uint32_t width, std::uint32_t height) {
    const auto image_size = width * height;
    write_u32(header, 2, static_cast<std::uint32_t>(indexed_bmp_pixel_offset + image_size));
    write_u32(header, 10, static_cast<std::uint32_t>(indexed_bmp_pixel_offset));
    write_u32(header, 18, width);
    write_u32(header, 22, height);
    write_u32(header, 34, image_size);
}

PanoramaResult compose_panorama(const std::array<std::filesystem::path, 3> &frames,
                                const std::filesystem::path &destination) {
    std::array<std::vector<std::uint8_t>, 3> source;
    for (std::size_t index = 0; index < source.size(); ++index) {
        if (!read_frame(frames[index], source[index])) return {false, "panorama source is missing or malformed"};
    }

    std::vector<std::uint8_t> output(output_size, 0);
    std::copy_n(source[0].begin(), pixel_offset, output.begin());
    normalize_indexed_bmp_header(output.data(), output_width, height);
    for (std::size_t row = 0; row < height; ++row) {
        const auto source_row = pixel_offset + row * source_width;
        const auto output_row = pixel_offset + row * output_width;
        std::copy_n(source[1].begin() + source_row, 309, output.begin() + output_row);
        std::copy_n(source[0].begin() + source_row + 10, 299, output.begin() + output_row + 309);
        std::copy_n(source[2].begin() + source_row + 10, 308, output.begin() + output_row + 608);
    }

    auto temporary = destination;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file || !file.write(reinterpret_cast<const char *>(output.data()), output.size()) || !file.flush()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return {false, "panorama temporary could not be written"};
        }
    }
    if (!atomic_replace(temporary, destination)) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return {false, "panorama could not be published"};
    }
    return {true, {}};
}

} // namespace noctis
