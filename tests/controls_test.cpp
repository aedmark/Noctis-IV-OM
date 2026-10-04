#include "controls_config.h"
#include "plus_presentation.h"

#include <raylib.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "controls test failure: %s\n", message);
    }
    return condition;
}
} // namespace

int main() {
    bool ok = true;

    // 1. Verify default controls settings
    // Default pitch behavior: pushing mouse forward pitches camera UP (invert_mouse_y == false)
    noctis::reset_controls_settings();
    const auto &defaults = noctis::get_controls_settings();
    ok &= require(!defaults.invert_mouse_y, "mouse pitch inversion must default to false (push forward = pitch up)");
    ok &= require(!noctis::is_mouse_inversion_active(), "is_mouse_inversion_active must be false by default");
    ok &= require(std::fabs(defaults.mouse_sensitivity - 1.0f) < 0.001f, "default mouse sensitivity must be 1.0");
    ok &= require(defaults.mouselook_mode == 1, "default mouselook mode must be 1 (mouselook on)");
    ok &= require(defaults.key_forward == KEY_W, "default key_forward must be KEY_W");
    ok &= require(defaults.key_backward == KEY_S, "default key_backward must be KEY_S");
    ok &= require(defaults.key_left == KEY_A, "default key_left must be KEY_A");
    ok &= require(defaults.key_right == KEY_D, "default key_right must be KEY_D");
    ok &= require(defaults.key_jump == KEY_SPACE, "default key_jump must be KEY_SPACE");

    // 2. Test toggle_mouse_inversion and set_mouse_inversion
    bool inv = noctis::toggle_mouse_inversion();
    ok &= require(inv, "toggle_mouse_inversion should switch to true");
    ok &= require(noctis::is_mouse_inversion_active(), "is_mouse_inversion_active should be true");
    inv = noctis::toggle_mouse_inversion();
    ok &= require(!inv, "toggle_mouse_inversion should switch back to false");
    ok &= require(!noctis::is_mouse_inversion_active(), "is_mouse_inversion_active should be false");

    noctis::set_mouse_inversion(true);
    ok &= require(noctis::is_mouse_inversion_active(), "set_mouse_inversion(true) should activate inversion");
    noctis::set_mouse_inversion(false);
    ok &= require(!noctis::is_mouse_inversion_active(), "set_mouse_inversion(false) should deactivate inversion");

    // 3. Test mouse sensitivity adjustment and bounds clamping
    noctis::set_mouse_sensitivity(1.5f);
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 1.5f) < 0.001f, "set_mouse_sensitivity should be 1.5");
    noctis::adjust_mouse_sensitivity(0.3f);
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 1.8f) < 0.001f, "adjust_mouse_sensitivity(+0.3) should be 1.8");
    noctis::adjust_mouse_sensitivity(-0.5f);
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 1.3f) < 0.001f, "adjust_mouse_sensitivity(-0.5) should be 1.3");

    // Clamp upper bound (5.0)
    noctis::set_mouse_sensitivity(10.0f);
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 5.0f) < 0.001f, "sensitivity should clamp to 5.0 max");

    // Clamp lower bound (0.1)
    noctis::set_mouse_sensitivity(0.01f);
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 0.1f) < 0.001f, "sensitivity should clamp to 0.1 min");

    // 4. Test key code and key name mapping
    ok &= require(noctis::key_code_from_name("W") == KEY_W, "W should map to KEY_W");
    ok &= require(noctis::key_code_from_name("w") == KEY_W, "w should map to KEY_W case-insensitively");
    ok &= require(noctis::key_code_from_name("SPACE") == KEY_SPACE, "SPACE should map to KEY_SPACE");
    ok &= require(noctis::key_code_from_name("space") == KEY_SPACE, "space should map to KEY_SPACE");
    ok &= require(noctis::key_code_from_name("UP") == KEY_UP, "UP should map to KEY_UP");
    ok &= require(noctis::key_code_from_name("DOWN") == KEY_DOWN, "DOWN should map to KEY_DOWN");
    ok &= require(noctis::key_code_from_name("LEFT") == KEY_LEFT, "LEFT should map to KEY_LEFT");
    ok &= require(noctis::key_code_from_name("RIGHT") == KEY_RIGHT, "RIGHT should map to KEY_RIGHT");
    ok &= require(noctis::key_code_from_name("NONEXISTENT_KEY") == 0, "unknown key should map to 0");

    ok &= require(noctis::key_name_from_code(KEY_W) == "W", "KEY_W should map to W");
    ok &= require(noctis::key_name_from_code(KEY_SPACE) == "SPACE", "KEY_SPACE should map to SPACE");
    ok &= require(noctis::key_name_from_code(KEY_UP) == "UP", "KEY_UP should map to UP");
    ok &= require(noctis::key_name_from_code(999999) == "UNKNOWN", "invalid key should map to UNKNOWN");

    // 5. Test INI persistence round-trip
    const auto test_dir = std::filesystem::temp_directory_path() / "noctis_controls_test_dir";
    std::error_code ec;
    std::filesystem::remove_all(test_dir, ec);
    std::filesystem::create_directories(test_dir, ec);

    // Set custom settings
    noctis::ControlsSettings custom;
    custom.invert_mouse_y = true;
    custom.mouse_sensitivity = 2.5f;
    custom.mouselook_mode = 2;
    custom.key_forward = KEY_UP;
    custom.key_backward = KEY_DOWN;
    custom.key_left = KEY_LEFT;
    custom.key_right = KEY_RIGHT;
    custom.key_jump = KEY_ENTER;
    noctis::set_controls_settings(custom);

    ok &= require(noctis::save_controls_settings(test_dir), "save_controls_settings should succeed");
    ok &= require(std::filesystem::exists(test_dir / "settings.ini"), "settings.ini should exist");
    ok &= require(std::filesystem::exists(test_dir / "control_settings.ini"), "control_settings.ini should exist");

    // Reset settings in memory to defaults
    noctis::reset_controls_settings();
    ok &= require(!noctis::is_mouse_inversion_active(), "settings should be reset");
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 1.0f) < 0.001f, "sensitivity should be reset");

    // Load from settings.ini
    ok &= require(noctis::load_controls_settings(test_dir), "load_controls_settings should succeed");
    ok &= require(noctis::is_mouse_inversion_active(), "loaded invert_mouse_y should be true");
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 2.5f) < 0.001f, "loaded sensitivity should be 2.5");
    const auto &loaded = noctis::get_controls_settings();
    ok &= require(loaded.mouselook_mode == 2, "loaded mouselook_mode should be 2");
    ok &= require(loaded.key_forward == KEY_UP, "loaded key_forward should be KEY_UP");
    ok &= require(loaded.key_backward == KEY_DOWN, "loaded key_backward should be KEY_DOWN");
    ok &= require(loaded.key_left == KEY_LEFT, "loaded key_left should be KEY_LEFT");
    ok &= require(loaded.key_right == KEY_RIGHT, "loaded key_right should be KEY_RIGHT");
    ok &= require(loaded.key_jump == KEY_ENTER, "loaded key_jump should be KEY_ENTER");

    // Test fallback to control_settings.ini when settings.ini is deleted
    std::filesystem::remove(test_dir / "settings.ini", ec);
    noctis::reset_controls_settings();
    ok &= require(noctis::load_controls_settings(test_dir), "loading from fallback control_settings.ini should succeed");
    ok &= require(noctis::is_mouse_inversion_active(), "fallback loaded invert_mouse_y should be true");
    ok &= require(std::fabs(noctis::get_mouse_sensitivity() - 2.5f) < 0.001f, "fallback loaded sensitivity should be 2.5");

    // 6. Test menu presentation formatting
    const auto lines_normal = noctis::plus_controls_menu_lines(false, 1.0f, 1, "W", "S", "A", "D");
    ok &= require(lines_normal.size() == 8, "controls menu should have 8 lines");
    ok &= require(lines_normal[1].find("NORMAL (LOOK UP)") != std::string::npos, "presentation should show NORMAL");
    ok &= require(lines_normal[2].find("1.0X") != std::string::npos, "presentation should show 1.0X");
    ok &= require(lines_normal[4].find("MOVE: W A S D") != std::string::npos, "presentation should show WASD");
    ok &= require(lines_normal[7].find("R: RUMBLE") != std::string::npos, "presentation should show R: RUMBLE footer");

    const auto lines_inverted = noctis::plus_controls_menu_lines(true, 3.2f, 0, "UP", "DOWN", "LEFT", "RIGHT");
    ok &= require(lines_inverted[1].find("INVERTED (LOOK DOWN)") != std::string::npos, "presentation should show INVERTED");
    ok &= require(lines_inverted[2].find("3.2X") != std::string::npos, "presentation should show 3.2X");
    ok &= require(lines_inverted[4].find("MOVE: UP LEFT DOWN RIGHT") != std::string::npos, "presentation should show arrow keys");

    // Cleanup
    std::filesystem::remove_all(test_dir, ec);

    if (ok) {
        std::printf("controls_test: all assertions passed.\n");
        return 0;
    }
    return 1;
}
