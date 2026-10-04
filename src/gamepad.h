#pragma once

#include "audio.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace noctis {

struct GamepadState {
    bool connected = false;
    std::string name;

    // Analog sticks (-1.0f to +1.0f, with deadzone applied)
    // Left stick: traversal / strafe / propulsion
    float left_stick_x = 0.0F; // -1.0 = full left, +1.0 = full right
    float left_stick_y = 0.0F; // -1.0 = backward, +1.0 = forward

    // Right stick: camera look / pitch / yaw
    float right_stick_x = 0.0F; // -1.0 = turn left, +1.0 = turn right
    float right_stick_y = 0.0F; // -1.0 = look down, +1.0 = look up (or inverted)

    // Analog triggers (0.0f to 1.0f)
    float left_trigger  = 0.0F; // LT (e.g. reverse thruster, brake)
    float right_trigger = 0.0F; // RT (e.g. forward thruster boost, jetpack)

    // Digital buttons (current frame state: held down)
    bool button_a      = false; // Bottom face button (Xbox A / PS Cross)
    bool button_b      = false; // Right face button (Xbox B / PS Circle)
    bool button_x      = false; // Left face button (Xbox X / PS Square)
    bool button_y      = false; // Top face button (Xbox Y / PS Triangle)
    bool bumper_left   = false; // LB / L1
    bool bumper_right  = false; // RB / R1
    bool dpad_up       = false;
    bool dpad_down     = false;
    bool dpad_left     = false;
    bool dpad_right    = false;
    bool start         = false; // Options / Menu / Start
    bool back          = false; // Share / Select / Back
    bool left_thumb    = false; // L3
    bool right_thumb   = false; // R3

    // Button edge triggers (newly pressed in this frame)
    bool button_a_pressed     = false;
    bool button_b_pressed     = false;
    bool button_x_pressed     = false;
    bool button_y_pressed     = false;
    bool bumper_left_pressed  = false;
    bool bumper_right_pressed = false;
    bool dpad_up_pressed      = false;
    bool dpad_down_pressed    = false;
    bool dpad_left_pressed    = false;
    bool dpad_right_pressed   = false;
    bool start_pressed        = false;
    bool back_pressed         = false;
    bool left_thumb_pressed   = false;
    bool right_thumb_pressed  = false;
};

// Rumble haptic vibration types
enum class GamepadRumbleType {
    touchdown,    // Heavy solid landing impact
    rcs_burst,    // Quick sharp pulse from RCS cold-gas thrusters
    jetpack,      // Sustained thruster vibration
    buffeting,    // Low-frequency atmospheric descent turbulence
    warp,         // Swelling jump vibration during Vimana drive
    click         // Subtle tactile button/switch click
};

// Polling and state query
GamepadState poll_gamepad_state(int gamepad_index = 0);
bool is_gamepad_connected(int gamepad_index = 0);
std::string get_gamepad_name(int gamepad_index = 0);

// Deadzone helper: applies linear scaling past deadzone threshold [0.0..1.0]
float apply_axis_deadzone(float raw_value, float deadzone);

// Rumble vibration triggers and updates
void trigger_gamepad_rumble(GamepadRumbleType type, float strength = 1.0F, int gamepad_index = 0);
void update_gamepad_rumble(const AudioTelemetry &telemetry, int gamepad_index = 0);
void stop_gamepad_rumble(int gamepad_index = 0);

// Rumble state inspection (useful for verification and tests)
void get_gamepad_rumble_state(float &left_motor, float &right_motor, float &duration, int gamepad_index = 0);
void reset_gamepad_rumble();

// Mock gamepad injection for automated testing
void set_mock_gamepad_state(const GamepadState *mock_state);
const GamepadState *get_mock_gamepad_state();

} // namespace noctis
