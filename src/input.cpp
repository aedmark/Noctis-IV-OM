#include "input.h"
#include "plus_controls.h"

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <algorithm>
#include <stack>

namespace noctis {
static bool g_cursor_lock_wanted = true;

bool is_cursor_lock_wanted() {
    return g_cursor_lock_wanted;
}

void set_cursor_lock_wanted(bool wanted) {
    g_cursor_lock_wanted = wanted;
}

bool is_mouse_locked_and_focused() {
#ifdef __EMSCRIPTEN__
    int locked = EM_ASM_INT({
        if (typeof document === 'undefined') return 1;
        var wantLock = (typeof window !== 'undefined' && typeof window.wantPointerLock !== 'undefined') ? window.wantPointerLock : true;
        var canvas = (typeof Module !== 'undefined' && Module.canvas) ? Module.canvas : document.querySelector('canvas');
        var isLocked = (document.pointerLockElement === canvas || (document.pointerLockElement !== null && document.pointerLockElement !== undefined));
        return (wantLock && isLocked && document.hasFocus()) ? 1 : 0;
    });
    return locked != 0;
#else
    if (!IsWindowReady()) {
        return g_cursor_lock_wanted;
    }
    return g_cursor_lock_wanted && IsCursorHidden() && IsWindowFocused();
#endif
}
} // namespace noctis

namespace {
std::stack<std::int16_t> keys;

noctis::InputFrame poll_raylib_input() {
    noctis::InputFrame frame;
    frame.mouse_locked     = noctis::is_mouse_locked_and_focused();
    frame.move_forward     = IsKeyDown(KEY_W);
    frame.move_backward    = IsKeyDown(KEY_S);
    frame.move_left        = IsKeyDown(KEY_A);
    frame.move_right       = IsKeyDown(KEY_D);
    frame.escape_down      = IsKeyDown(KEY_ESCAPE);
    frame.mouse_left_down  = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    frame.mouse_right_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    const auto mouse_delta = GetMouseDelta();
    frame.mouse_delta_x    = frame.mouse_locked ? mouse_delta.x : 0.0f;
    frame.mouse_delta_y    = frame.mouse_locked ? mouse_delta.y : 0.0f;

    std::int32_t key;
    while ((key = GetCharPressed()) != 0) {
        frame.text.push_back(key);
    }
    // IsKeyPressed misses a tap that is pressed and released between two polls
    // (common in the browser, where the frame yields for ~55 ms). The press
    // queue keeps every press, so a key counts if either source saw it.
    std::vector<std::int32_t> pressed_queue;
    while ((key = GetKeyPressed()) != 0) {
        pressed_queue.push_back(key);
    }
    const auto pressed = [&pressed_queue](std::int32_t code) {
        return IsKeyPressed(code)
            || std::find(pressed_queue.begin(), pressed_queue.end(), code) != pressed_queue.end();
    };

    frame.arrow_up_pressed    = pressed(KEY_UP);
    frame.arrow_down_pressed  = pressed(KEY_DOWN);
    frame.arrow_left_pressed  = pressed(KEY_LEFT);
    frame.arrow_right_pressed = pressed(KEY_RIGHT);
    frame.backspace_pressed   = pressed(KEY_BACKSPACE);
    frame.enter_pressed       = pressed(KEY_ENTER);
    frame.tab_pressed         = pressed(KEY_TAB);
    frame.apostrophe_pressed  = pressed(KEY_APOSTROPHE);
    frame.space_pressed       = pressed(KEY_SPACE);
    frame.space_down          = IsKeyDown(KEY_SPACE);
    frame.delete_pressed      = pressed(KEY_DELETE);
    frame.minus_pressed       = pressed(KEY_MINUS) || pressed(KEY_KP_SUBTRACT);
    frame.comma_pressed       = pressed(KEY_COMMA);
    frame.slash_pressed       = pressed(KEY_SLASH);
    frame.semicolon_pressed   = pressed(KEY_SEMICOLON);
    frame.plus_pressed      = (pressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) ||
                              pressed(KEY_KP_ADD);
    frame.control_down      = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    frame.f1_pressed        = pressed(KEY_F1);
    frame.f2_pressed        = pressed(KEY_F2);
    frame.f3_pressed        = pressed(KEY_F3);
    frame.f4_pressed        = pressed(KEY_F4);
    frame.page_up_pressed   = pressed(KEY_PAGE_UP);
    frame.page_down_pressed = pressed(KEY_PAGE_DOWN);
    frame.home_pressed      = pressed(KEY_HOME);
    frame.end_pressed       = pressed(KEY_END);
    frame.toggle_cursor_pressed = pressed(KEY_F10);
    frame.toggle_audio_pressed  = pressed(KEY_F9) || (frame.control_down && pressed(KEY_M));
    frame.toggle_fullscreen_pressed =
        pressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && pressed(KEY_ENTER));
    frame.toggle_aspect_pressed  = pressed(KEY_F8);
    frame.toggle_upscale_pressed = pressed(KEY_F7);
    frame.toggle_crt_pressed     = pressed(KEY_F6);
    return frame;
}

