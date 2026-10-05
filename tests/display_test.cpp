#include "display.h"
#include "simulation_clock.h"
#include "noctis-d.h"

#include <cmath>
#include <cstdio>
#include <fstream>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "display test failure: %s\n", message);
        return false;
    }
    return true;
}
} // namespace

int main() {
    bool ok = true;

    // 1. Invalid dimensions
    {
        const auto vp = noctis::calculate_viewport(0, 0, noctis::AspectRatioMode::crt_4_3);
        ok &= require(vp.width == 0.0f && vp.height == 0.0f, "zero size should yield zero viewport");
    }

    // 2. 1280x720 window (16:9 widescreen) in 4:3 CRT mode -> Pillarboxed
    {
        const auto vp = noctis::calculate_viewport(1280, 720, noctis::AspectRatioMode::crt_4_3);
        // Height should be 720, Width should be 720 * (4/3) = 960
        // Pillarbox X offset should be (1280 - 960) / 2 = 160
        ok &= require(vp.height == 720.0f, "4:3 in 1280x720: height should be 720");
        ok &= require(vp.width == 960.0f, "4:3 in 1280x720: width should be 960");
        ok &= require(vp.x == 160.0f, "4:3 in 1280x720: x offset should be 160");
        ok &= require(vp.y == 0.0f, "4:3 in 1280x720: y offset should be 0");
        ok &= require(std::abs(vp.width / vp.height - 4.0f / 3.0f) < 0.01f, "4:3 ratio verified");
    }

    // 3. 1920x1080 window (1080p 16:9) in 4:3 CRT mode -> Pillarboxed
    {
        const auto vp = noctis::calculate_viewport(1920, 1080, noctis::AspectRatioMode::crt_4_3);
        // Height = 1080, Width = 1080 * 4/3 = 1440, X = (1920 - 1440) / 2 = 240
        ok &= require(vp.height == 1080.0f, "4:3 in 1080p: height should be 1080");
        ok &= require(vp.width == 1440.0f, "4:3 in 1080p: width should be 1440");
        ok &= require(vp.x == 240.0f, "4:3 in 1080p: x offset should be 240");
        ok &= require(vp.y == 0.0f, "4:3 in 1080p: y offset should be 0");
    }

    // 4. Tall window (e.g. 800x800 square 1:1) in 4:3 CRT mode -> Letterboxed
    {
        const auto vp = noctis::calculate_viewport(800, 800, noctis::AspectRatioMode::crt_4_3);
        // Width = 800, Height = 800 / (4/3) = 600, Y = (800 - 600) / 2 = 100
        ok &= require(vp.width == 800.0f, "4:3 in 800x800: width should be 800");
        ok &= require(vp.height == 600.0f, "4:3 in 800x800: height should be 600");
        ok &= require(vp.x == 0.0f, "4:3 in 800x800: x offset should be 0");
        ok &= require(vp.y == 100.0f, "4:3 in 800x800: y offset should be 100");
    }

    // 5. 16:10 Pixel-Exact mode in 1280x720 window
    {
        const auto vp = noctis::calculate_viewport(1280, 720, noctis::AspectRatioMode::pixel_16_10);
        // Height = 720, Width = 720 * 1.6 = 1152, X = (1280 - 1152) / 2 = 64
        ok &= require(vp.height == 720.0f, "16:10 in 1280x720: height should be 720");
        ok &= require(vp.width == 1152.0f, "16:10 in 1280x720: width should be 1152");
        ok &= require(vp.x == 64.0f, "16:10 in 1280x720: x offset should be 64");
        ok &= require(vp.y == 0.0f, "16:10 in 1280x720: y offset should be 0");
    }

    // 6. 16:9 Stretch mode in 1280x720 window
    {
        const auto vp = noctis::calculate_viewport(1280, 720, noctis::AspectRatioMode::stretch_16_9);
        ok &= require(vp.width == 1280.0f && vp.height == 720.0f, "16:9 stretch width/height");
        ok &= require(vp.x == 0.0f && vp.y == 0.0f, "16:9 stretch x/y");
    }

    // 7. Cycle aspect ratio modes
    {
        auto mode = noctis::AspectRatioMode::crt_4_3;
        mode      = noctis::cycle_aspect_ratio_mode(mode);
        ok &= require(mode == noctis::AspectRatioMode::pixel_16_10, "cycle to 16:10");
        mode = noctis::cycle_aspect_ratio_mode(mode);
        ok &= require(mode == noctis::AspectRatioMode::stretch_16_9, "cycle to 16:9");
        mode = noctis::cycle_aspect_ratio_mode(mode);
        ok &= require(mode == noctis::AspectRatioMode::crt_4_3, "cycle back to 4:3");
    }

    // 8. Aspect mode names
    {
        ok &= require(std::string(noctis::aspect_ratio_mode_name(noctis::AspectRatioMode::crt_4_3)) ==
                          "ASPECT: 4:3 CRT",
                      "mode name 4:3");
        ok &= require(std::string(noctis::aspect_ratio_mode_name(noctis::AspectRatioMode::pixel_16_10)) ==
                          "ASPECT: 16:10 SQUARE",
                      "mode name 16:10");
        ok &= require(std::string(noctis::aspect_ratio_mode_name(noctis::AspectRatioMode::stretch_16_9)) ==
                          "ASPECT: 16:9 STRETCH",
                      "mode name 16:9");
    }

    // 9. Timewarp slider touch
    {
        noctis::touch_timewarp_slider();
        ok &= require(true, "touch_timewarp_slider did not crash");
    }

    // 10. CRT shader state and toggle
    {
        ok &= require(!noctis::is_crt_shader_enabled(), "CRT shader disabled by default");
        noctis::set_crt_shader_enabled(true);
        ok &= require(noctis::is_crt_shader_enabled(), "CRT shader enabled");
        const bool toggled = noctis::toggle_crt_shader();
        ok &= require(!toggled && !noctis::is_crt_shader_enabled(), "CRT shader toggled off");
        // Safe begin/end calls without initialized window/shader should not crash
        noctis::begin_crt_shader(640, 480);
        noctis::end_crt_shader();
    }

    // 11. Sub-pixel fidelity state and toggle
    {
        ok &= require(!noctis::get_subpixel_fidelity(), "subpixel fidelity disabled by default");
        noctis::set_subpixel_fidelity(true);
        ok &= require(noctis::get_subpixel_fidelity(), "subpixel fidelity enabled");
        const bool toggled = noctis::toggle_subpixel_fidelity();
        ok &= require(!toggled && !noctis::get_subpixel_fidelity(), "subpixel fidelity toggled off");
    }

    // 12. Display Settings persistence round-trip
    {
        const auto test_dir = std::filesystem::temp_directory_path() / "noctis_display_test_cfg";
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);

        // Configure custom settings
        noctis::set_aspect_ratio_mode(noctis::AspectRatioMode::pixel_16_10);
        noctis::set_upscale_mode(noctis::UpscaleMode::edge_scale2x);
        noctis::set_crt_shader_enabled(true);
        noctis::set_subpixel_fidelity(true);
        noctis::set_fullscreen(true);
        noctis::set_timewarp_multiplier(250);
        noctis::set_setting_draw_hud(0);
        noctis::set_setting_lens_flare_mode(-1);
        noctis::set_setting_seamless_border(1);
        noctis::set_draw_distance_mode(noctis::DrawDistanceMode::far);
        noctis::set_texture_filter_mode(noctis::TextureFilterMode::detailed);

        const bool saved = noctis::save_display_settings(test_dir);
        ok &= require(saved, "save_display_settings should succeed");
        ok &= require(std::filesystem::exists(test_dir / "display_settings.ini"), "display_settings.ini should exist");

        // Mutate all settings to different values
        noctis::set_aspect_ratio_mode(noctis::AspectRatioMode::stretch_16_9);
        noctis::set_upscale_mode(noctis::UpscaleMode::crisp_pixel);
        noctis::set_crt_shader_enabled(false);
        noctis::set_subpixel_fidelity(false);
        noctis::set_fullscreen(false);
        noctis::set_timewarp_multiplier(1);
        noctis::set_setting_draw_hud(1);
        noctis::set_setting_lens_flare_mode(1);
        noctis::set_setting_seamless_border(0);
        noctis::set_draw_distance_mode(noctis::DrawDistanceMode::standard);
        noctis::set_texture_filter_mode(noctis::TextureFilterMode::nearest);

        // Load settings back
        const bool loaded = noctis::load_display_settings(test_dir);
        ok &= require(loaded, "load_display_settings should succeed");

        ok &= require(noctis::get_aspect_ratio_mode() == noctis::AspectRatioMode::pixel_16_10,
                      "aspect_ratio restored to 16:10");
        ok &= require(noctis::get_upscale_mode() == noctis::UpscaleMode::edge_scale2x,
                      "upscale_mode restored to scale2x");
        ok &= require(noctis::is_crt_shader_enabled() == true,
                      "crt_shader restored to true");
        ok &= require(noctis::get_subpixel_fidelity() == true,
                      "subpixel_fidelity restored to true");
        ok &= require(noctis::is_fullscreen() == true,
                      "fullscreen restored to true");
        ok &= require(noctis::get_timewarp_multiplier() == 250,
                      "timewarp_multiplier restored to 250");
        ok &= require(noctis::get_setting_draw_hud() == 0,
                      "draw_hud restored to 0");
        ok &= require(noctis::get_setting_lens_flare_mode() == -1,
                      "lens_flare_mode restored to -1");
        ok &= require(noctis::get_setting_seamless_border() == 1,
                      "seamless_border restored to 1");
        ok &= require(noctis::get_draw_distance_mode() == noctis::DrawDistanceMode::far,
                      "draw_distance restored to far");
        ok &= require(noctis::get_texture_filter_mode() == noctis::TextureFilterMode::detailed,
                      "texture_filter restored to detailed");

        // 13. Missing file handling
        const auto non_existent = test_dir / "does_not_exist";
        const bool missing_load = noctis::load_display_settings(non_existent);
        ok &= require(!missing_load, "load_display_settings on missing path returns false");
        ok &= require(noctis::get_timewarp_multiplier() == 250, "settings preserved when missing file");

        // 14. Corrupt / partial file handling
        const auto corrupt_file = test_dir / "display_settings.ini";
        {
            std::ofstream out(corrupt_file, std::ios::trunc);
            out << "# Comment\n"
                << "invalid line without equal sign\n"
                << "aspect_ratio = 4:3\n"
                << "upscale_mode = smooth\n"
                << "internal_resolution = 4x\n"
                << "draw_distance = extended\n"
                << "texture_filter = bilinear\n"
                << "timewarp_multiplier = 99999\n" // Clamped to 5000
                << "lens_flare_mode = -99\n";      // Clamped to -1
        }
        const bool corrupt_load = noctis::load_display_settings(test_dir);
        ok &= require(corrupt_load, "corrupt load recovers gracefully");
        ok &= require(noctis::get_aspect_ratio_mode() == noctis::AspectRatioMode::crt_4_3, "aspect_ratio parsed 4:3");
        ok &= require(noctis::get_upscale_mode() == noctis::UpscaleMode::smooth_bilinear, "upscale_mode parsed smooth");
        ok &= require(noctis::get_internal_resolution_mode() == noctis::InternalResolutionMode::res_4x, "internal_resolution parsed 4x");
        ok &= require(noctis::get_draw_distance_mode() == noctis::DrawDistanceMode::extended, "draw_distance parsed extended");
        ok &= require(noctis::get_texture_filter_mode() == noctis::TextureFilterMode::bilinear, "texture_filter parsed bilinear");
        ok &= require(noctis::get_timewarp_multiplier() == 5000, "timewarp_multiplier clamped to 5000");
        ok &= require(noctis::get_setting_lens_flare_mode() == -1, "lens_flare_mode clamped to -1");

        // 15. Internal resolution cycling, scales, names, callback, and ini persistence
        {
            auto res = noctis::InternalResolutionMode::res_1x;
            res = noctis::cycle_internal_resolution_mode(res);
            ok &= require(res == noctis::InternalResolutionMode::res_2x, "res_1x cycles to res_2x");
            res = noctis::cycle_internal_resolution_mode(res);
            ok &= require(res == noctis::InternalResolutionMode::res_4x, "res_2x cycles to res_4x");
            res = noctis::cycle_internal_resolution_mode(res);
            ok &= require(res == noctis::InternalResolutionMode::res_1x, "res_4x cycles to res_1x");

            ok &= require(noctis::internal_resolution_scale(noctis::InternalResolutionMode::res_1x) == 1, "res_1x scale is 1");
            ok &= require(noctis::internal_resolution_scale(noctis::InternalResolutionMode::res_2x) == 2, "res_2x scale is 2");
            ok &= require(noctis::internal_resolution_scale(noctis::InternalResolutionMode::res_4x) == 4, "res_4x scale is 4");

            ok &= require(std::string(noctis::internal_resolution_mode_name(noctis::InternalResolutionMode::res_1x)) ==
                              "INTERNAL RES: 320X200 (1X)", "res_1x name format");
            ok &= require(std::string(noctis::internal_resolution_mode_name(noctis::InternalResolutionMode::res_2x)) ==
                              "INTERNAL RES: 640X400 (2X)", "res_2x name format");
            ok &= require(std::string(noctis::internal_resolution_mode_name(noctis::InternalResolutionMode::res_4x)) ==
                              "INTERNAL RES: 1280X800 (4X)", "res_4x name format");

            static noctis::InternalResolutionMode callback_received = noctis::InternalResolutionMode::res_1x;
            noctis::set_internal_resolution_change_callback([](noctis::InternalResolutionMode m) {
                callback_received = m;
            });

            noctis::set_internal_resolution_mode(noctis::InternalResolutionMode::res_2x);
            ok &= require(callback_received == noctis::InternalResolutionMode::res_2x, "callback fired with res_2x");
            ok &= require(internal_res_scale == 2, "internal_res_scale updated to 2");
            ok &= require(adapted_width == 640, "adapted_width updated to 640");
            ok &= require(adapted_height == 400, "adapted_height updated to 400");

            noctis::set_internal_resolution_mode(noctis::InternalResolutionMode::res_4x);
            ok &= require(callback_received == noctis::InternalResolutionMode::res_4x, "callback fired with res_4x");
            ok &= require(internal_res_scale == 4, "internal_res_scale updated to 4");
            ok &= require(adapted_width == 1280, "adapted_width updated to 1280");
            ok &= require(adapted_height == 800, "adapted_height updated to 800");

            // Save and verify round-trip
            noctis::save_display_settings(test_dir);
            noctis::set_internal_resolution_mode(noctis::InternalResolutionMode::res_1x);
            ok &= require(noctis::get_internal_resolution_mode() == noctis::InternalResolutionMode::res_1x, "reset to 1x");

            noctis::load_display_settings(test_dir);
            ok &= require(noctis::get_internal_resolution_mode() == noctis::InternalResolutionMode::res_4x, "loaded 4x from ini");
            ok &= require(internal_res_scale == 4, "internal_res_scale restored to 4");
            ok &= require(adapted_width == 1280, "adapted_width restored to 1280");
            ok &= require(adapted_height == 800, "adapted_height restored to 800");

            // Clean up: restore to 1x
            noctis::set_internal_resolution_change_callback(nullptr);
            noctis::set_internal_resolution_mode(noctis::InternalResolutionMode::res_1x);
            ok &= require(internal_res_scale == 1, "internal_res_scale restored to 1");
            ok &= require(adapted_width == 320, "adapted_width restored to 320");
            ok &= require(adapted_height == 200, "adapted_height restored to 200");
        }

        // 16. Draw distance cycling, max depth, names, and ini persistence
        {
            auto dist = noctis::DrawDistanceMode::standard;
            dist = noctis::cycle_draw_distance_mode(dist);
            ok &= require(dist == noctis::DrawDistanceMode::extended, "standard cycles to extended");
            dist = noctis::cycle_draw_distance_mode(dist);
            ok &= require(dist == noctis::DrawDistanceMode::far, "extended cycles to far");
            dist = noctis::cycle_draw_distance_mode(dist);
            ok &= require(dist == noctis::DrawDistanceMode::standard, "far cycles to standard");

            ok &= require(noctis::draw_distance_max_depth(noctis::DrawDistanceMode::standard) == 64, "standard max depth is 64");
            ok &= require(noctis::draw_distance_max_depth(noctis::DrawDistanceMode::extended) == 96, "extended max depth is 96");
            ok &= require(noctis::draw_distance_max_depth(noctis::DrawDistanceMode::far) == 128, "far max depth is 128");

            ok &= require(std::string(noctis::draw_distance_mode_name(noctis::DrawDistanceMode::standard)) ==
                              "DRAW DISTANCE: STANDARD (64Q)", "standard name format");
            ok &= require(std::string(noctis::draw_distance_mode_name(noctis::DrawDistanceMode::extended)) ==
                              "DRAW DISTANCE: EXTENDED (96Q)", "extended name format");
            ok &= require(std::string(noctis::draw_distance_mode_name(noctis::DrawDistanceMode::far)) ==
                              "DRAW DISTANCE: FAR (128Q)", "far name format");

            noctis::set_draw_distance_mode(noctis::DrawDistanceMode::extended);
            ok &= require(noctis::get_draw_distance_mode() == noctis::DrawDistanceMode::extended, "set extended");

            // Save and verify round-trip
            noctis::save_display_settings(test_dir);
            noctis::set_draw_distance_mode(noctis::DrawDistanceMode::standard);
            ok &= require(noctis::get_draw_distance_mode() == noctis::DrawDistanceMode::standard, "reset to standard");

            noctis::load_display_settings(test_dir);
            ok &= require(noctis::get_draw_distance_mode() == noctis::DrawDistanceMode::extended, "loaded extended from ini");

            // Clean up: restore to standard
            noctis::set_draw_distance_mode(noctis::DrawDistanceMode::standard);
            ok &= require(noctis::get_draw_distance_mode() == noctis::DrawDistanceMode::standard, "restored to standard");
        }

        // 17. Texture filter cycling, names, getter/setter, and ini persistence
        {
            auto mode = noctis::TextureFilterMode::nearest;
            mode = noctis::cycle_texture_filter_mode(mode);
            ok &= require(mode == noctis::TextureFilterMode::bilinear, "nearest cycles to bilinear");
            mode = noctis::cycle_texture_filter_mode(mode);
            ok &= require(mode == noctis::TextureFilterMode::detailed, "bilinear cycles to detailed");
            mode = noctis::cycle_texture_filter_mode(mode);
            ok &= require(mode == noctis::TextureFilterMode::nearest, "detailed cycles to nearest");

            ok &= require(std::string(noctis::texture_filter_mode_name(noctis::TextureFilterMode::nearest)) ==
                              "TEXTURE FILTER: NEAREST", "nearest name format");
            ok &= require(std::string(noctis::texture_filter_mode_name(noctis::TextureFilterMode::bilinear)) ==
                              "TEXTURE FILTER: BILINEAR", "bilinear name format");
            ok &= require(std::string(noctis::texture_filter_mode_name(noctis::TextureFilterMode::detailed)) ==
                              "TEXTURE FILTER: DETAILED", "detailed name format");

            noctis::set_texture_filter_mode(noctis::TextureFilterMode::detailed);
            ok &= require(noctis::get_texture_filter_mode() == noctis::TextureFilterMode::detailed, "set detailed");

            // Save and verify round-trip
            noctis::save_display_settings(test_dir);
            noctis::set_texture_filter_mode(noctis::TextureFilterMode::nearest);
            ok &= require(noctis::get_texture_filter_mode() == noctis::TextureFilterMode::nearest, "reset to nearest");

            noctis::load_display_settings(test_dir);
            ok &= require(noctis::get_texture_filter_mode() == noctis::TextureFilterMode::detailed, "loaded detailed from ini");

            // Clean up: restore to nearest
            noctis::set_texture_filter_mode(noctis::TextureFilterMode::nearest);
            ok &= require(noctis::get_texture_filter_mode() == noctis::TextureFilterMode::nearest, "restored to nearest");
        }

        // 18. Atmospheric scattering cycling, names, getter/setter, and ini persistence
        {
            auto mode = noctis::AtmosphericScatteringMode::authentic;
            mode = noctis::cycle_atmospheric_scattering_mode(mode);
            ok &= require(mode == noctis::AtmosphericScatteringMode::realistic, "authentic cycles to realistic");
            mode = noctis::cycle_atmospheric_scattering_mode(mode);
            ok &= require(mode == noctis::AtmosphericScatteringMode::vibrant, "realistic cycles to vibrant");
            mode = noctis::cycle_atmospheric_scattering_mode(mode);
            ok &= require(mode == noctis::AtmosphericScatteringMode::authentic, "vibrant cycles to authentic");

            ok &= require(std::string(noctis::atmospheric_scattering_mode_name(noctis::AtmosphericScatteringMode::authentic)) ==
                              "SCATTERING: AUTHENTIC", "authentic name format");
            ok &= require(std::string(noctis::atmospheric_scattering_mode_name(noctis::AtmosphericScatteringMode::realistic)) ==
                              "SCATTERING: REALISTIC", "realistic name format");
            ok &= require(std::string(noctis::atmospheric_scattering_mode_name(noctis::AtmosphericScatteringMode::vibrant)) ==
                              "SCATTERING: VIBRANT", "vibrant name format");

            noctis::set_atmospheric_scattering_mode(noctis::AtmosphericScatteringMode::realistic);
            ok &= require(noctis::get_atmospheric_scattering_mode() == noctis::AtmosphericScatteringMode::realistic, "set realistic");

            // Save and verify round-trip
            noctis::save_display_settings(test_dir);
            noctis::set_atmospheric_scattering_mode(noctis::AtmosphericScatteringMode::authentic);
            ok &= require(noctis::get_atmospheric_scattering_mode() == noctis::AtmosphericScatteringMode::authentic, "reset to authentic");

            noctis::load_display_settings(test_dir);
            ok &= require(noctis::get_atmospheric_scattering_mode() == noctis::AtmosphericScatteringMode::realistic, "loaded realistic from ini");

            // Clean up: restore to authentic
            noctis::set_atmospheric_scattering_mode(noctis::AtmosphericScatteringMode::authentic);
            ok &= require(noctis::get_atmospheric_scattering_mode() == noctis::AtmosphericScatteringMode::authentic, "restored to authentic");
        }

        // 19. Coronal flares cycling, names, getter/setter, and ini persistence
        {
            auto mode = noctis::CoronalFlaresMode::authentic;
            mode = noctis::cycle_coronal_flares_mode(mode);
            ok &= require(mode == noctis::CoronalFlaresMode::realistic, "authentic cycles to realistic");
            mode = noctis::cycle_coronal_flares_mode(mode);
            ok &= require(mode == noctis::CoronalFlaresMode::vibrant, "realistic cycles to vibrant");
            mode = noctis::cycle_coronal_flares_mode(mode);
            ok &= require(mode == noctis::CoronalFlaresMode::authentic, "vibrant cycles to authentic");

            ok &= require(std::string(noctis::coronal_flares_mode_name(noctis::CoronalFlaresMode::authentic)) ==
                              "CORONA FLARES: AUTHENTIC", "authentic name format");
            ok &= require(std::string(noctis::coronal_flares_mode_name(noctis::CoronalFlaresMode::realistic)) ==
                              "CORONA FLARES: REALISTIC", "realistic name format");
            ok &= require(std::string(noctis::coronal_flares_mode_name(noctis::CoronalFlaresMode::vibrant)) ==
                              "CORONA FLARES: VIBRANT", "vibrant name format");

            noctis::set_coronal_flares_mode(noctis::CoronalFlaresMode::realistic);
            ok &= require(noctis::get_coronal_flares_mode() == noctis::CoronalFlaresMode::realistic, "set realistic");

            // Save and verify round-trip
            noctis::save_display_settings(test_dir);
            noctis::set_coronal_flares_mode(noctis::CoronalFlaresMode::authentic);
            ok &= require(noctis::get_coronal_flares_mode() == noctis::CoronalFlaresMode::authentic, "reset to authentic");

            noctis::load_display_settings(test_dir);
            ok &= require(noctis::get_coronal_flares_mode() == noctis::CoronalFlaresMode::realistic, "loaded realistic from ini");

            // Clean up: restore to authentic
            noctis::set_coronal_flares_mode(noctis::CoronalFlaresMode::authentic);
            ok &= require(noctis::get_coronal_flares_mode() == noctis::CoronalFlaresMode::authentic, "restored to authentic");
        }

        std::filesystem::remove_all(test_dir, ec);
    }

    return ok ? 0 : 1;
}
