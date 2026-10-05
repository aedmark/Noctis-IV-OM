#include "stellar_coronal_flares.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noctis {

namespace {

CoronalFlaresMode g_coronal_flares_mode = CoronalFlaresMode::authentic;

constexpr float PI = std::numbers::pi_v<float>;

// Helper for angular difference wrapped to [-PI, PI]
inline float angle_diff(float a, float b) {
    float diff = a - b;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return diff;
}

} // namespace

CoronalFlaresMode get_coronal_flares_mode() {
    return g_coronal_flares_mode;
}

void set_coronal_flares_mode(CoronalFlaresMode mode) {
    g_coronal_flares_mode = mode;
}

CoronalFlaresMode cycle_coronal_flares_mode(CoronalFlaresMode current) {
    switch (current) {
        case CoronalFlaresMode::authentic: return CoronalFlaresMode::realistic;
        case CoronalFlaresMode::realistic: return CoronalFlaresMode::vibrant;
        case CoronalFlaresMode::vibrant:   return CoronalFlaresMode::authentic;
    }
    return CoronalFlaresMode::authentic;
}

CoronalFlaresMode cycle_coronal_flares_mode() {
    g_coronal_flares_mode = cycle_coronal_flares_mode(g_coronal_flares_mode);
    return g_coronal_flares_mode;
}

const char *coronal_flares_mode_name(CoronalFlaresMode mode) {
    switch (mode) {
        case CoronalFlaresMode::authentic: return "CORONA FLARES: AUTHENTIC";
        case CoronalFlaresMode::realistic: return "CORONA FLARES: REALISTIC";
        case CoronalFlaresMode::vibrant:   return "CORONA FLARES: VIBRANT";
    }
    return "CORONA FLARES: AUTHENTIC";
}

std::optional<CoronalFlaresMode> parse_coronal_flares_mode(std::string_view str) {
    if (str == "authentic" || str == "AUTHENTIC" || str == "off" || str == "0") {
        return CoronalFlaresMode::authentic;
    }
    if (str == "realistic" || str == "REALISTIC" || str == "standard" || str == "1") {
        return CoronalFlaresMode::realistic;
    }
    if (str == "vibrant" || str == "VIBRANT" || str == "on" || str == "2") {
        return CoronalFlaresMode::vibrant;
    }
    return std::nullopt;
}

