#include "navigation_hud.h"

#include <algorithm>
#include <cmath>

namespace noctis {
namespace {

constexpr double pi_val = 3.14159265358979323846;
VisorHudMode s_visor_hud_mode = VisorHudMode::explorer_telemetry;

} // namespace

CompassHeading compute_heading(float user_beta) {
    double ccom = 360.0 - static_cast<double>(user_beta);
    while (ccom >= 360.0) ccom -= 360.0;
    while (ccom < 0.0) ccom += 360.0;

    if (ccom >= 337.5 || ccom < 22.5) return {ccom, "N"};
    if (ccom >= 22.5 && ccom < 67.5) return {ccom, "NE"};
    if (ccom >= 67.5 && ccom < 112.5) return {ccom, "E"};
    if (ccom >= 112.5 && ccom < 157.5) return {ccom, "SE"};
    if (ccom >= 157.5 && ccom < 202.5) return {ccom, "S"};
    if (ccom >= 202.5 && ccom < 247.5) return {ccom, "SW"};
    if (ccom >= 247.5 && ccom < 292.5) return {ccom, "W"};
    return {ccom, "NW"};
}

SurfaceCoordinates compute_surface_coordinates(
    std::int16_t landing_pt_lon,
    std::int16_t landing_pt_lat,
    double pos_x, double pos_y, double pos_z,
    double ground_y,
    double origin_x, double origin_z) {
    const double base_lat = (static_cast<double>(landing_pt_lat) - 60.0) * 1.5;
    const double base_lon = static_cast<double>(landing_pt_lon);

    const double dx = pos_x - origin_x;
    const double dz = pos_z - origin_z;

    double lat = base_lat + (dz / 163840.0) * 1.5;
    double lon = base_lon + (dx / 163840.0);

    while (lon >= 360.0) lon -= 360.0;
    while (lon < 0.0) lon += 360.0;
    lat = std::clamp(lat, -90.0, 90.0);

    const double alt_agl = std::max(0.0, (pos_y - ground_y) / 20.0);
    const double elev_msl = pos_y / 20.0;

    return {lat, lon, alt_agl, elev_msl};
}

LanderBeacon compute_lander_beacon(
    double player_x, double player_z,
    float user_beta,
    double capsule_x, double capsule_z) {
    const double dx = capsule_x - player_x;
    const double dz = capsule_z - player_z;
    const double dist_units = std::sqrt(dx * dx + dz * dz);
    const double dist_m = dist_units / 20.0;
    const bool is_docked = (dist_units < 1600.0);

    double ccom = 360.0 - static_cast<double>(user_beta);
    while (ccom >= 360.0) ccom -= 360.0;
    while (ccom < 0.0) ccom += 360.0;

    double world_bearing = std::atan2(dx, dz) * (180.0 / pi_val);
    if (world_bearing < 0.0) world_bearing += 360.0;

    double rel = world_bearing - ccom;
    while (rel > 180.0) rel -= 360.0;
    while (rel < -180.0) rel += 360.0;

    std::string_view arrow = "[^]";
    if (std::abs(rel) <= 22.5) {
        arrow = "[^]";
    } else if (rel > 22.5 && rel <= 67.5) {
        arrow = "[^>]";
    } else if (rel > 67.5 && rel <= 112.5) {
        arrow = "[>]";
    } else if (rel > 112.5 && rel <= 157.5) {
        arrow = "[v>]";
    } else if (std::abs(rel) > 157.5) {
        arrow = "[v]";
    } else if (rel < -22.5 && rel >= -67.5) {
        arrow = "[<^]";
    } else if (rel < -67.5 && rel >= -112.5) {
        arrow = "[<]";
    } else {
        arrow = "[<v]";
    }

    return {dist_m, rel, arrow, is_docked};
}

VisorHudMode cycle_visor_hud_mode(VisorHudMode mode) {
    switch (mode) {
    case VisorHudMode::standard: return VisorHudMode::explorer_telemetry;
    case VisorHudMode::explorer_telemetry: return VisorHudMode::minimal;
    case VisorHudMode::minimal: return VisorHudMode::standard;
    }
    return VisorHudMode::explorer_telemetry;
}

const char *visor_hud_mode_name(VisorHudMode mode) {
    switch (mode) {
    case VisorHudMode::standard: return "VISOR: STANDARD";
    case VisorHudMode::explorer_telemetry: return "VISOR: EXPLORER HUD";
    case VisorHudMode::minimal: return "VISOR: MINIMAL";
    }
    return "VISOR: EXPLORER HUD";
}

VisorHudMode get_visor_hud_mode() {
    return s_visor_hud_mode;
}

void set_visor_hud_mode(VisorHudMode mode) {
    s_visor_hud_mode = mode;
}

} // namespace noctis
