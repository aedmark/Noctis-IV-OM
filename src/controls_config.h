#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace noctis {

struct ControlsSettings {
    float mouse_sensitivity = 1.0F; // 0.1F .. 5.0F
    bool invert_mouse_y = false;     // false = Normal (push forward -> look UP), true = Inverted (push forward -> look DOWN)
    int mouselook_mode = 1;          // 0 = off, 1 = on, 2 = inv. y axis
    int key_forward = 87;           // KEY_W
    int key_backward = 83;          // KEY_S
    int key_left = 65;              // KEY_A
    int key_right = 68;             // KEY_D
    int key_jump = 32;              // KEY_SPACE

    // Gamepad settings
    float gamepad_sensitivity = 1.0F; // 0.1F .. 5.0F
    float gamepad_deadzone = 0.15F;   // 0.05F .. 0.50F
    bool gamepad_invert_y = false;    // false = Normal, true = Inverted
    bool rumble_enabled = true;       // Haptic vibration feedback
};

const ControlsSettings &get_controls_settings();
void set_controls_settings(const ControlsSettings &settings);
void reset_controls_settings();

// Toggles invert_mouse_y and returns the new value
bool toggle_mouse_inversion();
void set_mouse_inversion(bool invert);
bool is_mouse_inversion_active();

// Sensitivity adjustments (clamped between 0.1F and 5.0F)
float set_mouse_sensitivity(float sensitivity);
float adjust_mouse_sensitivity(float delta);
float get_mouse_sensitivity();

// Gamepad controls & rumble helpers
bool is_rumble_enabled();
bool toggle_rumble();
void set_rumble_enabled(bool enabled);
float get_gamepad_sensitivity();
float set_gamepad_sensitivity(float sensitivity);
float adjust_gamepad_sensitivity(float delta);
float get_gamepad_deadzone();
float set_gamepad_deadzone(float deadzone);

// Persistence in config_dir / "settings.ini" (and "control_settings.ini")
bool save_controls_settings(const std::filesystem::path &config_dir);
bool load_controls_settings(const std::filesystem::path &config_dir);

// Key code <-> string translation
int key_code_from_name(std::string_view name);
std::string key_name_from_code(int key_code);

} // namespace noctis