SpectralPaletteRamp get_spectral_palette_ramp(int16_t star_class, CoronalFlaresMode mode,
                                             double sim_secs, uint16_t star_seed) {
    SpectralPaletteRamp ramp{};
    switch (star_class) {
        case 0: // Solar G-type (~5,778K) - warm golden incandescent
            ramp.core_r = 64; ramp.core_g = 60; ramp.core_b = 48;
            ramp.corona_r = 64; ramp.corona_g = 46; ramp.corona_b = 20;
            ramp.limb_u = 0.45f; ramp.limb_v = 0.22f; ramp.corona_extent = 1.8f;
            break;
        case 1: // A/F-type (~8,500K) - brilliant white with ice-blue fringe
            ramp.core_r = 62; ramp.core_g = 63; ramp.core_b = 64;
            ramp.corona_r = 38; ramp.corona_g = 48; ramp.corona_b = 64;
            ramp.limb_u = 0.36f; ramp.limb_v = 0.20f; ramp.corona_extent = 2.0f;
            break;
        case 2: // White Dwarf (~25,000K) - diamond sapphire white with deep violet corona
            ramp.core_r = 60; ramp.core_g = 62; ramp.core_b = 64;
            ramp.corona_r = 26; ramp.corona_g = 34; ramp.corona_b = 62;
            ramp.limb_u = 0.30f; ramp.limb_v = 0.15f; ramp.corona_extent = 1.5f;
            break;
        case 3: // Red Giant (~3,000K) - incandescent scarlet-orange with vast vermilion envelope
            ramp.core_r = 64; ramp.core_g = 38; ramp.core_b = 20;
            ramp.corona_r = 64; ramp.corona_g = 18; ramp.corona_b = 10;
            ramp.limb_u = 0.60f; ramp.limb_v = 0.26f; ramp.corona_extent = 2.5f;
            break;
        case 4: // K-type Orange Dwarf (~4,400K) - molten amber
            ramp.core_r = 64; ramp.core_g = 48; ramp.core_b = 30;
            ramp.corona_r = 64; ramp.corona_g = 32; ramp.corona_b = 16;
            ramp.limb_u = 0.50f; ramp.limb_v = 0.24f; ramp.corona_extent = 1.9f;
            break;
        case 5: // M-type Red Dwarf (~3,200K) - deep ruby crimson
            ramp.core_r = 64; ramp.core_g = 28; ramp.core_b = 16;
            ramp.corona_r = 48; ramp.corona_g = 16; ramp.corona_b = 10;
            ramp.limb_u = 0.58f; ramp.limb_v = 0.26f; ramp.corona_extent = 1.6f;
            break;
        case 6: // Brown Dwarf (~1,500K) - deep infrared magenta-brown
            ramp.core_r = 42; ramp.core_g = 24; ramp.core_b = 26;
            ramp.corona_r = 28; ramp.corona_g = 16; ramp.corona_b = 20;
            ramp.limb_u = 0.65f; ramp.limb_v = 0.25f; ramp.corona_extent = 1.4f;
            break;
        case 7: // Blue Hypergiant / Wolf-Rayet (~40,000K) - blinding electric cerulean-violet
            ramp.core_r = 58; ramp.core_g = 62; ramp.core_b = 64;
            ramp.corona_r = 24; ramp.corona_g = 42; ramp.corona_b = 64;
            ramp.limb_u = 0.28f; ramp.limb_v = 0.16f; ramp.corona_extent = 2.8f;
            break;
        case 8: { // Variable / Flare Star - dynamically shifting chromatic flares
            const float pulse = static_cast<float>(0.5 + 0.5 * std::sin(sim_secs * 0.4 + (star_seed & 0xFF)));
            ramp.core_r = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(60.0f + 4.0f * pulse), 0, 64));
            ramp.core_g = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(50.0f + 10.0f * pulse), 0, 64));
            ramp.core_b = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(32.0f + 24.0f * pulse), 0, 64));
            ramp.corona_r = 64; ramp.corona_g = 36; ramp.corona_b = 24;
            ramp.limb_u = 0.48f; ramp.limb_v = 0.22f; ramp.corona_extent = 2.4f;
            break;
        }
        case 9: // Exotic / Nebular - ionized cyan-emerald and violet
            ramp.core_r = 48; ramp.core_g = 64; ramp.core_b = 56;
            ramp.corona_r = 30; ramp.corona_g = 60; ramp.corona_b = 52;
            ramp.limb_u = 0.35f; ramp.limb_v = 0.20f; ramp.corona_extent = 2.3f;
            break;
        case 10: // Dim Collapsed / Dark Core (~1,200K) - faint infrared ember boundary
            ramp.core_r = 36; ramp.core_g = 26; ramp.core_b = 20;
            ramp.corona_r = 24; ramp.corona_g = 16; ramp.corona_b = 12;
            ramp.limb_u = 0.68f; ramp.limb_v = 0.22f; ramp.corona_extent = 1.3f;
            break;
        case 11: // Pulsar / Neutron Star - intense synchrotron cyan/violet relativistic jet
            ramp.core_r = 62; ramp.core_g = 63; ramp.core_b = 64;
            ramp.corona_r = 30; ramp.corona_g = 52; ramp.corona_b = 64;
            ramp.limb_u = 0.25f; ramp.limb_v = 0.10f; ramp.corona_extent = 3.2f;
            break;
        default:
            ramp.core_r = 64; ramp.core_g = 58; ramp.core_b = 40;
            ramp.corona_r = 64; ramp.corona_g = 48; ramp.corona_b = 24;
            ramp.limb_u = 0.45f; ramp.limb_v = 0.22f; ramp.corona_extent = 1.8f;
            break;
    }

    if (mode == CoronalFlaresMode::vibrant) {
        // Boost contrast and coronal radius for cinematic vibrance
        ramp.corona_extent *= 1.25f;
        auto boost_sat = [](uint8_t &val, uint8_t pivot) {
            float diff = static_cast<float>(val) - static_cast<float>(pivot);
            val = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(pivot + diff * 1.30f), 0, 64));
        };
        uint8_t avg_c = (ramp.corona_r + ramp.corona_g + ramp.corona_b) / 3;
        boost_sat(ramp.corona_r, avg_c);
        boost_sat(ramp.corona_g, avg_c);
        boost_sat(ramp.corona_b, avg_c);
    }

    return ramp;
}

