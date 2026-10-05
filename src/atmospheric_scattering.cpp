#include "atmospheric_scattering.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noctis {

namespace {

AtmosphericScatteringMode g_atmospheric_scattering_mode = AtmosphericScatteringMode::authentic;

constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadToDeg = 180.0f / kPi;
constexpr float kDegToRad = kPi / 180.0f;

} // namespace

AtmosphericScatteringMode get_atmospheric_scattering_mode() {
    return g_atmospheric_scattering_mode;
}

void set_atmospheric_scattering_mode(AtmosphericScatteringMode mode) {
    g_atmospheric_scattering_mode = mode;
}

AtmosphericScatteringMode cycle_atmospheric_scattering_mode(AtmosphericScatteringMode current) {
    switch (current) {
        case AtmosphericScatteringMode::authentic: return AtmosphericScatteringMode::realistic;
        case AtmosphericScatteringMode::realistic: return AtmosphericScatteringMode::vibrant;
        case AtmosphericScatteringMode::vibrant:   return AtmosphericScatteringMode::authentic;
    }
    return AtmosphericScatteringMode::authentic;
}

AtmosphericScatteringMode cycle_atmospheric_scattering_mode() {
    auto next = cycle_atmospheric_scattering_mode(g_atmospheric_scattering_mode);
    set_atmospheric_scattering_mode(next);
    return next;
}

const char *atmospheric_scattering_mode_name(AtmosphericScatteringMode mode) {
    switch (mode) {
        case AtmosphericScatteringMode::authentic: return "SCATTERING: AUTHENTIC";
        case AtmosphericScatteringMode::realistic: return "SCATTERING: REALISTIC";
        case AtmosphericScatteringMode::vibrant:   return "SCATTERING: VIBRANT";
    }
    return "SCATTERING: AUTHENTIC";
}

