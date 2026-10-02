#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace noctis {

inline constexpr std::uint16_t native_save_version = 1;

struct NativeSaveState {
    std::int8_t nsync = 1;
    std::int8_t anti_rad = 1;
    std::int8_t pl_search = 0;
    std::int8_t field_amplificator = 0;
    std::int8_t ilight = 63;
    std::int8_t ilightv = 1;
    std::int8_t charge = 120;
    std::int8_t revcontrols = 0;
    std::int8_t ap_targetting = 0;
    std::int8_t ap_targetted = 0;
    std::int8_t ip_targetting = 0;
    std::int8_t ip_targetted = -1;
    std::int8_t ip_reaching = 0;
    std::int8_t ip_reached = 0;
    std::int8_t ap_target_spin = 0;
    std::int8_t ap_target_r = 0;
    std::int8_t ap_target_g = 0;
    std::int8_t ap_target_b = 0;
    std::int8_t nearstar_spin = 0;
    std::int8_t nearstar_r = 0;
    std::int8_t nearstar_g = 0;
    std::int8_t nearstar_b = 0;
    std::int8_t gburst = 0;
    std::int8_t menusalwayson = 1;
    std::int8_t depolarize = 0;
    std::int16_t sys = 4;
    std::int16_t pwr = 20000;
    std::int16_t dev_page = 0;
    std::int16_t ap_target_class = 0;
    std::int16_t f_ray_elapsed = 0;
    std::int16_t nearstar_class = 0;
    std::int16_t nearstar_nop = 0;
    float pos_x = 0;
    float pos_y = 0;
    float pos_z = -500;
    float user_alfa = 0;
    float user_beta = 0;
    float navigation_beta = 0;
    float ap_target_ray = 1000;
    float nearstar_ray = 1000;
    double dzat_x = 3797120;
    double dzat_y = -4352112;
    double dzat_z = -925018;
    double ap_target_x = 0;
    double ap_target_y = 1E8;
    double ap_target_z = 0;
    double nearstar_x = 0;
    double nearstar_y = 1E8;
    double nearstar_z = 0;
    double helptime = 0;
    double ip_target_initial_d = 1E8;
    double requested_approach_coefficient = 1;
    double current_approach_coefficient = 1;
    double reaction_time = 0.01;
    std::array<std::int8_t, 11> fcs_status{'S', 'T', 'A', 'N', 'D', 'B', 'Y'};
    std::int16_t fcs_status_delay = 0;
    std::int16_t psys = 4;
    double ap_target_initial_d = 1E8;
    double requested_vimana_coefficient = 1;
    double current_vimana_coefficient = 1;
    double vimana_reaction_time = 0.01;
    std::int8_t lithium_collector = 0;
    std::int8_t autoscreenoff = 0;
    std::int8_t ap_reached = 0;
    std::int16_t lifter = 0;
    double secs = 0;
    std::int8_t data = 0;
    std::int8_t surlight = 16;
    std::int8_t gnc_pos = 0;
    std::int32_t goesfile_pos = 0;
    std::array<char, 120> goesnet_command{'_'};
    std::uint32_t last_snapshot = UINT32_MAX;
    std::int8_t option_mouse_look = 0;
    std::int16_t roof_speed = 0;
    std::int8_t hud_closed = 1;
    std::int8_t draw_hud = 1;
    std::int8_t lens_flare_mode = 0;
    std::int8_t seamless_border = 0;
};

struct SurfaceSaveState {
    std::int16_t landing_longitude = 0;
    std::int16_t landing_latitude = 60;
    std::int32_t atl_x = 0;
    std::int32_t atl_z = 0;
    std::int32_t atl_x2 = 0;
    std::int32_t atl_z2 = 0;
    float pos_x = 0;
    float pos_y = 0;
    float pos_z = 0;
    float user_alfa = 0;
    float user_beta = 0;
    std::int16_t openhuddelta = 0;
    std::int16_t openhudcount = 180;
    std::int8_t hud_rtl_closed = 1;
};

enum class NativeSaveStatus {
    ok,
    not_found,
    invalid,
    unsupported_version,
    io_error,
};

struct NativeSaveResult {
    NativeSaveStatus status;
    std::string message;
};

std::vector<std::uint8_t> encode_native_save(const NativeSaveState &state);
NativeSaveResult decode_native_save(std::span<const std::uint8_t> bytes, NativeSaveState &state);
NativeSaveResult load_native_save(const std::filesystem::path &path, NativeSaveState &state);
NativeSaveResult save_native_save(const std::filesystem::path &path, const NativeSaveState &state);
std::vector<std::uint8_t> encode_surface_save(const SurfaceSaveState &state);
NativeSaveResult decode_surface_save(std::span<const std::uint8_t> bytes, SurfaceSaveState &state);
NativeSaveResult load_surface_save(const std::filesystem::path &path, SurfaceSaveState &state);
NativeSaveResult save_surface_save(const std::filesystem::path &path, const SurfaceSaveState &state);

} // namespace noctis
