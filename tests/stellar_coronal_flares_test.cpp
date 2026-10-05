#include "stellar_coronal_flares.h"

#include <cmath>
#include <iostream>
#include <numbers>
#include <vector>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

constexpr float PI = std::numbers::pi_v<float>;

} // namespace

int main() {
    bool ok = true;

    // 1. Mode cycling and parsing
    {
        auto mode = noctis::CoronalFlaresMode::authentic;
        mode = noctis::cycle_coronal_flares_mode(mode);
        ok &= require(mode == noctis::CoronalFlaresMode::realistic, "authentic cycles to realistic");
        mode = noctis::cycle_coronal_flares_mode(mode);
        ok &= require(mode == noctis::CoronalFlaresMode::vibrant, "realistic cycles to vibrant");
        mode = noctis::cycle_coronal_flares_mode(mode);
        ok &= require(mode == noctis::CoronalFlaresMode::authentic, "vibrant cycles to authentic");

        ok &= require(std::string(noctis::coronal_flares_mode_name(noctis::CoronalFlaresMode::authentic)) ==
                          "CORONA FLARES: AUTHENTIC", "authentic name");
        ok &= require(std::string(noctis::coronal_flares_mode_name(noctis::CoronalFlaresMode::realistic)) ==
                          "CORONA FLARES: REALISTIC", "realistic name");
        ok &= require(std::string(noctis::coronal_flares_mode_name(noctis::CoronalFlaresMode::vibrant)) ==
                          "CORONA FLARES: VIBRANT", "vibrant name");

        ok &= require(noctis::parse_coronal_flares_mode("authentic") == noctis::CoronalFlaresMode::authentic, "parse authentic");
        ok &= require(noctis::parse_coronal_flares_mode("realistic") == noctis::CoronalFlaresMode::realistic, "parse realistic");
        ok &= require(noctis::parse_coronal_flares_mode("vibrant") == noctis::CoronalFlaresMode::vibrant, "parse vibrant");
        ok &= require(noctis::parse_coronal_flares_mode("off") == noctis::CoronalFlaresMode::authentic, "parse off");
        ok &= require(noctis::parse_coronal_flares_mode("on") == noctis::CoronalFlaresMode::vibrant, "parse on");
        ok &= require(!noctis::parse_coronal_flares_mode("unknown").has_value(), "parse invalid nullopt");
    }

    // 2. Limb darkening math
    {
        const float u = 0.45f;
        const float v = 0.22f;

        const float center = noctis::compute_limb_darkening(0.0f, u, v);
        ok &= require(std::abs(center - 1.0f) < 1e-4f, "center limb darkening is 1.0");

        const float limb = noctis::compute_limb_darkening(1.0f, u, v);
        const float expected_limb = 1.0f - u - v;
        ok &= require(std::abs(limb - expected_limb) < 1e-4f, "limb intensity matches 1 - u - v");
        ok &= require(limb < center, "limb is darker than disk center");

        // Monotonic non-increasing check from center to limb
        float prev = center;
        bool monotonic = true;
        for (int i = 1; i <= 20; ++i) {
            float r = static_cast<float>(i) / 20.0f;
            float val = noctis::compute_limb_darkening(r, u, v);
            if (val > prev + 1e-5f) {
                monotonic = false;
                break;
            }
            prev = val;
        }
        ok &= require(monotonic, "limb darkening is monotonically non-increasing");
    }

    // 3. Coronal streamer modulation & pulsar jet symmetry
    {
        const uint16_t seed = 4211;
        const double t = 100.0;

        // Class 11 (Pulsar): check that streamer output is bounded and periodic
        for (int deg = 0; deg < 360; deg += 15) {
            float rad = deg * PI / 180.0f;
            float s = noctis::compute_coronal_streamer(rad, 11, t, seed, noctis::CoronalFlaresMode::realistic);
            ok &= require(s >= -0.6f && s <= 2.0f, "pulsar streamer value bounded");
        }

        // Test Wolf-Rayet (Class 7) vs White Dwarf (Class 2): Wolf-Rayet has more energetic corona
        float max_wr = -1.0f;
        float max_wd = -1.0f;
        for (int deg = 0; deg < 360; deg += 5) {
            float rad = deg * PI / 180.0f;
            max_wr = std::max(max_wr, noctis::compute_coronal_streamer(rad, 7, t, seed, noctis::CoronalFlaresMode::realistic));
            max_wd = std::max(max_wd, noctis::compute_coronal_streamer(rad, 2, t, seed, noctis::CoronalFlaresMode::realistic));
        }
        ok &= require(max_wr > max_wd, "Wolf-Rayet coronal streamer exceeds White Dwarf");

        // Test dynamic time variation
        float s_t0 = noctis::compute_coronal_streamer(0.5f, 0, 10.0, seed, noctis::CoronalFlaresMode::realistic);
        float s_t1 = noctis::compute_coronal_streamer(0.5f, 0, 20.0, seed, noctis::CoronalFlaresMode::realistic);
        ok &= require(s_t0 != s_t1, "streamer evolves across simulation time");
    }

    // 4. Spectral palette ramps
    {
        // Class 7: Blue Hypergiant should be blue-dominant
        const auto wr = noctis::get_spectral_palette_ramp(7, noctis::CoronalFlaresMode::realistic, 0.0, 123);
        ok &= require(wr.corona_b > wr.corona_r, "Class 7 blue hypergiant corona is blue-dominated");
        ok &= require(wr.core_b >= wr.core_r, "Class 7 core is blue-dominated");

        // Class 3: Red Giant should be red-dominant
        const auto rg = noctis::get_spectral_palette_ramp(3, noctis::CoronalFlaresMode::realistic, 0.0, 123);
        ok &= require(rg.core_r > rg.core_b, "Class 3 red giant core is red-dominated");
        ok &= require(rg.corona_r > rg.corona_b, "Class 3 corona is red-dominated");

        // Class 0: Solar G-type should have warm golden corona (R > B, G > B)
        const auto sun = noctis::get_spectral_palette_ramp(0, noctis::CoronalFlaresMode::realistic, 0.0, 123);
        ok &= require(sun.corona_r > sun.corona_b && sun.corona_g > sun.corona_b, "Class 0 solar corona is golden");

        // Vibrant mode extends coronal radius
        const auto rg_vib = noctis::get_spectral_palette_ramp(3, noctis::CoronalFlaresMode::vibrant, 0.0, 123);
        ok &= require(rg_vib.corona_extent >= rg.corona_extent, "Vibrant mode increases coronal extent");
    }

    // 5. Palette application helper
    {
        int32_t ir = 0, ig = 0, ib = 0, ir2 = 0, ig2 = 0, ib2 = 0;
        // In authentic mode, colors should NOT be modified
        ir = 10; ig = 20; ib = 30; ir2 = 40; ig2 = 50; ib2 = 60;
        noctis::apply_enhanced_spectral_palette(ir, ig, ib, ir2, ig2, ib2, 0,
                                               noctis::CoronalFlaresMode::authentic, 0.0, 123);
        ok &= require(ir == 10 && ig == 20 && ib == 30, "authentic mode does not alter colors");

        // In realistic mode, colors should be updated
        noctis::apply_enhanced_spectral_palette(ir, ig, ib, ir2, ig2, ib2, 0,
                                               noctis::CoronalFlaresMode::realistic, 0.0, 123);
        ok &= require(ir == 64 && ig == 60 && ib == 48, "realistic mode applies solar core palette");
    }

    // 6. Buffer rendering safety and pixel output
    {
        const int32_t width = 320;
        const int32_t height = 200;
        std::vector<uint8_t> buffer(width * height, 0);

        // Render globe into buffer with pixel-scaled mag and fgm
        const double mag = 1.2 * 100.0 + 1.5; // ~121.5 pixels radius
        const double fgm = 0.35 * mag;         // ~42.5 pixels disk radius
        noctis::render_coronal_globe(buffer.data(), width, height, 1, 160.0, 100.0,
                                    mag, fgm, 0, 50.0, 999,
                                    noctis::CoronalFlaresMode::realistic);

        int non_zero_count = 0;
        bool all_valid_range = true;
        for (uint8_t pix : buffer) {
            if (pix > 0) non_zero_count++;
            if (pix > 0x3F) all_valid_range = false;
        }
        ok &= require(non_zero_count > 100, "coronal globe rendered non-zero pixels");
        ok &= require(all_valid_range, "all globe pixels within 0..0x3F VGA palette range");

        // Render sun into fresh buffer
        std::vector<uint8_t> sun_buffer(width * height, 0);
        noctis::render_coronal_sun(sun_buffer.data(), width, height, 1, 160.0, 100.0,
                                   mag, fgm, 7, 50.0, 999,
                                   noctis::CoronalFlaresMode::vibrant);

        int sun_non_zero = 0;
        bool sun_valid = true;
        for (uint8_t pix : sun_buffer) {
            if (pix > 0) sun_non_zero++;
            if (pix > 0x3F) sun_valid = false;
        }
        ok &= require(sun_non_zero > 100, "coronal sun rendered non-zero pixels");
        ok &= require(sun_valid, "all sun pixels within 0..0x3F range");

        // Boundary null safety checks
        noctis::render_coronal_globe(nullptr, width, height, 1, 160.0, 100.0, 1.0, 0.3, 0, 0.0, 0, noctis::CoronalFlaresMode::realistic);
        noctis::render_coronal_sun(nullptr, width, height, 1, 160.0, 100.0, 1.0, 0.3, 0, 0.0, 0, noctis::CoronalFlaresMode::realistic);
        noctis::render_coronal_globe(buffer.data(), -1, height, 1, 160.0, 100.0, 1.0, 0.3, 0, 0.0, 0, noctis::CoronalFlaresMode::realistic);
    }

    if (ok) {
        std::cout << "All stellar coronal flares tests passed.\n";
        return 0;
    }
    return 1;
}
