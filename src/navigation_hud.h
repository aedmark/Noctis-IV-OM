#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace noctis {

struct CompassHeading {
    double degrees{0.0};
    std::string_view cardinal; // "N", "NE", "E", "SE", "S", "SW", "W", "NW"
};

[[nodiscard]] CompassHeading compute_heading(float user_beta);

struct SurfaceCoordinates {
    double latitude_deg{0.0};
    double longitude_deg{0.0};
    double altitude_agl_m{0.0};
    double elevation_msl_m{0.0};
};

[[nodiscard]] SurfaceCoordinates compute_surface_coordinates(
    std::int16_t landing_pt_lon,
    std::int16_t landing_pt_lat,
    double pos_x, double pos_y, double pos_z,
    double ground_y,
    double origin_x, double origin_z);

struct LanderBeacon {
    double distance_m{0.0};
    double relative_bearing_deg{0.0};
    std::string_view direction_arrow; // "[^]", "[>]", "[v]", "[<]", etc.
    bool is_docked{false};
};

[[nodiscard]] LanderBeacon compute_lander_beacon(
    double player_x, double player_z,
    float user_beta,
    double capsule_x, double capsule_z);

enum class VisorHudMode : std::uint8_t {
    standard,
    explorer_telemetry,
    minimal,
};

[[nodiscard]] VisorHudMode cycle_visor_hud_mode(VisorHudMode mode);
[[nodiscard]] const char *visor_hud_mode_name(VisorHudMode mode);

VisorHudMode get_visor_hud_mode();
void set_visor_hud_mode(VisorHudMode mode);

} // namespace noctis
