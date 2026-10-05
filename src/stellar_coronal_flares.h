#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace noctis {

enum class CoronalFlaresMode : uint8_t {
    authentic = 0,
    realistic = 1,
    vibrant   = 2,
};

CoronalFlaresMode get_coronal_flares_mode();
void set_coronal_flares_mode(CoronalFlaresMode mode);
CoronalFlaresMode cycle_coronal_flares_mode(CoronalFlaresMode current);
CoronalFlaresMode cycle_coronal_flares_mode();
const char *coronal_flares_mode_name(CoronalFlaresMode mode);
std::optional<CoronalFlaresMode> parse_coronal_flares_mode(std::string_view str);

struct SpectralPaletteRamp {
    uint8_t core_r;
    uint8_t core_g;
    uint8_t core_b;
    uint8_t corona_r;
    uint8_t corona_g;
    uint8_t corona_b;
    float limb_u;          // Quadratic limb darkening linear coefficient
    float limb_v;          // Quadratic limb darkening second-order coefficient
    float corona_extent;   // Baseline extent of corona relative to disk radius
};

// Returns enhanced Planckian blackbody spectral radiation palette for a star class
SpectralPaletteRamp get_spectral_palette_ramp(int16_t star_class, CoronalFlaresMode mode,
                                             double sim_secs, uint16_t star_seed);

// Evaluates quadratic Eddington limb darkening factor [0..1] for normalized radius r_norm in [0..1]
float compute_limb_darkening(float r_norm, float u, float v);

// Evaluates procedural angular prominence / streamer modulation factor around the star limb
float compute_coronal_streamer(float angle_rad, int16_t star_class, double sim_secs,
                               uint16_t star_seed, CoronalFlaresMode mode);

// Modifies the stellar space palette colors according to spectral model
void apply_enhanced_spectral_palette(int32_t &ir, int32_t &ig, int32_t &ib,
                                     int32_t &ir2, int32_t &ig2, int32_t &ib2,
                                     int16_t star_class, CoronalFlaresMode mode,
                                     double sim_secs, uint16_t star_seed);

// Enhanced stellar globe rendering (coronal flares + limb darkening in space)
void render_coronal_globe(uint8_t *target, int32_t width, int32_t height, int32_t scale,
                          double center_x, double center_y, double mag, double fgm,
                          int16_t star_class, double sim_secs, uint16_t star_seed,
                          CoronalFlaresMode mode);

// Enhanced sun rendering on planetary surfaces (limb darkening + prominence halo in sky)
void render_coronal_sun(uint8_t *target, int32_t width, int32_t height, int32_t scale,
                        double center_x, double center_y, double mag, double fgm,
                        int16_t star_class, double sim_secs, uint16_t star_seed,
                        CoronalFlaresMode mode);

} // namespace noctis