float compute_limb_darkening(float r_norm, float u, float v) {
    const float r = std::clamp(r_norm, 0.0f, 1.0f);
    const float mu = std::sqrt(std::max(0.0f, 1.0f - r * r));
    const float om_mu = 1.0f - mu;
    const float factor = 1.0f - u * om_mu - v * om_mu * om_mu;
    return std::clamp(factor, 0.15f, 1.0f);
}

float compute_coronal_streamer(float angle_rad, int16_t star_class, double sim_secs,
                               uint16_t star_seed, CoronalFlaresMode mode) {
    const float t = static_cast<float>(sim_secs);
    const float seed_phase = static_cast<float>(star_seed & 0xFF) * 0.0245f;

    // Multi-harmonic natural streamer base
    float s = 0.0f;
    s += 0.35f * std::cos(2.0f * angle_rad + 0.08f * t + seed_phase);
    s += 0.28f * std::cos(3.0f * angle_rad - 0.05f * t + seed_phase * 1.7f);
    s += 0.20f * std::cos(5.0f * angle_rad + 0.11f * t + seed_phase * 2.3f);
    s += 0.12f * std::cos(7.0f * angle_rad - 0.14f * t + seed_phase * 3.1f);
    s += 0.08f * std::cos(11.0f * angle_rad + 0.20f * t + seed_phase * 4.7f);

    if (star_class == 11) {
        // Pulsar: Relativistic dual-polar jet beams aligned along rotating magnetic axis
        const float spin_freq = 1.5f + static_cast<float>((star_seed >> 4) & 0x07) * 0.25f;
        const float jet_angle = std::fmod(2.0f * PI * spin_freq * t + seed_phase, 2.0f * PI);
        const float diff1 = std::abs(angle_diff(angle_rad, jet_angle));
        const float diff2 = std::abs(angle_diff(angle_rad, jet_angle + PI));
        const float min_diff = std::min(diff1, diff2);
        const float sigma = 0.22f; // ~12.5 degrees beam width
        const float jet_beam = std::exp(-(min_diff * min_diff) / (2.0f * sigma * sigma));
        s = s * 0.3f + jet_beam * 1.8f;
    } else if (star_class == 7) {
        // Wolf-Rayet / Blue Hypergiant: High-energy turbulent supersonic stellar wind
        s += 0.15f * std::cos(13.0f * angle_rad + 0.35f * t + seed_phase * 5.1f);
        s += 0.10f * std::cos(17.0f * angle_rad - 0.42f * t + seed_phase * 6.3f);
        s += 0.07f * std::cos(23.0f * angle_rad + 0.55f * t + seed_phase * 7.9f);
        s *= 1.35f;
    } else if (star_class == 3) {
        // Red Giant: Expansive, undulating convective coronal plumes
        s = 0.50f * std::cos(2.0f * angle_rad + 0.03f * t + seed_phase) +
            0.40f * std::cos(3.0f * angle_rad - 0.04f * t + seed_phase * 1.4f) +
            0.25f * std::cos(4.0f * angle_rad + 0.06f * t + seed_phase * 2.1f);
        s *= 1.25f;
    } else if (star_class == 8) {
        // Variable / Flare Star: Dynamic explosive coronal mass ejection loop
        const float flare_phase = std::fmod(0.25f * t + seed_phase, 2.0f * PI);
        const float flare_burst = std::pow(std::max(0.0f, std::sin(flare_phase)), 10.0f);
        const float flare_angle = seed_phase * 2.0f;
        const float flare_diff = std::abs(angle_diff(angle_rad, flare_angle));
        const float flare_spot = std::exp(-(flare_diff * flare_diff) / 0.18f) * flare_burst * 2.2f;
        s += flare_spot;
    } else if (star_class == 2) {
        // White Dwarf: Ultra-compact high-surface-gravity sheath
        s *= 0.55f;
    }

    if (mode == CoronalFlaresMode::vibrant) {
        s *= 1.35f;
    }

    return std::clamp(s, -0.6f, 2.0f);
}