std::optional<AtmosphericScatteringMode> parse_atmospheric_scattering_mode(std::string_view str) {
    if (str == "authentic" || str == "0" || str == "off" || str == "legacy") {
        return AtmosphericScatteringMode::authentic;
    }
    if (str == "realistic" || str == "1" || str == "on" || str == "standard") {
        return AtmosphericScatteringMode::realistic;
    }
    if (str == "vibrant" || str == "2" || str == "enhanced") {
        return AtmosphericScatteringMode::vibrant;
    }
    return std::nullopt;
}

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
    float rainy) {
    TwilightAtmosphereData data;

    // Solar elevation angle relative to horizon in degrees (-90 to +90)
    // In dayzone (nightzone == 0), crepzone is distance from terminator where crepzone * 0.7826 is elevation.
    // In nightzone (nightzone == 1), the sun is below the horizon by crepzone * 0.7826 degrees.
    const float raw_elevation = static_cast<float>(crepzone) * 0.7826f;
    data.solar_elevation_deg = nightzone ? -raw_elevation : raw_elevation;
    data.solar_zenith_deg = 90.0f - data.solar_elevation_deg;

    // Calculate azimuth of the sun in panoramic sky coordinates (0 to 360 degrees)
    // Noctis coordinate convention:
    // When looking at yaw beta, screen center maps to cylindrical column -(beta % 360).
    // The sun is located along the horizontal plane vector (-sun_x, sun_z).
    float raw_azimuth_deg = 0.0f;
    if (std::fabs(sun_x) > 1e-4f || std::fabs(sun_z) > 1e-4f) {
        float angle = std::atan2(-sun_x, sun_z) * kRadToDeg;
        raw_azimuth_deg = std::fmod(360.0f - angle + 360.0f, 360.0f);
    } else {
        raw_azimuth_deg = (sun_x_factor >= 0) ? 90.0f : 270.0f;
    }
    data.sun_azimuth_deg = raw_azimuth_deg;

    // Atmospheric density scaling:
    // Dense atmospheres (pp_pressure > 1.0) scatter more light and extend twilight duration.
    // Thin atmospheres (pp_pressure < 1.0) have rapid transitions.
    const float density = std::clamp(pp_pressure, 0.1f, 5.0f);
    const float density_factor = std::clamp(std::sqrt(density), 0.5f, 2.0f);
    const float elev = data.solar_elevation_deg;

    // Twilight phases:
    // - elev > 15.0: Full day
    // - 2.0 < elev <= 15.0: Golden hour
    // - -4.0 <= elev <= 2.0: Sunset / Sunrise / Civil twilight (peak glow arch)
    // - -10.0 <= elev < -4.0: Nautical twilight
    // - -18.0 <= elev < -10.0: Astronomical twilight
    // - elev < -18.0: Full night
    const float twilight_limit = -18.0f * density_factor;

    if (elev > 15.0f) {
        data.twilight_factor = 0.0f;
        data.day_factor = 1.0f;
        data.night_factor = 0.0f;
        data.sunset_warmth = 0.0f;
        data.horizon_glow_peak = 0.0f;
    } else if (elev > 3.0f) {
        // Golden hour: sun approaching horizon
        float t = (15.0f - elev) / 12.0f;
        data.twilight_factor = t * 0.85f;
        data.day_factor = 1.0f - t * 0.30f;
        data.night_factor = 0.0f;
        data.sunset_warmth = t * 0.90f;
        data.horizon_glow_peak = t * 0.70f;
    } else if (elev >= -2.0f) {
        // Sunset / Sunrise: sun disk on horizon
        float t = (3.0f - elev) / 5.0f; // 0.0 at +3 deg, 1.0 at -2 deg
        data.twilight_factor = 0.85f + t * 0.15f; // Peaks at 1.0
        data.day_factor = 0.70f * (1.0f - t);
        data.night_factor = t * 0.15f;
        data.sunset_warmth = 1.0f;
        data.horizon_glow_peak = 0.70f + t * 0.30f; // Peaks at 1.0
    } else if (elev >= -6.0f) {
        // Civil twilight: sun just below horizon, peak atmospheric glow
        float t = (-elev - 2.0f) / 4.0f;
        data.twilight_factor = 1.0f - t * 0.20f; // 1.0 to 0.80
        data.day_factor = 0.0f;
        data.night_factor = 0.15f + t * 0.25f; // 0.15 to 0.40
        data.sunset_warmth = 1.0f;
        data.horizon_glow_peak = 1.0f - t * 0.25f;
    } else if (elev >= -12.0f) {
        // Nautical twilight: Transitioning into deep indigo/violet
        float t = (-elev - 6.0f) / 6.0f;
        data.twilight_factor = 0.80f * (1.0f - t);
        data.day_factor = 0.0f;
        data.night_factor = 0.40f + t * 0.40f; // 0.40 to 0.80
        data.sunset_warmth = 1.0f - t * 0.50f;
        data.horizon_glow_peak = 0.75f * (1.0f - t);
    } else if (elev >= twilight_limit) {
        // Astronomical twilight: Fading into stellar night
        float span = std::max(1.0f, -twilight_limit - 10.0f);
        float t = std::clamp((-elev - 10.0f) / span, 0.0f, 1.0f);
        data.twilight_factor = 0.35f * (1.0f - t);
        data.day_factor = 0.0f;
        data.night_factor = 0.75f + t * 0.25f;
        data.sunset_warmth = 0.55f * (1.0f - t);
        data.horizon_glow_peak = 0.40f * (1.0f - t);
    } else {
        // Full night
        data.twilight_factor = 0.0f;
        data.day_factor = 0.0f;
        data.night_factor = 1.0f;
        data.sunset_warmth = 0.0f;
        data.horizon_glow_peak = 0.0f;
    }

    data.is_twilight = (data.twilight_factor > 0.05f) || (data.horizon_glow_peak > 0.05f);
    return data;
}