noctis::InputProvider input_provider                   = poll_raylib_input;
noctis::AudioToggleHandler audio_toggle_handler        = nullptr;
noctis::DisplayToggleHandler fullscreen_toggle_handler = nullptr;
noctis::DisplayToggleHandler aspect_toggle_handler     = nullptr;
noctis::DisplayToggleHandler upscale_toggle_handler    = nullptr;
noctis::DisplayToggleHandler crt_toggle_handler        = nullptr;
noctis::OverlayInputHandler overlay_input_handler      = nullptr;

void push_extended_key(std::int16_t scan_code) {
    keys.push(scan_code);
    keys.push(0);
}
} // namespace

std::int16_t mdltx = 0, mdlty = 0, mouse_x = 0, mouse_y = 0;
std::uint16_t mpul = 0;
wasdmov key_move_dir{};
bool key_space_down = false;

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
    if (!frame.mouse_locked) {
        mdltx = 0;
        mdlty = 0;
        mpul  = 0;
        key_move_dir   = {};
        key_space_down = false;
        return;
    }

    mdltx = static_cast<std::int16_t>(frame.mouse_delta_x / 5.0F);
    mdlty = static_cast<std::int16_t>(frame.mouse_delta_y / 5.0F);
    mouse_x += mdltx;
    mouse_y += mdlty;
    mpul = static_cast<std::uint16_t>((frame.mouse_left_down ? 1u : 0u) | (frame.mouse_right_down ? 2u : 0u));

    key_move_dir.forward  = frame.move_forward;
    key_move_dir.backward = frame.move_backward;
    key_move_dir.left     = frame.move_left;
    key_move_dir.right    = frame.move_right;
    key_space_down        = frame.space_down;

    if (frame.escape_down) {
        keys.push(27);
    }
    for (const auto key : frame.text) {
        if (frame.control_down && (key == '+' || key == '-'))
            continue;
        if (key >= 32 && key <= 126) {
            keys.push(static_cast<std::int16_t>(key));
        }
    }
    const auto has_text = [&frame](std::int32_t value) {
        return std::find(frame.text.begin(), frame.text.end(), value) != frame.text.end();
    };
    if (frame.arrow_up_pressed)
        push_extended_key(72);
    if (frame.arrow_down_pressed)
        push_extended_key(80);
    if (frame.arrow_left_pressed)
        push_extended_key(75);
    if (frame.arrow_right_pressed)
        push_extended_key(77);
    if (frame.page_up_pressed)
        push_extended_key(0x49);
    if (frame.page_down_pressed)
        push_extended_key(0x51);
    if (frame.home_pressed)
        push_extended_key(0x47);
    if (frame.end_pressed)
        push_extended_key(0x4F);
    if (frame.backspace_pressed)
        keys.push(8);
    if (frame.enter_pressed)
        keys.push(13);
    if (frame.tab_pressed)
        keys.push(9);
    if (frame.apostrophe_pressed && !has_text('\''))
        keys.push(39);
    if (frame.space_pressed && !has_text(' '))
        keys.push(' ');
    if (frame.delete_pressed)
        keys.push(noctis::delete_snapshot_key);
    if (frame.minus_pressed && !frame.control_down && !has_text('-'))
        keys.push('-');
    if (frame.comma_pressed && !has_text(','))
        keys.push(',');
    if (frame.slash_pressed && !has_text('/'))
        keys.push('/');
    if (frame.semicolon_pressed && !has_text(';') && !has_text(':'))
        keys.push(':');
    if (frame.plus_pressed && !frame.control_down && !has_text('+'))
        keys.push('+');
    if (frame.control_down && frame.minus_pressed)
        push_extended_key(142);
    if (frame.control_down && frame.plus_pressed)
        push_extended_key(144);
    if (frame.f1_pressed)
        push_extended_key(0x3B);
    if (frame.f2_pressed)
        push_extended_key(0x3C);
    if (frame.f3_pressed)
        push_extended_key(0x3D);
    if (frame.f4_pressed)
        push_extended_key(0x3E);
}

