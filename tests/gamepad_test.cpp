#include "gamepad.h"
#include "controls_config.h"
#include "input.h"
#include "plus_presentation.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "gamepad test failure: %s\n", message);
    }
    return condition;
}
} // namespace

int main() {
    bool ok = true;

    // 1. Deadzone filtering tests
    const float deadzone = 0.15F;
    ok &= require(noctis::apply_axis_deadzone(0.0F, deadzone) == 0.0F, "center axis should be 0");
    ok &= require(noctis::apply_axis_deadzone(0.10F, deadzone) == 0.0F, "sub-deadzone positive should be 0");
    ok &= require(noctis::apply_axis_deadzone(-0.12F, deadzone) == 0.0F, "sub-deadzone negative should be 0");
    ok &= require(noctis::apply_axis_deadzone(0.15F, deadzone) == 0.0F, "exact deadzone edge should be 0");

    const float scaled_half = noctis::apply_axis_deadzone(0.575F, deadzone); // halfway past deadzone
    ok &= require(std::fabs(scaled_half - 0.5F) < 0.01F, "halfway past deadzone should scale to 0.5");

    const float scaled_full = noctis::apply_axis_deadzone(1.0F, deadzone);
    ok &= require(std::fabs(scaled_full - 1.0F) < 0.001F, "full positive stick should scale to 1.0");

    const float scaled_neg = noctis::apply_axis_deadzone(-1.0F, deadzone);
    ok &= require(std::fabs(scaled_neg - -1.0F) < 0.001F, "full negative stick should scale to -1.0");

    // 2. Default controls & gamepad settings
    noctis::reset_controls_settings();
    const auto &defaults = noctis::get_controls_settings();
    ok &= require(std::fabs(defaults.gamepad_sensitivity - 1.0F) < 0.001F, "default gamepad sensitivity should be 1.0");
    ok &= require(std::fabs(defaults.gamepad_deadzone - 0.15F) < 0.001F, "default gamepad deadzone should be 0.15");
    ok &= require(!defaults.gamepad_invert_y, "default gamepad_invert_y should be false");
    ok &= require(defaults.rumble_enabled, "default rumble_enabled should be true");
    ok &= require(noctis::is_rumble_enabled(), "is_rumble_enabled should return true by default");

    // 3. Gamepad settings adjustments & clamping
    noctis::set_gamepad_sensitivity(2.5F);
    ok &= require(std::fabs(noctis::get_gamepad_sensitivity() - 2.5F) < 0.001F, "set_gamepad_sensitivity should be 2.5");
    noctis::adjust_gamepad_sensitivity(0.5F);
    ok &= require(std::fabs(noctis::get_gamepad_sensitivity() - 3.0F) < 0.001F, "adjust_gamepad_sensitivity(+0.5) should be 3.0");
    noctis::set_gamepad_sensitivity(10.0F);
    ok &= require(std::fabs(noctis::get_gamepad_sensitivity() - 5.0F) < 0.001F, "gamepad sensitivity should clamp to 5.0 max");
    noctis::set_gamepad_sensitivity(0.01F);
    ok &= require(std::fabs(noctis::get_gamepad_sensitivity() - 0.1F) < 0.001F, "gamepad sensitivity should clamp to 0.1 min");

    noctis::set_gamepad_deadzone(0.20F);
    ok &= require(std::fabs(noctis::get_gamepad_deadzone() - 0.20F) < 0.001F, "set_gamepad_deadzone should be 0.20");
    noctis::set_gamepad_deadzone(0.90F);
    ok &= require(std::fabs(noctis::get_gamepad_deadzone() - 0.50F) < 0.001F, "gamepad deadzone should clamp to 0.50 max");
    noctis::set_gamepad_deadzone(0.01F);
    ok &= require(std::fabs(noctis::get_gamepad_deadzone() - 0.05F) < 0.001F, "gamepad deadzone should clamp to 0.05 min");

    noctis::set_rumble_enabled(false);
    ok &= require(!noctis::is_rumble_enabled(), "set_rumble_enabled(false) should disable rumble");
    noctis::toggle_rumble();
    ok &= require(noctis::is_rumble_enabled(), "toggle_rumble should re-enable rumble");

    // 4. Mock gamepad polling & state verification
    noctis::GamepadState mock{};
    mock.connected = true;
    mock.name = "Xbox Wireless Controller";
    mock.left_stick_x = 0.75F;
    mock.left_stick_y = 0.90F;
    mock.right_stick_x = -0.60F;
    mock.right_stick_y = 0.40F;
    mock.right_trigger = 0.85F;
    mock.button_a = true;
    mock.button_a_pressed = true;
    mock.bumper_right_pressed = true;
    mock.dpad_up_pressed = true;
    mock.start_pressed = true;

    noctis::set_mock_gamepad_state(&mock);
    ok &= require(noctis::is_gamepad_connected(0), "is_gamepad_connected should report mock state connected");
    ok &= require(noctis::get_gamepad_name(0) == "Xbox Wireless Controller", "get_gamepad_name should match mock name");

    mock.button_b = true;
    mock.button_b_pressed = true;

    const auto polled = noctis::poll_gamepad_state(0);
    ok &= require(polled.connected, "polled state should be connected");
    ok &= require(polled.left_stick_y == 0.90F, "polled left_stick_y mismatch");
    ok &= require(polled.right_stick_x == -0.60F, "polled right_stick_x mismatch");
    ok &= require(polled.button_a && polled.button_a_pressed, "polled button A mismatch");
    ok &= require(polled.button_b && polled.button_b_pressed, "polled button B mismatch");

    // Verify gamepad button B does NOT emit raw escape_down or exit session
    noctis::reset_input_state();
    noctis::InputFrame btn_b_frame;
    btn_b_frame.mouse_locked = true;
    btn_b_frame.cancel_pressed = true;
    btn_b_frame.escape_down = false;
    btn_b_frame.mouse_right_down = true;
    noctis::apply_input_frame(btn_b_frame);
    ok &= require(!is_key(), "button B cancel must not push raw escape 27 key");
    ok &= require(noctis::is_cancel_requested(), "button B must register cancel request");
    ok &= require(noctis::consume_cancel(), "consume_cancel should return true");
    ok &= require(!noctis::is_cancel_requested(), "cancel request should be cleared after consumption");

    // 5. Rumble feedback haptics tests
    noctis::reset_gamepad_rumble();
    float left = 0.0F, right = 0.0F, duration = 0.0F;

    // Touchdown impact rumble
    noctis::trigger_gamepad_rumble(noctis::GamepadRumbleType::touchdown);
    noctis::get_gamepad_rumble_state(left, right, duration);
    ok &= require(left >= 0.65F && right >= 0.35F && duration >= 0.30F, "touchdown rumble should be heavy impact");

    // RCS attitude burst rumble
    noctis::trigger_gamepad_rumble(noctis::GamepadRumbleType::rcs_burst);
    noctis::get_gamepad_rumble_state(left, right, duration);
    ok &= require(right >= 0.40F && duration <= 0.15F, "rcs burst rumble should be quick high-frequency pulse");

    // Jetpack continuous vibration
    noctis::trigger_gamepad_rumble(noctis::GamepadRumbleType::jetpack);
    noctis::get_gamepad_rumble_state(left, right, duration);
    ok &= require(left >= 0.20F && right >= 0.25F, "jetpack rumble motors should be active");

    // Atmospheric descent buffeting via telemetry update
    noctis::AudioTelemetry tele{};
    tele.entry_buffeting = 0.8F;
    noctis::update_gamepad_rumble(tele);
    noctis::get_gamepad_rumble_state(left, right, duration);
    ok &= require(left >= 0.35F, "entry buffeting should drive low-frequency motor proportional to turbulence");

    // Stop rumble
    noctis::stop_gamepad_rumble();
    noctis::get_gamepad_rumble_state(left, right, duration);
    ok &= require(left == 0.0F && right == 0.0F && duration == 0.0F, "stop_gamepad_rumble should zero motors");

    // Rumble muting test
    noctis::set_rumble_enabled(false);
    noctis::trigger_gamepad_rumble(noctis::GamepadRumbleType::touchdown);
    noctis::get_gamepad_rumble_state(left, right, duration);
    ok &= require(left == 0.0F && right == 0.0F, "rumble should not fire when rumble_enabled is false");
    noctis::set_rumble_enabled(true);

    // 6. Persistence round-trip with gamepad settings
    const auto test_dir = std::filesystem::temp_directory_path() / "noctis_gamepad_test_dir";
    std::error_code ec;
    std::filesystem::remove_all(test_dir, ec);
    std::filesystem::create_directories(test_dir, ec);

    noctis::ControlsSettings custom;
    custom.gamepad_sensitivity = 2.2F;
    custom.gamepad_deadzone = 0.22F;
    custom.gamepad_invert_y = true;
    custom.rumble_enabled = false;
    noctis::set_controls_settings(custom);

    ok &= require(noctis::save_controls_settings(test_dir), "save_controls_settings should succeed with gamepad fields");
    noctis::reset_controls_settings();
    ok &= require(noctis::is_rumble_enabled(), "settings reset should restore rumble_enabled default true");

    ok &= require(noctis::load_controls_settings(test_dir), "load_controls_settings should succeed");
    const auto &loaded = noctis::get_controls_settings();
    ok &= require(std::fabs(loaded.gamepad_sensitivity - 2.2F) < 0.001F, "loaded gamepad_sensitivity mismatch");
    ok &= require(std::fabs(loaded.gamepad_deadzone - 0.22F) < 0.001F, "loaded gamepad_deadzone mismatch");
    ok &= require(loaded.gamepad_invert_y, "loaded gamepad_invert_y should be true");
    ok &= require(!loaded.rumble_enabled, "loaded rumble_enabled should be false");

    // 7. Presentation menu formatting with gamepad info
    const auto menu_connected = noctis::plus_controls_menu_lines(
        false, 1.0F, 1, "W", "S", "A", "D", true, "Xbox Wireless Controller", true);
    ok &= require(menu_connected.size() == 8, "controls menu should have 8 lines with gamepad rows");
    ok &= require(menu_connected[5].find("GAMEPAD: Xbox Wireless Controller (CONNECTED)") != std::string::npos,
                  "menu line 5 should show connected gamepad name");
    ok &= require(menu_connected[6].find("RUMBLE HAPTICS: ACTIVE (R)") != std::string::npos,
                  "menu line 6 should show active rumble");
    ok &= require(menu_connected[7].find("R: RUMBLE") != std::string::npos,
                  "menu footer should include R: RUMBLE");

    const auto menu_disconnected = noctis::plus_controls_menu_lines(
        true, 1.5F, 0, "W", "S", "A", "D", false, {}, false);
    ok &= require(menu_disconnected[5].find("NO CONTROLLER DETECTED") != std::string::npos,
                  "menu should report no controller detected");
    ok &= require(menu_disconnected[6].find("RUMBLE HAPTICS: MUTED (R)") != std::string::npos,
                  "menu should report rumble muted");

    // Cleanup
    noctis::set_mock_gamepad_state(nullptr);
    std::filesystem::remove_all(test_dir, ec);

    if (ok) {
        std::printf("gamepad_test: all assertions passed.\n");
        return 0;
    }
    return 1;
}