void generate_twilight_sky_map(
    uint8_t *s_background,
    uint16_t total_bytes,
    uint16_t lines_to_horizon,
    uint8_t sky_brightness,
    const TwilightAtmosphereData &twilight,
    AtmosphericScatteringMode mode,
    float rainy) {
    if (!s_background || lines_to_horizon == 0 || total_bytes == 0) {
        return;
    }

    const float intensity_mult = (mode == AtmosphericScatteringMode::vibrant) ? 1.35f : 1.0f;
    const float cloud_atten = 1.0f / (1.0f + 0.30f * rainy);
    const float sigma_mie = 55.0f;
    const float two_sigma_mie_sq = 2.0f * sigma_mie * sigma_mie;
    const float sigma_anti = 65.0f;
    const float two_sigma_anti_sq = 2.0f * sigma_anti * sigma_anti;

    uint16_t temp_vptr = 0;
    for (uint16_t cpos = 0; cpos < lines_to_horizon; ++cpos) {
        // y_norm: 0.0 at zenith, 1.0 at horizon
        const float y_norm = static_cast<float>(cpos) / static_cast<float>(lines_to_horizon);

        // Forward Mie vertical profile: increases sharply near horizon
        const float v_mie = std::pow(y_norm, 2.3f);

        // Anti-solar Belt of Venus: peaks slightly above horizon (around y_norm = 0.80)
        const float dy_anti = y_norm - 0.80f;
        const float v_anti = std::exp(-(dy_anti * dy_anti) / 0.035f);

        // Base daylight elevation falloff
        const float v_day = std::pow(y_norm, 1.35f);

        // Baseline night/dusk gradient
        const float v_night = y_norm * 0.25f * (1.0f - twilight.night_factor);

        for (uint16_t mpul = 0; mpul < 360; ++mpul) {
            if (temp_vptr >= total_bytes) {
                break;
            }

            // Calculate azimuth offset relative to the sun
            float delta_phi = std::fabs(static_cast<float>(mpul) - twilight.sun_azimuth_deg);
            if (delta_phi > 180.0f) {
                delta_phi = 360.0f - delta_phi;
            }

            // Forward Mie scattering arch
            const float f_mie = std::exp(-(delta_phi * delta_phi) / two_sigma_mie_sq);

            // Anti-solar Belt of Venus arch (180 degrees away from the sun)
            const float delta_phi_anti = std::fabs(180.0f - delta_phi);
            const float f_anti = std::exp(-(delta_phi_anti * delta_phi_anti) / two_sigma_anti_sq);

            // Brightness components:
            const float day_contrib = static_cast<float>(sky_brightness) * v_day * twilight.day_factor;
            const float night_contrib = static_cast<float>(sky_brightness) * v_night;
            const float glow_contrib = 54.0f * twilight.horizon_glow_peak * f_mie * v_mie * intensity_mult;
            const float anti_contrib = 18.0f * twilight.twilight_factor * f_anti * v_anti * intensity_mult;

            // Existing pattern factor (clouds / nebulae)
            const float orig_val = static_cast<float>(s_background[temp_vptr]);
            const float cloud_mod = (sky_brightness > 0)
                                      ? (0.75f + 0.25f * (orig_val / static_cast<float>(sky_brightness)))
                                      : 1.0f;

            float combined = day_contrib + night_contrib + (glow_contrib + anti_contrib) * cloud_atten;
            combined *= cloud_mod;

            // Clamp into valid 6-bit palette index brightness [0, 63]
            s_background[temp_vptr] = static_cast<uint8_t>(std::clamp(std::round(combined), 0.0f, 63.0f));
            ++temp_vptr;
        }
    }
}