void apply_enhanced_spectral_palette(int32_t &ir, int32_t &ig, int32_t &ib,
                                     int32_t &ir2, int32_t &ig2, int32_t &ib2,
                                     int16_t star_class, CoronalFlaresMode mode,
                                     double sim_secs, uint16_t star_seed) {
    if (mode == CoronalFlaresMode::authentic) return;

    const auto ramp = get_spectral_palette_ramp(star_class, mode, sim_secs, star_seed);
    ir  = ramp.core_r;
    ig  = ramp.core_g;
    ib  = ramp.core_b;
    ir2 = ramp.corona_r;
    ig2 = ramp.corona_g;
    ib2 = ramp.corona_b;
}

void render_coronal_globe(uint8_t *target, int32_t width, int32_t height, int32_t scale,
                          double center_x, double center_y, double mag, double fgm,
                          int16_t star_class, double sim_secs, uint16_t star_seed,
                          CoronalFlaresMode mode) {
    if (!target || width <= 0 || height <= 0 || scale <= 0) return;

    const auto ramp = get_spectral_palette_ramp(star_class, mode, sim_secs, star_seed);
    const double disk_radius = (fgm > 0.0) ? fgm : (mag * 0.35);
    const double disk_radius_sq = disk_radius * disk_radius;
    const double corona_mult = ramp.corona_extent;
    const double max_extent = mag * (1.0 + 0.35 * (corona_mult - 1.0));
    const double max_extent_sq = max_extent * max_extent;

    const double y_begin = -max_extent * 1.2;
    const double y_end   = center_y + max_extent;
    double cur_y         = center_y - max_extent;

    double ya = y_begin;
    while (cur_y < y_end) {
        double cur_x = center_x - max_extent;
        double xa    = -max_extent;
        const double cur_x_end = center_x + max_extent;

        while (cur_x < cur_x_end) {
            const int32_t px = static_cast<int32_t>(cur_x);
            const int32_t py = static_cast<int32_t>(cur_y);

            if (px > 9 * scale && px < width - 7 * scale &&
                py > 9 * scale && py < height - 10 * scale) {

                const double r_sq = xa * xa + ya * ya;
                if (r_sq < max_extent_sq) {
                    const double r = std::sqrt(r_sq);
                    int8_t pix = 0;

                    if (r <= disk_radius) {
                        // Photospheric stellar disk: physically informed limb darkening
                        const float r_norm = static_cast<float>(r / std::max(1.0, disk_radius));
                        const float limb_factor = compute_limb_darkening(r_norm, ramp.limb_u, ramp.limb_v);
                        const float disk_intensity = 0.50f + 0.50f * limb_factor;
                        pix = static_cast<int8_t>(std::clamp<int>(static_cast<int>(std::round(0x3F * disk_intensity)), 1, 0x3F));
                    } else {
                        // Coronal region: dynamic prominence streamers and atmospheric falloff
                        const float angle = static_cast<float>(std::atan2(ya, xa));
                        const float streamer = compute_coronal_streamer(angle, star_class, sim_secs, star_seed, mode);
                        const double eff_corona_r = disk_radius + (max_extent - disk_radius) * (1.0 + 0.40 * streamer);

                        if (r <= eff_corona_r) {
                            const double t_norm = (r - disk_radius) / std::max(1.0, eff_corona_r - disk_radius);
                            const float falloff = static_cast<float>(std::pow(std::max(0.0, 1.0 - t_norm), 1.6));
                            const float corona_intensity = falloff * (0.85f + 0.25f * std::max(-0.5f, streamer));
                            pix = static_cast<int8_t>(std::clamp<int>(static_cast<int>(std::round(0x3F * corona_intensity)), 0, 0x3F));
                        }
                    }

                    if (pix > 0) {
                        const uint32_t pixptr = static_cast<uint32_t>(width * py + px);
                        if (pixptr + width + 1 < static_cast<uint32_t>(width * height)) {
                            int16_t combined = static_cast<int16_t>(pix) + target[pixptr];
                            uint8_t final_pix = static_cast<uint8_t>(std::min<int16_t>(0x3F, combined));

                            target[pixptr]             = final_pix;
                            target[pixptr + 1]         = final_pix;
                            target[pixptr + width]     = final_pix;
                            target[pixptr + width + 1] = final_pix;
                        }
                    }
                }
            }

            xa += 2.0;
            cur_x += 2.0;
        }

        ya += 2.4;
        cur_y += 2.0;
    }
}

