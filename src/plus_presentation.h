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
void apply_suit_torch(std::uint8_t *framebuffer, std::int32_t width, std::int32_t height, bool torch_active);
void apply_suit_torch_rgba(std::uint8_t *rgba, const std::uint8_t *adapted,
                           std::int32_t width, std::int32_t height);
void draw_surface_status_text(std::uint8_t *framebuffer, std::int32_t width, std::int32_t height,
                              const char *text);

const std::vector<std::string_view> &plus_help_lines(bool surface);
const std::vector<std::string> plus_visual_menu_lines(bool draw_hud,
                                                       std::int8_t lens_flare_mode,
                                                       bool seamless_border,
                                                       int timewarp_multiplier = 0,
                                                       int upscale_mode = 0,
                                                       bool crt_shader = false,
                                                       bool subpixel_fidelity = false,
                                                       bool show_advanced_fx = false,
                                                       int internal_res_mode = 0,
                                                       int draw_distance_mode = 0,
                                                       int texture_filter_mode = 0);
const std::vector<std::string> plus_audio_menu_lines(int selected_category,
                                                     float master_vol,
                                                     float cabin_vol,
                                                     float propulsion_vol,
                                                     float weather_vol,
                                                     float foley_vol,
                                                     bool muted);
const std::vector<std::string> plus_movie_menu_lines(std::uint16_t deck,
                                                      std::uint16_t cadence,
                                                      bool black_flash,
                                                      bool occupied,
                                                      bool recording,
                                                      bool paused,
                                                      double captured_fps);
const std::vector<std::string> plus_controls_menu_lines(bool invert_y,
                                                        float sensitivity,
                                                        int mouselook_mode,
                                                        std::string_view forward_key,
                                                        std::string_view backward_key,
                                                        std::string_view left_key,
                                                        std::string_view right_key,
                                                        bool gamepad_connected = false,
                                                        std::string_view gamepad_name = {},
                                                        bool rumble_enabled = true);

} // namespace noctis
