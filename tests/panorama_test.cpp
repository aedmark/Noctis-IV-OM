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

    bool ok = true;
    auto result = noctis::compose_panorama(paths, output);
    auto bytes = read_file(output);
    ok &= require(result.ok && bytes.size() == 54 + 1024 + 916 * 200, "first panorama failed");
    ok &= require(bytes[1078] == 22 && bytes[1078 + 308] == 22
                      && bytes[1078 + 309] == 11 && bytes[1078 + 607] == 11
                      && bytes[1078 + 608] == 33 && bytes[1078 + 915] == 33,
                  "panorama seam layout changed");
    ok &= require(read_file(reserved) == std::vector<std::uint8_t>({1, 2, 3, 4}),
                  "reserved numbered snapshot was overwritten");

    const auto first = bytes;
    result = noctis::compose_panorama(paths, output);
    ok &= require(result.ok && read_file(output) == first, "consecutive panorama was not stable");
    ok &= require(!std::filesystem::exists(output.string() + ".tmp"), "panorama temporary survived");
    std::filesystem::remove_all(root, ignored);
    return ok ? 0 : 1;
}
