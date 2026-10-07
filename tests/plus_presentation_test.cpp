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
    const auto adv_menu_1x = noctis::plus_visual_menu_lines(true, 1, false, 0, 0, false, false, true, 0, 0, 0, 0, 0);
    ok &= require(adv_menu_1x.size() == 13 && adv_menu_1x[5] == "INTERNAL RES: 320X200 1X (R)"
                      && adv_menu_1x[6] == "DRAW DISTANCE: STANDARD 64Q (D)"
                      && adv_menu_1x[7] == "TEXTURE FILTER: NEAREST (X)"
                      && adv_menu_1x[8] == "SCATTERING: AUTHENTIC (S)"
                      && adv_menu_1x[9] == "CORONA FLARES: AUTHENTIC (E)",
                  "F2 advanced menu 1x resolution, standard draw distance, nearest filter, authentic scattering, and authentic coronal flares line");
    const auto adv_menu_2x = noctis::plus_visual_menu_lines(true, 1, false, 0, 0, false, false, true, 1, 1, 1, 1, 1);
    ok &= require(adv_menu_2x.size() == 13 && adv_menu_2x[5] == "INTERNAL RES: 640X400 2X (R)"
                      && adv_menu_2x[6] == "DRAW DISTANCE: EXTENDED 96Q (D)"
                      && adv_menu_2x[7] == "TEXTURE FILTER: BILINEAR (X)"
                      && adv_menu_2x[8] == "SCATTERING: REALISTIC (S)"
                      && adv_menu_2x[9] == "CORONA FLARES: REALISTIC (E)",
                  "F2 advanced menu 2x resolution, extended draw distance, bilinear filter, realistic scattering, and realistic coronal flares line");
    const auto adv_menu_4x = noctis::plus_visual_menu_lines(true, 1, false, 0, 0, false, false, true, 2, 2, 2, 2, 2);
    ok &= require(adv_menu_4x.size() == 13 && adv_menu_4x[5] == "INTERNAL RES: 1280X800 4X (R)"
                      && adv_menu_4x[6] == "DRAW DISTANCE: FAR 128Q (D)"
                      && adv_menu_4x[7] == "TEXTURE FILTER: DETAILED (X)"
                      && adv_menu_4x[8] == "SCATTERING: VIBRANT (S)"
                      && adv_menu_4x[9] == "CORONA FLARES: VIBRANT (E)",
                  "F2 advanced menu 4x resolution, far draw distance, detailed filter, vibrant scattering, and vibrant coronal flares line");
    const auto movie = noctis::plus_movie_menu_lines(7, 3, false, true, false, false, 0.0);
    const auto recording = noctis::plus_movie_menu_lines(7, 3, true, false, true, false, 12.5);
    ok &= require(movie.size() == 8 && movie[1].find("007 EXISTS") != std::string::npos
                      && movie[2].find("003") != std::string::npos
                      && movie[4] == "START RECORDING (ENTER)"
                      && movie[5].find("PLAY DECK") != std::string::npos
                      && movie[5].find("EXPORT") != std::string::npos
                      && movie[6].find("LEFT/RIGHT STEP") != std::string::npos
                      && movie[7].find("ESC: CLOSE") != std::string::npos
                      && recording[3] == "BLACK FLASH WHEN CAPTURING (F)"
                      && recording[4].find("FPS 12.50") != std::string::npos,
                  "F3 menu state text changed");
    ok &= require(surface[2].find("TORCH") != std::string_view::npos,
                  "F1 surface help should include torch");

    // F2 Page 2: Audio Volume Controls presentation test
    const auto audio_menu = noctis::plus_audio_menu_lines(0, 0.8f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f, false, "HYBRID", "Stellaria");
    ok &= require(audio_menu.size() == 10, "audio menu line count");
    ok &= require(audio_menu[0] == "NOCTIS IV OM AUDIO VOLUME SETTINGS", "audio menu title");
    ok &= require(audio_menu[1].find("> 1. MASTER:") != std::string::npos, "audio menu line 1 selected pointer");
    ok &= require(audio_menu[1].find("80%") != std::string::npos, "audio menu line 1 percentage");
    ok &= require(audio_menu[2].find("2. MUSIC:") != std::string::npos, "audio menu line 2 music");
    ok &= require(audio_menu[3].find("3. CABIN:") != std::string::npos, "audio menu line 3 cabin");
    ok &= require(audio_menu[7].find("MODE: HYBRID") != std::string::npos, "audio menu mode line");
    ok &= require(audio_menu[7].find("Stellaria") != std::string::npos, "audio menu current track name");
    ok &= require(audio_menu[8].find("ACTIVE") != std::string::npos, "audio menu active status");
    ok &= require(audio_menu[9].find("TAB: CONTROLS") != std::string::npos, "audio menu footer");

    const auto audio_muted = noctis::plus_audio_menu_lines(1, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, true);
    ok &= require(audio_muted.size() == 10, "audio menu 7-arg overload line count");
    ok &= require(audio_muted[8].find("MUTED") != std::string::npos, "audio menu muted status");

    // F2 Page 3: Controls & Input Settings presentation test
    const auto ctrl_menu = noctis::plus_controls_menu_lines(false, 1.0f, 1, "W", "S", "A", "D");
    ok &= require(ctrl_menu.size() == 8, "controls menu line count");
    ok &= require(ctrl_menu[0] == "NOCTIS IV OM CONTROLS & INPUT SETTINGS", "controls menu title");
    ok &= require(ctrl_menu[1].find("NORMAL (LOOK UP)") != std::string::npos, "controls menu normal inversion");
    ok &= require(ctrl_menu[2].find("1.0X") != std::string::npos, "controls menu sensitivity");
    ok &= require(ctrl_menu[3].find("ON (ALWAYS)") != std::string::npos, "controls menu mouselook mode");
    ok &= require(ctrl_menu[4].find("MOVE: W A S D") != std::string::npos, "controls menu keybindings");
    ok &= require(ctrl_menu[5].find("GAMEPAD:") != std::string::npos, "controls menu gamepad line");
    ok &= require(ctrl_menu[6].find("RUMBLE HAPTICS:") != std::string::npos, "controls menu rumble line");
    ok &= require(ctrl_menu[7].find("TAB: VIDEO") != std::string::npos, "controls menu footer");

    const auto ctrl_inverted = noctis::plus_controls_menu_lines(true, 2.5f, 0, "W", "S", "A", "D");
    ok &= require(ctrl_inverted[1].find("INVERTED (LOOK DOWN)") != std::string::npos, "controls menu inverted");
    ok &= require(ctrl_inverted[2].find("2.5X") != std::string::npos, "controls menu sensitivity 2.5X");
    ok &= require(ctrl_inverted[3].find("OFF (HOLD RMB)") != std::string::npos, "controls menu mouselook off");

    // Suit torch presentation test
    std::vector<std::uint8_t> test_frame(320 * 200, 0);
    test_frame[145 * 320 + 160] = 0;   // Dark terrain at center of beam
    test_frame[10 * 320 + 10] = 5;     // Outside the beam
    test_frame[145 * 320 + 161] = 70;  // Sky/stars pixel in beam area
    test_frame[145 * 320 + 162] = 192; // Object/vegetation in beam area
    test_frame[100 * 320 + 160] = 30;  // Distant mountain near horizon

    // Torch inactive: no changes
    noctis::apply_suit_torch(test_frame.data(), 320, 200, false);
    ok &= require(test_frame[145 * 320 + 160] == 0, "torch inactive should not modify pixels");

    // Torch active: illuminates dark ground and objects, preserves sky, distant mountains, and outside pixels
    noctis::apply_suit_torch(test_frame.data(), 320, 200, true);
    ok &= require(test_frame[145 * 320 + 160] >= 10, "torch active should illuminate center ground");
    ok &= require(test_frame[100 * 320 + 160] == 30, "torch active should not illuminate distant mountain");
    ok &= require(test_frame[10 * 320 + 10] == 5, "torch active should not modify outside pixels");
    ok &= require(test_frame[145 * 320 + 161] == 70, "torch active should preserve sky/stars");
    ok &= require(test_frame[145 * 320 + 162] > 192, "torch active should illuminate objects");

    // Suit torch RGBA spotlight presentation test
    std::vector<std::uint8_t> test_rgba(320 * 200 * 4, 16);
    std::vector<std::uint8_t> test_indices(320 * 200, 0);
    test_indices[145 * 320 + 160] = 0;   // Ground at hotspot
    test_indices[145 * 320 + 161] = 70;  // Sky in beam area
    test_indices[145 * 320 + 162] = 195; // Object in beam area
    test_indices[100 * 320 + 160] = 32;  // Distant mountain 1 km away
    test_indices[5 * 320 + 160] = 0;     // Visor top margin
    test_indices[10 * 320 + 10] = 0;     // Outside beam

    noctis::apply_suit_torch_rgba(test_rgba.data(), test_indices.data(), 320, 200);

    const std::size_t hotspot_offset = (145 * 320 + 160) * 4;
    const std::size_t sky_offset = (145 * 320 + 161) * 4;
    const std::size_t obj_offset = (145 * 320 + 162) * 4;
    const std::size_t mountain_offset = (100 * 320 + 160) * 4;
    const std::size_t visor_offset = (5 * 320 + 160) * 4;
    const std::size_t outside_offset = (10 * 320 + 10) * 4;

    ok &= require(test_rgba[hotspot_offset + 0] > 40, "RGBA torch should illuminate ground hotspot without washed-out blowout");
    ok &= require(test_rgba[mountain_offset + 0] == 16, "RGBA torch should not light up mountain 1 km away");
    ok &= require(test_rgba[sky_offset + 0] == 16, "RGBA torch should preserve sky pixels");
    ok &= require(test_rgba[obj_offset + 0] > 40, "RGBA torch should illuminate objects in beam");
    ok &= require(test_rgba[visor_offset + 0] == 16, "RGBA torch should preserve visor margin");
    ok &= require(test_rgba[outside_offset + 0] == 16, "RGBA torch should not modify pixels outside beam");

    // Lit surface washout prevention test: verify torch adds soft fill without clipping to 255
    std::vector<std::uint8_t> lit_rgba(320 * 200 * 4, 180);
    std::vector<std::uint8_t> lit_indices(320 * 200, 10);
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