void render_coronal_sun(uint8_t *target, int32_t width, int32_t height, int32_t scale,
                        double center_x, double center_y, double mag, double fgm,
                        int16_t star_class, double sim_secs, uint16_t star_seed,
                        CoronalFlaresMode mode) {
    if (!target || width <= 0 || height <= 0 || scale <= 0) return;

    const auto ramp = get_spectral_palette_ramp(star_class, mode, sim_secs, star_seed);
    const double disk_radius = (fgm > 0.0) ? fgm : (mag * 0.35);
    const double max_extent = mag * (1.0 + 0.30 * (ramp.corona_extent - 1.0));
    const double max_extent_sq = max_extent * max_extent;

    const double y_begin = -max_extent * 1.2;
    const double y_end   = center_y + max_extent;
    double cur_y         = center_y - max_extent;

    double ya = y_begin;
    while (cur_y < y_end) {
        double cur_x = center_x - max_extent;
        double xa    = -max_extent;
        const double cur_x_end = center_x + max_extent;

        while (cur_x < cur_x_end) {
            const int32_t px = static_cast<int32_t>(cur_x);
            const int32_t py = static_cast<int32_t>(cur_y);

            if (px > 9 * scale && px < width - 7 * scale &&
                py > 9 * scale && py < height - 10 * scale) {

                const double r_sq = xa * xa + ya * ya;
                if (r_sq < max_extent_sq) {
                    const double r = std::sqrt(r_sq);
                    int8_t pix = 0;

                    if (r <= disk_radius) {
                        const float r_norm = static_cast<float>(r / std::max(1.0, disk_radius));
                        const float limb_factor = compute_limb_darkening(r_norm, ramp.limb_u, ramp.limb_v);
                        const float disk_intensity = 0.55f + 0.45f * limb_factor;
                        pix = static_cast<int8_t>(std::clamp<int>(static_cast<int>(std::round(0x3F * disk_intensity)), 1, 0x3F));
                    } else {
                        const float angle = static_cast<float>(std::atan2(ya, xa));
                        const float streamer = compute_coronal_streamer(angle, star_class, sim_secs, star_seed, mode);
                        const double eff_corona_r = disk_radius + (max_extent - disk_radius) * (1.0 + 0.35 * streamer);

                        if (r <= eff_corona_r) {
                            const double t_norm = (r - disk_radius) / std::max(1.0, eff_corona_r - disk_radius);
                            const float falloff = static_cast<float>(std::pow(std::max(0.0, 1.0 - t_norm), 1.5));
                            const float corona_intensity = falloff * (0.85f + 0.20f * std::max(-0.5f, streamer));
                            pix = static_cast<int8_t>(std::clamp<int>(static_cast<int>(std::round(0x3F * corona_intensity)), 0, 0x3F));
                        }
                    }

                    if (pix > 0) {
                        const uint32_t pixptr = static_cast<uint32_t>(width * py + px);
                        if (pixptr < static_cast<uint32_t>(width * height)) {
                            int16_t combined = static_cast<int16_t>(pix) + target[pixptr];
                            target[pixptr] = static_cast<uint8_t>(std::min<int16_t>(0x3F, combined));
                        }
                    }
                }
            }

            xa += 1.0;
            cur_x += 1.0;
        }

        ya += 1.2;
        cur_y += 1.0;
    }
}

} // namespace noctis
