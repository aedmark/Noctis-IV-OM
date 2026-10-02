#include "input.h"
#include "plus_controls.h"

#include <raylib.h>

#include <algorithm>
#include <stack>

namespace {
std::stack<std::int16_t> keys;

noctis::InputFrame poll_raylib_input() {
    noctis::InputFrame frame;
    frame.move_forward = IsKeyDown(KEY_W);
    frame.move_backward = IsKeyDown(KEY_S);
    frame.move_left = IsKeyDown(KEY_A);
    frame.move_right = IsKeyDown(KEY_D);
    frame.escape_down = IsKeyDown(KEY_ESCAPE);
    frame.mouse_left_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    frame.mouse_right_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    const auto mouse_delta = GetMouseDelta();
    frame.mouse_delta_x = mouse_delta.x;
    frame.mouse_delta_y = mouse_delta.y;

    std::int32_t key;
    while ((key = GetCharPressed()) != 0) {
        frame.text.push_back(key);
    }

    frame.arrow_up_pressed = IsKeyPressed(KEY_UP);
    frame.arrow_down_pressed = IsKeyPressed(KEY_DOWN);
    frame.arrow_left_pressed = IsKeyPressed(KEY_LEFT);
    frame.arrow_right_pressed = IsKeyPressed(KEY_RIGHT);
    frame.backspace_pressed = IsKeyPressed(KEY_BACKSPACE);
    frame.enter_pressed = IsKeyPressed(KEY_ENTER);
    frame.apostrophe_pressed = IsKeyPressed(KEY_APOSTROPHE);
    frame.space_pressed = IsKeyPressed(KEY_SPACE);
    frame.delete_pressed = IsKeyPressed(KEY_DELETE);
    frame.minus_pressed = IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT);
    frame.comma_pressed = IsKeyPressed(KEY_COMMA);
    frame.slash_pressed = IsKeyPressed(KEY_SLASH);
    frame.semicolon_pressed = IsKeyPressed(KEY_SEMICOLON);
    frame.plus_pressed = (IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
                         || IsKeyPressed(KEY_KP_ADD);
    frame.control_down = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    frame.f1_pressed = IsKeyPressed(KEY_F1);
    frame.f2_pressed = IsKeyPressed(KEY_F2);
    frame.f3_pressed = IsKeyPressed(KEY_F3);
    frame.page_up_pressed = IsKeyPressed(KEY_PAGE_UP);
    frame.page_down_pressed = IsKeyPressed(KEY_PAGE_DOWN);
    frame.home_pressed = IsKeyPressed(KEY_HOME);
    frame.end_pressed = IsKeyPressed(KEY_END);
    frame.toggle_cursor_pressed = IsKeyPressed(KEY_F10);
    return frame;
}

noctis::InputProvider input_provider = poll_raylib_input;
bool cursor_captured = false;

void push_extended_key(std::int16_t scan_code) {
    keys.push(scan_code);
    keys.push(0);
}
}

std::int16_t mdltx = 0, mdlty = 0, mouse_x = 0, mouse_y = 0;
std::uint16_t mpul = 0;
wasdmov key_move_dir{};

std::int16_t get_key() {
    if (keys.empty()) {
        return 0;
    }
    const auto top = keys.top();
    keys.pop();
    return top;
}

bool is_key() { return !keys.empty(); }

namespace noctis {

void apply_input_frame(const InputFrame &frame) {
    mdltx = static_cast<std::int16_t>(frame.mouse_delta_x / 5.0F);
    mdlty = static_cast<std::int16_t>(frame.mouse_delta_y / 5.0F);
    mouse_x += mdltx;
    mouse_y += mdlty;
    mpul = static_cast<std::uint16_t>((frame.mouse_left_down ? 1u : 0u)
                                     | (frame.mouse_right_down ? 2u : 0u));

    key_move_dir.forward = frame.move_forward;
    key_move_dir.backward = frame.move_backward;
    key_move_dir.left = frame.move_left;
    key_move_dir.right = frame.move_right;

    if (frame.escape_down) {
        keys.push(27);
    }
    for (const auto key : frame.text) {
        if (frame.control_down && (key == '+' || key == '-')) continue;
        if (key >= 32 && key <= 126) {
            keys.push(static_cast<std::int16_t>(key));
        }
    }
    const auto has_text = [&frame](std::int32_t value) {
        return std::find(frame.text.begin(), frame.text.end(), value) != frame.text.end();
    };
    if (frame.arrow_up_pressed) push_extended_key(72);
    if (frame.arrow_down_pressed) push_extended_key(80);
    if (frame.arrow_left_pressed) push_extended_key(75);
    if (frame.arrow_right_pressed) push_extended_key(77);
    if (frame.page_up_pressed) push_extended_key(0x49);
    if (frame.page_down_pressed) push_extended_key(0x51);
    if (frame.home_pressed) push_extended_key(0x47);
    if (frame.end_pressed) push_extended_key(0x4F);
    if (frame.backspace_pressed) keys.push(8);
    if (frame.enter_pressed) keys.push(13);
    if (frame.apostrophe_pressed && !has_text('\'')) keys.push(39);
    if (frame.space_pressed && !has_text(' ')) keys.push(' ');
    if (frame.delete_pressed) keys.push(noctis::delete_snapshot_key);
    if (frame.minus_pressed && !frame.control_down && !has_text('-')) keys.push('-');
    if (frame.comma_pressed && !has_text(',')) keys.push(',');
    if (frame.slash_pressed && !has_text('/')) keys.push('/');
    if (frame.semicolon_pressed && !has_text(';') && !has_text(':')) keys.push(':');
    if (frame.plus_pressed && !frame.control_down && !has_text('+')) keys.push('+');
    if (frame.control_down && frame.minus_pressed) push_extended_key(142);
    if (frame.control_down && frame.plus_pressed) push_extended_key(144);
    if (frame.f1_pressed) push_extended_key(0x3B);
    if (frame.f2_pressed) push_extended_key(0x3C);
    if (frame.f3_pressed) push_extended_key(0x3D);
}

void set_input_provider(InputProvider provider) { input_provider = provider ? provider : poll_raylib_input; }

void reset_input_provider() { input_provider = poll_raylib_input; }

void reset_input_state() {
    keys = {};
    mdltx = mdlty = mouse_x = mouse_y = 0;
    mpul = 0;
    key_move_dir = {};
    cursor_captured = false;
}

} // namespace noctis

void handle_input() {
    const auto frame = input_provider();
    noctis::apply_input_frame(frame);
    if (frame.toggle_cursor_pressed) {
        cursor_captured = !cursor_captured;
        if (cursor_captured) {
            DisableCursor();
        } else {
            EnableCursor();
        }
    }
}
