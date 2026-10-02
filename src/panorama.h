#pragma once

#include <array>
#include <filesystem>
#include <string>

namespace noctis {

struct PanoramaResult {
    bool ok{};
    std::string message;
};

PanoramaResult compose_panorama(const std::array<std::filesystem::path, 3> &frames,
                                const std::filesystem::path &destination);

} // namespace noctis
