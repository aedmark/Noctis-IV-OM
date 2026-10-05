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
    std::filesystem::path gallery_path;
    std::filesystem::path bookmarks_path;
    double current_star_id{0.0};
    std::string current_star_name;
    std::int16_t current_star_class{0};
    std::int16_t current_planet_index{-1};
    std::string current_planet_name;
    bool is_on_surface{false};
    double surface_lat{0.0};
    double surface_lon{0.0};
};

GoesResult execute_goes_command(std::string_view console_line, const GoesCommandContext &context);

} // namespace noctis
