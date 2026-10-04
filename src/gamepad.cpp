#include "gamepad.h"
#include "controls_config.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace noctis {
namespace {

const GamepadState *s_mock_gamepad_state = nullptr;

float s_active_rumble_left     = 0.0F;
float s_active_rumble_right    = 0.0F;
float s_active_rumble_duration = 0.0F;

float normalize_trigger(float raw) {
    if (raw <= -0.95F) return 0.0F;
    if (raw < 0.0F) return std::clamp((raw + 1.0F) * 0.5F, 0.0F, 1.0F);
    return std::clamp(raw, 0.0F, 1.0F);
}

} // namespace

float apply_axis_deadzone(float raw_value, float deadzone) {
    const float mag = std::abs(raw_value);
    if (mag <= deadzone) return 0.0F;
    const float scaled = (mag - deadzone) / (1.0F - deadzone);
    const float sign   = (raw_value > 0.0F) ? 1.0F : -1.0F;
    return sign * std::clamp(scaled, 0.0F, 1.0F);
}

bool is_gamepad_connected(int gamepad_index) {
    if (s_mock_gamepad_state) {
        return s_mock_gamepad_state->connected;
    }
    return IsGamepadAvailable(gamepad_index);
}

std::string get_gamepad_name(int gamepad_index) {
    if (s_mock_gamepad_state) {
        return s_mock_gamepad_state->name;
    }
    if (!IsGamepadAvailable(gamepad_index)) {
        return {};
    }
    const char *name = GetGamepadName(gamepad_index);
    return name ? std::string(name) : std::string("GAMEPAD");
}

GamepadState poll_gamepad_state(int gamepad_index) {
    if (s_mock_gamepad_state) {
        return *s_mock_gamepad_state;
    }

    GamepadState state;
    if (!IsGamepadAvailable(gamepad_index)) {
        return state;
    }

    state.connected = true;
    const char *name = GetGamepadName(gamepad_index);
    state.name = name ? std::string(name) : std::string("GAMEPAD");

    const auto &settings = get_controls_settings();
    const float deadzone = settings.gamepad_deadzone;

    // Read Left Stick (movement)
    const float raw_lx = GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_LEFT_X);
    const float raw_ly = GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_LEFT_Y);
    state.left_stick_x = apply_axis_deadzone(raw_lx, deadzone);
    state.left_stick_y = apply_axis_deadzone(-raw_ly, deadzone); // Negative Y is stick pushed forward/up

    // Read Right Stick (camera look / pitch / yaw)
    const float raw_rx = GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_RIGHT_X);
    const float raw_ry = GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_RIGHT_Y);
    state.right_stick_x = apply_axis_deadzone(raw_rx, deadzone);

    // Negative Y is forward/up. Invert pitch toggle inverts this direction.
    const bool invert_pitch = settings.gamepad_invert_y || settings.invert_mouse_y;
    const float pitch_dir   = invert_pitch ? 1.0F : -1.0F;
    state.right_stick_y     = apply_axis_deadzone(raw_ry * pitch_dir, deadzone);

    // Read Analog Triggers
    const float raw_lt = GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_LEFT_TRIGGER);
    const float raw_rt = GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_RIGHT_TRIGGER);
    state.left_trigger  = normalize_trigger(raw_lt);
    state.right_trigger = normalize_trigger(raw_rt);

    if (IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_TRIGGER_2)) {
        state.left_trigger = std::max(state.left_trigger, 1.0F);
    }
    if (IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
        state.right_trigger = std::max(state.right_trigger, 1.0F);
    }

    // Read Digital Buttons (held down)
    state.button_a     = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    state.button_b     = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    state.button_x     = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    state.button_y     = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    state.bumper_left  = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
    state.bumper_right = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    state.dpad_up      = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_UP);
    state.dpad_down    = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    state.dpad_left    = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    state.dpad_right   = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    state.start        = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    state.back         = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_MIDDLE_LEFT);
    state.left_thumb   = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_LEFT_THUMB);
    state.right_thumb  = IsGamepadButtonDown(gamepad_index, GAMEPAD_BUTTON_RIGHT_THUMB);

    // Read Edge-Triggered Buttons (newly pressed this frame)
    state.button_a_pressed     = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    state.button_b_pressed     = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    state.button_x_pressed     = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    state.button_y_pressed     = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    state.bumper_left_pressed  = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
    state.bumper_right_pressed = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    state.dpad_up_pressed      = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_UP);
    state.dpad_down_pressed    = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    state.dpad_left_pressed    = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    state.dpad_right_pressed   = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    state.start_pressed        = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    state.back_pressed         = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_MIDDLE_LEFT);
    state.left_thumb_pressed   = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_THUMB);
    state.right_thumb_pressed  = IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_THUMB);

    return state;
}