void set_input_provider(InputProvider provider) { input_provider = provider ? provider : poll_raylib_input; }

void reset_input_provider() { input_provider = poll_raylib_input; }

void reset_input_state() {
    keys  = {};
    mdltx = mdlty = mouse_x = mouse_y = 0;
    mpul                              = 0;
    key_move_dir                      = {};
    key_space_down                    = false;
    g_cursor_lock_wanted              = true;
}

void set_audio_toggle_handler(AudioToggleHandler handler) { audio_toggle_handler = handler; }
void set_fullscreen_toggle_handler(DisplayToggleHandler handler) { fullscreen_toggle_handler = handler; }
void set_aspect_toggle_handler(DisplayToggleHandler handler) { aspect_toggle_handler = handler; }
void set_upscale_toggle_handler(DisplayToggleHandler handler) { upscale_toggle_handler = handler; }
void set_crt_toggle_handler(DisplayToggleHandler handler) { crt_toggle_handler = handler; }
void set_overlay_input_handler(OverlayInputHandler handler) { overlay_input_handler = handler; }

} // namespace noctis

void handle_input() {
    auto frame = input_provider();

#ifndef __EMSCRIPTEN__
    if (IsWindowReady()) {
        const bool is_focused = IsWindowFocused();
        static bool s_was_window_focused = true;
        if (!s_was_window_focused && is_focused) {
            if (noctis::is_cursor_lock_wanted() && !IsCursorHidden()) {
                DisableCursor();
            }
        }
        s_was_window_focused = is_focused;
    }
#endif

    if (frame.toggle_cursor_pressed) {
#ifndef __EMSCRIPTEN__
        noctis::set_cursor_lock_wanted(!noctis::is_cursor_lock_wanted());
        if (noctis::is_cursor_lock_wanted()) {
            if (IsWindowReady()) {
                DisableCursor();
            }
        } else {
            if (IsWindowReady()) {
                EnableCursor();
            }
        }
#else
        noctis::set_cursor_lock_wanted(!noctis::is_cursor_lock_wanted());
#endif
    }

    if (IsWindowReady()) {
        frame.mouse_locked = noctis::is_mouse_locked_and_focused();
    }

    static bool s_was_locked = false;
    if (!s_was_locked && frame.mouse_locked) {
        // Just transitioned into locked state; suppress any warp/re-center delta
        frame.mouse_delta_x = 0.0f;
        frame.mouse_delta_y = 0.0f;
    }
    s_was_locked = frame.mouse_locked;

    if (!frame.mouse_locked) {
        frame.mouse_delta_x       = 0.0f;
        frame.mouse_delta_y       = 0.0f;
        frame.move_forward        = false;
        frame.move_backward       = false;
        frame.move_left           = false;
        frame.move_right          = false;
        frame.space_down          = false;
        frame.space_pressed       = false;
        frame.mouse_left_down     = false;
        frame.mouse_right_down    = false;
        frame.arrow_up_pressed    = false;
        frame.arrow_down_pressed  = false;
        frame.arrow_left_pressed  = false;
        frame.arrow_right_pressed = false;
        frame.text.clear();
    }

    if (overlay_input_handler && overlay_input_handler(frame)) {
        noctis::apply_input_frame({});
    } else {
        noctis::apply_input_frame(frame);
    }

    if (frame.toggle_audio_pressed && audio_toggle_handler) {
        audio_toggle_handler();
    }
    if (frame.toggle_fullscreen_pressed && fullscreen_toggle_handler) {
        fullscreen_toggle_handler();
    }
    if (frame.toggle_aspect_pressed && aspect_toggle_handler) {
        aspect_toggle_handler();
    }
    if (frame.toggle_upscale_pressed && upscale_toggle_handler) {
        upscale_toggle_handler();
    }
    if (frame.toggle_crt_pressed && crt_toggle_handler) {
        crt_toggle_handler();
    }
}
