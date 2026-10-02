#pragma once

#include "goesnet_protocol.h"

#include <filesystem>
#include <string_view>

namespace noctis {

struct GoesCommandContext {
    std::filesystem::path starmap_path;
    std::filesystem::path guide_path;
    std::filesystem::path export_path;
    double observer_x{};
    double observer_y{};
    double observer_z{};
    double local_star_x{};
    double local_star_y{};
    double local_star_z{};
};

GoesResult execute_goes_command(std::string_view console_line, const GoesCommandContext &context);

} // namespace noctis
