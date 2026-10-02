#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

std::string format_radius(float centidyams);
std::int8_t cycle_lens_flare_mode(std::int8_t mode);
bool lens_flare_on_hud(bool call_requests_hud_flare, std::int8_t mode);
std::uint16_t visible_surface_objects(std::uint16_t count, std::int32_t depth);

const std::vector<std::string_view> &plus_help_lines(bool surface);
const std::vector<std::string> plus_visual_menu_lines(bool draw_hud,
                                                       std::int8_t lens_flare_mode,
                                                       bool seamless_border);
const std::vector<std::string> plus_movie_menu_lines(std::uint16_t deck,
                                                      std::uint16_t cadence,
                                                      bool black_flash,
                                                      bool occupied,
                                                      bool recording,
                                                      bool paused,
                                                      double captured_fps);

} // namespace noctis
