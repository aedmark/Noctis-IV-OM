#include "plus_presentation.h"

#include <cstdio>
#include <vector>

namespace {
bool require(bool value, const char *message) {
    if (!value) std::fprintf(stderr, "plus presentation: %s\n", message);
    return value;
}
}

int main() {
    bool ok = true;
    ok &= require(noctis::format_radius(5.021F) == "RADIUS: 5.0210 CENTIDYAMS",
                  "target radius formatting changed");
    ok &= require(noctis::cycle_lens_flare_mode(0) == 1
                      && noctis::cycle_lens_flare_mode(1) == -1
                      && noctis::cycle_lens_flare_mode(-1) == 0,
                  "lens-flare mode cycle changed");
    ok &= require(noctis::lens_flare_on_hud(true, 0)
                      && !noctis::lens_flare_on_hud(false, 0)
                      && noctis::lens_flare_on_hud(false, 1)
                      && !noctis::lens_flare_on_hud(true, -1),
                  "lens-flare override changed");
    ok &= require(noctis::visible_surface_objects(9, 16) == 9
                      && noctis::visible_surface_objects(9, 255) == 9
                      && noctis::visible_surface_objects(9, 256) == 4,
                  "extended surface viewfield threshold changed");
    const auto &space = noctis::plus_help_lines(false);
    const auto &surface = noctis::plus_help_lines(true);
    ok &= require(space.size() == 4 && surface.size() == 4
                      && space[1].find("RAW SNAPSHOT") != std::string_view::npos
                      && surface[1].find("JETPACK") != std::string_view::npos,
                  "F1 help content is incomplete");
    const auto menu = noctis::plus_visual_menu_lines(false, -1, true);
    ok &= require(menu.size() == 4 && menu[1] == "HUD TEXT OFF (T)"
                      && menu[2] == "LENS FLARES ALWAYS OFF (F)"
                      && menu[3] == "SEAMLESS BORDER (B)",
                  "F2 menu state text changed");
    const auto movie = noctis::plus_movie_menu_lines(7, 3, false, true, false, false, 0.0);
    const auto recording = noctis::plus_movie_menu_lines(7, 3, true, false, true, false, 12.5);
    ok &= require(movie.size() == 5 && movie[1].find("007 EXISTS") != std::string::npos
                      && movie[2].find("003") != std::string::npos
                      && movie[4] == "START RECORDING (ENTER)"
                      && recording[3] == "BLACK FLASH WHEN CAPTURING (F)"
                      && recording[4].find("FPS 12.50") != std::string::npos,
                  "F3 menu state text changed");
    ok &= require(surface[2].find("TORCH") != std::string_view::npos,
                  "F1 surface help should include torch");

    // Suit torch presentation test
    std::vector<std::uint8_t> test_frame(320 * 200, 0);
    test_frame[108 * 320 + 160] = 0;   // Dark terrain at center of beam
    test_frame[10 * 320 + 10] = 5;     // Outside the beam
    test_frame[108 * 320 + 161] = 70;  // Sky/stars pixel in beam area
    test_frame[108 * 320 + 162] = 192; // Object/vegetation in beam area

    // Torch inactive: no changes
    noctis::apply_suit_torch(test_frame.data(), 320, 200, false);
    ok &= require(test_frame[108 * 320 + 160] == 0, "torch inactive should not modify pixels");

    // Torch active: illuminates dark ground and objects, preserves sky and outside pixels
    noctis::apply_suit_torch(test_frame.data(), 320, 200, true);
    ok &= require(test_frame[108 * 320 + 160] >= 25, "torch active should illuminate center ground");
    ok &= require(test_frame[10 * 320 + 10] == 5, "torch active should not modify outside pixels");
    ok &= require(test_frame[108 * 320 + 161] == 70, "torch active should preserve sky/stars");
    ok &= require(test_frame[108 * 320 + 162] > 192, "torch active should illuminate objects");

    // Suit torch RGBA spotlight presentation test
    std::vector<std::uint8_t> test_rgba(320 * 200 * 4, 16);
    std::vector<std::uint8_t> test_indices(320 * 200, 0);
    test_indices[110 * 320 + 160] = 0;   // Ground at hotspot
    test_indices[110 * 320 + 161] = 70;  // Sky in beam area
    test_indices[110 * 320 + 162] = 195; // Object in beam area
    test_indices[5 * 320 + 160] = 0;     // Visor top margin
    test_indices[10 * 320 + 10] = 0;     // Outside beam

    noctis::apply_suit_torch_rgba(test_rgba.data(), test_indices.data(), 320, 200);

    const std::size_t hotspot_offset = (110 * 320 + 160) * 4;
    const std::size_t sky_offset = (110 * 320 + 161) * 4;
    const std::size_t obj_offset = (110 * 320 + 162) * 4;
    const std::size_t visor_offset = (5 * 320 + 160) * 4;
    const std::size_t outside_offset = (10 * 320 + 10) * 4;

    ok &= require(test_rgba[hotspot_offset + 0] > 150, "RGBA torch should brightly illuminate ground hotspot");
    ok &= require(test_rgba[sky_offset + 0] == 16, "RGBA torch should preserve sky pixels");
    ok &= require(test_rgba[obj_offset + 0] > 150, "RGBA torch should brightly illuminate objects in beam");
    ok &= require(test_rgba[visor_offset + 0] == 16, "RGBA torch should preserve visor margin");
    ok &= require(test_rgba[outside_offset + 0] == 16, "RGBA torch should not modify pixels outside beam");

    // Lit surface washout prevention test: verify torch adds soft fill without clipping to 255
    std::vector<std::uint8_t> lit_rgba(320 * 200 * 4, 180);
    std::vector<std::uint8_t> lit_indices(320 * 200, 30);
    noctis::apply_suit_torch_rgba(lit_rgba.data(), lit_indices.data(), 320, 200);
    ok &= require(lit_rgba[hotspot_offset + 0] < 240, "torch on lit surface should not blow out to 255");
    ok &= require(lit_rgba[hotspot_offset + 0] > 180, "torch on lit surface should still add subtle fill light");

    // Surface HUD status text test
    std::vector<std::uint8_t> hud_frame(320 * 200, 0);
    noctis::draw_surface_status_text(hud_frame.data(), 320, 200, "TORCH ON");

    bool found_white = false;
    bool found_shadow = false;
    for (int y = 98; y <= 106; ++y) {
        for (int x = 120; x <= 200; ++x) {
            const auto val = hud_frame[y * 320 + x];
            if (val == 127) found_white = true;
            if (val == 64) found_shadow = true;
        }
    }
    ok &= require(found_white, "HUD status text should render crisp star-white glyph pixels (index 127)");
    ok &= require(found_shadow, "HUD status text should render drop shadow pixels (index 64)");

    return ok ? 0 : 1;
}
