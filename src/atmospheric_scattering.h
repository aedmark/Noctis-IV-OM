#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

namespace noctis {

enum class AtmosphericScatteringMode : std::uint8_t {
    authentic, // 0: Authentic legacy uniform sky and binary day/night step
    realistic, // 1: Dynamic multi-stop Rayleigh and Mie scattering twilight gradients
    vibrant    // 2: Enhanced chromatic saturation for dramatic alien sunsets
};

AtmosphericScatteringMode get_atmospheric_scattering_mode();
void set_atmospheric_scattering_mode(AtmosphericScatteringMode mode);
AtmosphericScatteringMode cycle_atmospheric_scattering_mode(AtmosphericScatteringMode current);
AtmosphericScatteringMode cycle_atmospheric_scattering_mode();
const char *atmospheric_scattering_mode_name(AtmosphericScatteringMode mode);
std::optional<AtmosphericScatteringMode> parse_atmospheric_scattering_mode(std::string_view str);

struct TwilightAtmosphereData {
    float solar_elevation_deg = 0.0f; // -90 to +90 degrees relative to horizon
    float solar_zenith_deg = 90.0f;   // 0 to 180 degrees (zenith = 0, horizon = 90, nadir = 180)
    float twilight_factor = 0.0f;     // 0.0 = day or full night, 1.0 = peak twilight
    float day_factor = 1.0f;          // 1.0 = full day, 0.0 = night
    float night_factor = 0.0f;        // 0.0 = day, 1.0 = full night
    float sunset_warmth = 0.0f;       // Red/amber shift factor [0.0, 1.0]
    float horizon_glow_peak = 0.0f;   // Intensity of forward Mie glow arch
    float sun_azimuth_deg = 0.0f;     // 0 to 360 degrees in panoramic sky coordinates
    bool is_twilight = false;         // true if within twilight window
};

TwilightAtmosphereData compute_twilight_data(
    int16_t crepzone,
    int16_t nightzone,
    int16_t sun_x_factor,
    float sun_x,
    float sun_y,
    float sun_z,
    float dsd1,
    float pp_pressure,
    int16_t nearstar_class,
    float rainy);

// Generates the 360x180 panoramic sky texture `s_background` with multi-stop twilight scattering
void generate_twilight_sky_map(
    uint8_t *s_background,
    uint16_t total_bytes,
    uint16_t lines_to_horizon,
    uint8_t sky_brightness,
    const TwilightAtmosphereData &twilight,
    AtmosphericScatteringMode mode,
    float rainy);

// Modulates palette color ramps for sky, horizon, and ground ambient during twilight
void apply_twilight_palette_filters(
    float &fr_ground, float &fg_ground, float &fb_ground,
    float &fr_sky,    float &fg_sky,    float &fb_sky,
    float &fr_horiz,  float &fg_horiz,  float &fb_horiz,
    float &fr_veg,    float &fg_veg,    float &fb_veg,
    int16_t planet_type,
    const TwilightAtmosphereData &twilight,
    AtmosphericScatteringMode mode,
    float rainy);

// Checks if the upper limb of the sun is visible during twilight at the horizon
bool is_sun_limb_visible_at_twilight(int16_t crepzone, int16_t nightzone);

} // namespace noctis