void trigger_gamepad_rumble(GamepadRumbleType type, float strength, int gamepad_index) {
    const auto &settings = get_controls_settings();
    if (!settings.rumble_enabled) return;
    if (!s_mock_gamepad_state && !IsGamepadAvailable(gamepad_index)) return;

    float left_motor  = 0.0F;
    float right_motor = 0.0F;
    float duration    = 0.0F;

    switch (type) {
    case GamepadRumbleType::touchdown:
        left_motor  = 0.70F;
        right_motor = 0.40F;
        duration    = 0.35F;
        break;
    case GamepadRumbleType::rcs_burst:
        left_motor  = 0.15F;
        right_motor = 0.45F;
        duration    = 0.12F;
        break;
    case GamepadRumbleType::jetpack:
        left_motor  = 0.25F;
        right_motor = 0.30F;
        duration    = 0.18F;
        break;
    case GamepadRumbleType::buffeting:
        left_motor  = 0.50F;
        right_motor = 0.20F;
        duration    = 0.15F;
        break;
    case GamepadRumbleType::warp:
        left_motor  = 0.60F;
        right_motor = 0.70F;
        duration    = 0.50F;
        break;
    case GamepadRumbleType::click:
        left_motor  = 0.00F;
        right_motor = 0.20F;
        duration    = 0.06F;
        break;
    }

    const float s = std::clamp(strength, 0.0F, 1.0F);
    left_motor    = std::clamp(left_motor * s, 0.0F, 1.0F);
    right_motor   = std::clamp(right_motor * s, 0.0F, 1.0F);

    s_active_rumble_left     = left_motor;
    s_active_rumble_right    = right_motor;
    s_active_rumble_duration = duration;

#ifdef __EMSCRIPTEN__
    if (!s_mock_gamepad_state && IsGamepadAvailable(gamepad_index)) {
        SetGamepadVibration(gamepad_index, left_motor, right_motor, duration);
    }
#endif
}

void update_gamepad_rumble(const AudioTelemetry &telemetry, int gamepad_index) {
    const auto &settings = get_controls_settings();
    if (!settings.rumble_enabled) return;

    if (telemetry.entry_buffeting > 0.05F) {
        trigger_gamepad_rumble(GamepadRumbleType::buffeting, telemetry.entry_buffeting, gamepad_index);
    } else if (telemetry.jetpack_active) {
        trigger_gamepad_rumble(GamepadRumbleType::jetpack, 0.6F, gamepad_index);
    }
}

void stop_gamepad_rumble(int gamepad_index) {
    s_active_rumble_left     = 0.0F;
    s_active_rumble_right    = 0.0F;
    s_active_rumble_duration = 0.0F;

#ifdef __EMSCRIPTEN__
    if (!s_mock_gamepad_state && IsGamepadAvailable(gamepad_index)) {
        SetGamepadVibration(gamepad_index, 0.0F, 0.0F, 0.0F);
    }
#endif
}

void get_gamepad_rumble_state(float &left_motor, float &right_motor, float &duration, int /*gamepad_index*/) {
    left_motor  = s_active_rumble_left;
    right_motor = s_active_rumble_right;
    duration    = s_active_rumble_duration;
}

void reset_gamepad_rumble() {
    s_active_rumble_left     = 0.0F;
    s_active_rumble_right    = 0.0F;
    s_active_rumble_duration = 0.0F;
}

void set_mock_gamepad_state(const GamepadState *mock_state) {
    s_mock_gamepad_state = mock_state;
}

const GamepadState *get_mock_gamepad_state() {
    return s_mock_gamepad_state;
}

} // namespace noctis
