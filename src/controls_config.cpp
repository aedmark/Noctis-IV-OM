#include "controls_config.h"

#include <raylib.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <string>
#include <system_error>

namespace noctis {
namespace {

ControlsSettings s_active_settings;

std::string trim(std::string_view str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    auto end = str.find_last_not_of(" \t\r\n");
    return std::string(str.substr(start, end - start + 1));
}

std::string to_upper(std::string_view str) {
    std::string result(str);
    for (char &c : result) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return result;
}

std::string to_lower(std::string_view str) {
    std::string result(str);
    for (char &c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

struct KeyEntry {
    const char *name;
    int code;
};

const KeyEntry key_table[] = {
    {"W", KEY_W}, {"A", KEY_A}, {"S", KEY_S}, {"D", KEY_D},
    {"Q", KEY_Q}, {"E", KEY_E}, {"R", KEY_R}, {"F", KEY_F},
    {"C", KEY_C}, {"Z", KEY_Z}, {"X", KEY_X}, {"T", KEY_T},
    {"G", KEY_G}, {"V", KEY_V}, {"B", KEY_B}, {"N", KEY_N},
    {"M", KEY_M}, {"H", KEY_H}, {"J", KEY_J}, {"K", KEY_K},
    {"L", KEY_L}, {"I", KEY_I}, {"O", KEY_O}, {"P", KEY_P},
    {"U", KEY_U}, {"Y", KEY_Y},
    {"SPACE", KEY_SPACE}, {"TAB", KEY_TAB}, {"ENTER", KEY_ENTER},
    {"ESCAPE", KEY_ESCAPE}, {"ESC", KEY_ESCAPE}, {"BACKSPACE", KEY_BACKSPACE},
    {"DELETE", KEY_DELETE}, {"DEL", KEY_DELETE},
    {"UP", KEY_UP}, {"DOWN", KEY_DOWN}, {"LEFT", KEY_LEFT}, {"RIGHT", KEY_RIGHT},
    {"LSHIFT", KEY_LEFT_SHIFT}, {"RSHIFT", KEY_RIGHT_SHIFT}, {"SHIFT", KEY_LEFT_SHIFT},
    {"LCTRL", KEY_LEFT_CONTROL}, {"RCTRL", KEY_RIGHT_CONTROL}, {"CTRL", KEY_LEFT_CONTROL},
    {"LALT", KEY_LEFT_ALT}, {"RALT", KEY_RIGHT_ALT}, {"ALT", KEY_LEFT_ALT},
    {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3}, {"F4", KEY_F4},
    {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7}, {"F8", KEY_F8},
    {"F9", KEY_F9}, {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
    {"0", KEY_ZERO}, {"1", KEY_ONE}, {"2", KEY_TWO}, {"3", KEY_THREE}, {"4", KEY_FOUR},
    {"5", KEY_FIVE}, {"6", KEY_SIX}, {"7", KEY_SEVEN}, {"8", KEY_EIGHT}, {"9", KEY_NINE},
    {"KP_ADD", KEY_KP_ADD}, {"KP_SUBTRACT", KEY_KP_SUBTRACT},
    {"MINUS", KEY_MINUS}, {"EQUAL", KEY_EQUAL},
};

} // namespace

int key_code_from_name(std::string_view name) {
    const std::string upper = to_upper(trim(name));
    if (upper.empty()) return 0;
    for (const auto &entry : key_table) {
        if (upper == entry.name) return entry.code;
    }
    if (upper.size() == 1 && upper[0] >= 'A' && upper[0] <= 'Z') {
        return static_cast<int>(upper[0]);
    }
    if (upper.size() == 1 && upper[0] >= '0' && upper[0] <= '9') {
        return static_cast<int>(upper[0]);
    }
    return 0;
}

std::string key_name_from_code(int key_code) {
    if (key_code == 0) return "NONE";
    for (const auto &entry : key_table) {
        if (entry.code == key_code) return entry.name;
    }
    if (key_code >= 33 && key_code <= 126) {
        return std::string(1, static_cast<char>(key_code));
    }
    return "UNKNOWN";
}

const ControlsSettings &get_controls_settings() { return s_active_settings; }

void set_controls_settings(const ControlsSettings &settings) { s_active_settings = settings; }

void reset_controls_settings() { s_active_settings = ControlsSettings{}; }

bool toggle_mouse_inversion() {
    s_active_settings.invert_mouse_y = !s_active_settings.invert_mouse_y;
    return s_active_settings.invert_mouse_y;
}

void set_mouse_inversion(bool invert) { s_active_settings.invert_mouse_y = invert; }

bool is_mouse_inversion_active() { return s_active_settings.invert_mouse_y; }

float set_mouse_sensitivity(float sensitivity) {
    s_active_settings.mouse_sensitivity = std::clamp(sensitivity, 0.1F, 5.0F);
    return s_active_settings.mouse_sensitivity;
}

float adjust_mouse_sensitivity(float delta) {
    return set_mouse_sensitivity(s_active_settings.mouse_sensitivity + delta);
}

float get_mouse_sensitivity() { return s_active_settings.mouse_sensitivity; }

bool is_rumble_enabled() { return s_active_settings.rumble_enabled; }

bool toggle_rumble() {
    s_active_settings.rumble_enabled = !s_active_settings.rumble_enabled;
    return s_active_settings.rumble_enabled;
}

void set_rumble_enabled(bool enabled) { s_active_settings.rumble_enabled = enabled; }

float get_gamepad_sensitivity() { return s_active_settings.gamepad_sensitivity; }

float set_gamepad_sensitivity(float sensitivity) {
    s_active_settings.gamepad_sensitivity = std::clamp(sensitivity, 0.1F, 5.0F);
    return s_active_settings.gamepad_sensitivity;
}

float adjust_gamepad_sensitivity(float delta) {
    return set_gamepad_sensitivity(s_active_settings.gamepad_sensitivity + delta);
}

float get_gamepad_deadzone() { return s_active_settings.gamepad_deadzone; }

float set_gamepad_deadzone(float deadzone) {
    s_active_settings.gamepad_deadzone = std::clamp(deadzone, 0.05F, 0.50F);
    return s_active_settings.gamepad_deadzone;
}

bool save_controls_settings(const std::filesystem::path &config_dir) {
    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);
    if (ec) return false;

    auto write_file = [&](const std::filesystem::path &file_path) -> bool {
        const auto tmp_path = file_path.string() + ".tmp";
        std::ofstream out(tmp_path, std::ios::trunc);
        if (!out.is_open()) return false;

        out << "[Controls]\n";
        out << "mouse_sensitivity = " << s_active_settings.mouse_sensitivity << "\n";
        out << "invert_pitch = " << (s_active_settings.invert_mouse_y ? 1 : 0) << "\n";
        out << "mouselook_mode = " << s_active_settings.mouselook_mode << "\n";
        out << "key_forward = " << key_name_from_code(s_active_settings.key_forward) << "\n";
        out << "key_backward = " << key_name_from_code(s_active_settings.key_backward) << "\n";
        out << "key_left = " << key_name_from_code(s_active_settings.key_left) << "\n";
        out << "key_right = " << key_name_from_code(s_active_settings.key_right) << "\n";
        out << "key_jump = " << key_name_from_code(s_active_settings.key_jump) << "\n";
        out << "gamepad_sensitivity = " << s_active_settings.gamepad_sensitivity << "\n";
        out << "gamepad_deadzone = " << s_active_settings.gamepad_deadzone << "\n";
        out << "gamepad_invert_y = " << (s_active_settings.gamepad_invert_y ? 1 : 0) << "\n";
        out << "rumble_enabled = " << (s_active_settings.rumble_enabled ? 1 : 0) << "\n";

        out.close();
        if (!out) return false;

        std::error_code err;
        std::filesystem::rename(tmp_path, file_path, err);
        if (err) {
            std::filesystem::copy_file(tmp_path, file_path, std::filesystem::copy_options::overwrite_existing, err);
            std::filesystem::remove(tmp_path, err);
        }
        return !err;
    };

    bool ok = write_file(config_dir / "settings.ini");
    write_file(config_dir / "control_settings.ini");
    return ok;
}

bool load_controls_settings(const std::filesystem::path &config_dir) {
    auto file_path = config_dir / "settings.ini";
    std::error_code ec;
    if (!std::filesystem::exists(file_path, ec)) {
        file_path = config_dir / "control_settings.ini";
        if (!std::filesystem::exists(file_path, ec)) return false;
    }

    std::ifstream in(file_path);
    if (!in.is_open()) return false;

    std::string line;
    while (std::getline(in, line)) {
        const auto comment_pos = line.find_first_of(";#");
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '[') continue;

        const auto eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = to_lower(trim(trimmed.substr(0, eq_pos)));
        std::string val = trim(trimmed.substr(eq_pos + 1));
        std::string val_lower = to_lower(val);

        if (key == "mouse_sensitivity" || key == "sensitivity") {
            try {
                float v = std::stof(val);
                set_mouse_sensitivity(v);
            } catch (...) {}
        } else if (key == "invert_pitch" || key == "invert_mouse_y" || key == "mouse_invert_y" || key == "invert") {
            s_active_settings.invert_mouse_y = (val_lower == "1" || val_lower == "true" || val_lower == "yes");
        } else if (key == "mouselook_mode" || key == "mouselook") {
            try {
                s_active_settings.mouselook_mode = std::stoi(val);
            } catch (...) {}
        } else if (key == "key_forward" || key == "forward") {
            int code = key_code_from_name(val);
            if (code != 0) s_active_settings.key_forward = code;
        } else if (key == "key_backward" || key == "backward") {
            int code = key_code_from_name(val);
            if (code != 0) s_active_settings.key_backward = code;
        } else if (key == "key_left" || key == "left") {
            int code = key_code_from_name(val);
            if (code != 0) s_active_settings.key_left = code;
        } else if (key == "key_right" || key == "right") {
            int code = key_code_from_name(val);
            if (code != 0) s_active_settings.key_right = code;
        } else if (key == "key_jump" || key == "jump") {
            int code = key_code_from_name(val);
            if (code != 0) s_active_settings.key_jump = code;
        } else if (key == "gamepad_sensitivity") {
            try {
                float v = std::stof(val);
                set_gamepad_sensitivity(v);
            } catch (...) {}
        } else if (key == "gamepad_deadzone") {
            try {
                float v = std::stof(val);
                set_gamepad_deadzone(v);
            } catch (...) {}
        } else if (key == "gamepad_invert_y") {
            s_active_settings.gamepad_invert_y = (val_lower == "1" || val_lower == "true" || val_lower == "yes");
        } else if (key == "rumble_enabled" || key == "rumble" || key == "vibration") {
            s_active_settings.rumble_enabled = (val_lower == "1" || val_lower == "true" || val_lower == "yes");
        }
    }
    return true;
}

} // namespace noctis
