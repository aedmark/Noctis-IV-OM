#include "panorama.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::fprintf(stderr, "panorama: %s\n", message);
    return condition;
}

void write_u32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
}

std::vector<std::uint8_t> frame(std::uint8_t value) {
    std::vector<std::uint8_t> bytes(54 + 1024 + 320 * 200, value);
    bytes[0] = 'B'; bytes[1] = 'M';
    write_u32(bytes, 2, bytes.size());
    write_u32(bytes, 10, 1078);
    write_u32(bytes, 14, 40);
    write_u32(bytes, 18, 320);
    write_u32(bytes, 22, 200);
    write_u32(bytes, 34, 320 * 200);
    return bytes;
}

void write_file(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

std::uint32_t read_u32(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) value |= std::uint32_t{bytes[offset++]} << shift;
    return value;
}

// Bytes of the header_bmp template stored in supports.nct, including its stale
// bfSize (0x1C666) and biSizeImage (0x1C230) fields.
constexpr std::array<std::uint8_t, 54> legacy_template{
    0x42, 0x4d, 0x66, 0xc6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x04, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x30, 0xc2, 0x01, 0x00, 0x74, 0x12, 0x00, 0x00, 0x74, 0x12, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

bool check_snapshot_header() {
    std::vector<std::uint8_t> header(legacy_template.begin(), legacy_template.end());
    noctis::normalize_indexed_bmp_header(header.data(), 320, 200);
    bool ok = require(read_u32(header, 2) == 54 + 1024 + 320 * 200, "snapshot bfSize does not match file size");
    ok &= require(read_u32(header, 10) == 1078, "snapshot bfOffBits changed");
    ok &= require(read_u32(header, 18) == 320 && read_u32(header, 22) == 200, "snapshot dimensions changed");
    ok &= require(read_u32(header, 34) == 320 * 200, "snapshot biSizeImage does not match pixel data");
    for (const std::size_t offset : {2, 10, 18, 22, 34}) {
        for (std::size_t byte = 0; byte < 4; ++byte) header[offset + byte] = legacy_template[offset + byte];
    }
    ok &= require(header == std::vector<std::uint8_t>(legacy_template.begin(), legacy_template.end()),
                  "snapshot header normalization touched unrelated fields");
    return ok;
}

std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
}

int main() {
    const auto root = std::filesystem::current_path() / "panorama-test";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directory(root);
    const std::array<std::filesystem::path, 3> paths{
        root / "WIDE9997.BMP", root / "WIDE9998.BMP", root / "WIDE9999.BMP"};
    write_file(paths[0], frame(11));
    write_file(paths[1], frame(22));
    write_file(paths[2], frame(33));
    const auto reserved = root / "SNAP9998.BMP";
    write_file(reserved, {1, 2, 3, 4});
    const auto output = root / "SNAP0001.BMP";

    bool ok = check_snapshot_header();
    auto result = noctis::compose_panorama(paths, output);
    auto bytes = read_file(output);
    ok &= require(result.ok && bytes.size() == 54 + 1024 + 916 * 200, "first panorama failed");
    ok &= require(bytes[1078] == 22 && bytes[1078 + 308] == 22
                      && bytes[1078 + 309] == 11 && bytes[1078 + 607] == 11
                      && bytes[1078 + 608] == 33 && bytes[1078 + 915] == 33,
                  "panorama seam layout changed");
    ok &= require(read_u32(bytes, 2) == bytes.size() && read_u32(bytes, 10) == 1078
                      && read_u32(bytes, 18) == 916 && read_u32(bytes, 22) == 200
                      && read_u32(bytes, 34) == 916 * 200,
                  "panorama header size fields are inconsistent");
    ok &= require(read_file(reserved) == std::vector<std::uint8_t>({1, 2, 3, 4}),
                  "reserved numbered snapshot was overwritten");

    const auto first = bytes;
    result = noctis::compose_panorama(paths, output);
    ok &= require(result.ok && read_file(output) == first, "consecutive panorama was not stable");
    ok &= require(!std::filesystem::exists(output.string() + ".tmp"), "panorama temporary survived");
    std::filesystem::remove_all(root, ignored);
    return ok ? 0 : 1;
}