void apply_twilight_palette_filters(
    float &fr_ground, float &fg_ground, float &fb_ground,
    float &fr_sky,    float &fg_sky,    float &fb_sky,
    float &fr_horiz,  float &fg_horiz,  float &fb_horiz,
    float &fr_veg,    float &fg_veg,    float &fb_veg,
    int16_t planet_type,
    const TwilightAtmosphereData &twilight,
    AtmosphericScatteringMode mode,
    float rainy) {
    if (twilight.twilight_factor < 0.005f && twilight.night_factor >= 0.99f) {
        return;
    }

    const float sat_boost = (mode == AtmosphericScatteringMode::vibrant) ? 1.35f : 1.0f;

    // 1. Horizon scattering color target
    // Rayleigh and Mie forward scattering at the horizon:
    float target_horiz_r = 1.35f;
    float target_horiz_g = 0.65f;
    float target_horiz_b = 0.22f;

    if (planet_type == 2) {
        // Venusian / thick atmosphere: deep bronze / copper
        target_horiz_r = 1.40f;
        target_horiz_g = 0.70f;
        target_horiz_b = 0.15f;
    } else if (planet_type == 5) {
        // Thin atmosphere: pastel peach / soft lavender
        target_horiz_r = 1.15f;
        target_horiz_g = 0.80f;
        target_horiz_b = 0.50f;
    }

    // Deepen twilight hue as sun sinks into civil and nautical twilight:
    // Golden amber (high sun) -> Fiery crimson -> Deep magenta/violet
    if (twilight.solar_elevation_deg < 0.0f) {
        float sink = std::clamp(-twilight.solar_elevation_deg / 10.0f, 0.0f, 1.0f);
        target_horiz_r = std::lerp(target_horiz_r, 0.60f, sink);
        target_horiz_g = std::lerp(target_horiz_g, 0.25f, sink);
        target_horiz_b = std::lerp(target_horiz_b, 0.55f, sink);
    }

    if (mode == AtmosphericScatteringMode::vibrant) {
        target_horiz_r *= sat_boost;
        target_horiz_g *= 0.90f; // Increase contrast
        target_horiz_b *= (target_horiz_b > 0.4f ? 1.2f : 0.8f);
    }

    // Blend horizon color smoothly between daytime color and twilight color
    fr_horiz = std::lerp(fr_horiz * twilight.day_factor, target_horiz_r, twilight.twilight_factor);
    fg_horiz = std::lerp(fg_horiz * twilight.day_factor, target_horiz_g, twilight.twilight_factor);
    fb_horiz = std::lerp(fb_horiz * twilight.day_factor, target_horiz_b, twilight.twilight_factor);

    // Ensure horizon colors maintain minimum visibility
    fr_horiz = std::max(fr_horiz, 0.08f);
    fg_horiz = std::max(fg_horiz, 0.08f);
    fb_horiz = std::max(fb_horiz, 0.10f);

    // 2. Sky zenith color target
    // Transitions from daytime sky -> deep twilight indigo/ultramarine -> dark night
    float twilight_sky_r = 0.15f;
    float twilight_sky_g = 0.20f;
    float twilight_sky_b = 0.55f;

    fr_sky = std::lerp(fr_sky * twilight.day_factor, twilight_sky_r, twilight.twilight_factor);
    fg_sky = std::lerp(fg_sky * twilight.day_factor, twilight_sky_g, twilight.twilight_factor);
    fb_sky = std::lerp(fb_sky * twilight.day_factor, twilight_sky_b, twilight.twilight_factor);

    // 3. Ambient ground illumination
    // Continuous lighting falloff from day through sunset to night
    const float ambient = 0.30f + 0.70f * twilight.day_factor + 0.18f * twilight.twilight_factor;
    fr_ground *= ambient;
    fg_ground *= ambient;
    fb_ground *= ambient;

    fr_veg *= ambient;
    fg_veg *= ambient;
    fb_veg *= ambient;
}

bool is_sun_limb_visible_at_twilight(int16_t crepzone, int16_t nightzone) {
    if (!nightzone) {
        return true;
    }
    // In nightzone, sun is below horizon. The angular radius of the sun disk
    // is ~2-3 degrees. Allow the upper limb to be visible during early civil twilight.
    return (crepzone <= 3);
}

} // namespace noctis
