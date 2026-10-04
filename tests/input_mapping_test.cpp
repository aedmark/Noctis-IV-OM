#include "input.h"
#include "plus_controls.h"

#include <array>
#include <cstdio>

namespace {
noctis::InputFrame scripted_frame() {
    noctis::InputFrame frame;
    frame.move_right = true;
    frame.text = {'b'};
    return frame;
}
}

int main() {
    noctis::reset_input_state();

    noctis::InputFrame frame;
    frame.move_forward = true;
    frame.move_left = true;
    frame.escape_down = true;
    frame.mouse_left_down = true;
    frame.mouse_right_down = true;
    frame.mouse_delta_x = 12.9F;
    frame.mouse_delta_y = -6.0F;
    frame.text = {'a', '7', 'A', '!'};
    frame.arrow_up_pressed = true;
    frame.enter_pressed = true;
    frame.delete_pressed = true;
    noctis::apply_input_frame(frame);

    bool ok = true;
    ok &= key_move_dir.forward && key_move_dir.left;
    ok &= !key_move_dir.backward && !key_move_dir.right;
    ok &= mpul == 3;
    ok &= mdltx == 2 && mdlty == -1;
    ok &= mouse_x == 2 && mouse_y == -1;

    // The inherited consumers expect a LIFO queue and DOS extended keys as
    // zero followed by the scan code when popped.
    constexpr std::array<std::int16_t, 9> expected{noctis::delete_snapshot_key, 13, 0, 72, '!', 'A', '7', 'a', 27};
    for (const auto value : expected) {
        const auto actual = get_key();
        if (actual != value) {
            std::fprintf(stderr, "key queue: expected %d, got %d\n", value, actual);
            ok = false;
        }
    }
    ok &= !is_key();

    noctis::InputFrame function_keys;
    function_keys.f1_pressed = true;
    function_keys.f2_pressed = true;
    function_keys.f3_pressed = true;
    noctis::apply_input_frame(function_keys);
    constexpr std::array<std::int16_t, 6> expected_function_keys{0, 0x3D, 0, 0x3C, 0, 0x3B};
    for (const auto value : expected_function_keys) ok &= get_key() == value;
    ok &= !is_key();

    noctis::InputFrame nav_keys;
    nav_keys.page_up_pressed = true;
    nav_keys.page_down_pressed = true;
    nav_keys.home_pressed = true;
    nav_keys.end_pressed = true;
    noctis::apply_input_frame(nav_keys);
    constexpr std::array<std::int16_t, 8> expected_nav_keys{0, 0x4F, 0, 0x47, 0, 0x51, 0, 0x49};
    for (const auto value : expected_nav_keys) ok &= get_key() == value;
    ok &= !is_key();

    noctis::InputFrame movie_decks;
    movie_decks.control_down = true;
    movie_decks.minus_pressed = true;
    movie_decks.plus_pressed = true;
    movie_decks.text = {'-', '+'};
    noctis::apply_input_frame(movie_decks);
    constexpr std::array<std::int16_t, 4> expected_movie_decks{0, 144, 0, 142};
    for (const auto value : expected_movie_decks) ok &= get_key() == value;
    ok &= !is_key();

    noctis::InputFrame symbols;
    symbols.text = {';', '=', '.'};
    symbols.plus_pressed = true;
    noctis::apply_input_frame(symbols);
    constexpr std::array<std::int16_t, 4> expected_symbols{'+', '.', '=', ';'};
    for (const auto value : expected_symbols) {
        ok &= get_key() == value;
    }
    ok &= !is_key();

    noctis::InputFrame released;
    released.mouse_delta_x = -5.0F;
    released.mouse_delta_y = 10.0F;
    noctis::apply_input_frame(released);
    ok &= !key_move_dir.forward && !key_move_dir.left;
    ok &= mpul == 0;
    ok &= mouse_x == 1 && mouse_y == 1;

    noctis::reset_input_state();
    noctis::set_input_provider(scripted_frame);
    handle_input();
    ok &= key_move_dir.right;
    ok &= get_key() == 'b' && !is_key();

    bool fullscreen_toggled = false;
    bool aspect_toggled     = false;
    noctis::set_fullscreen_toggle_handler([]() {
        static bool *flag = nullptr;
        if (flag) *flag = true;
    });
    // Use static flag pointers to test C-style function pointer handlers safely
    static bool s_fullscreen_invoked = false;
    static bool s_aspect_invoked     = false;
    static bool s_upscale_invoked    = false;
    static bool s_crt_invoked        = false;
    noctis::set_fullscreen_toggle_handler([]() { s_fullscreen_invoked = true; });
    noctis::set_aspect_toggle_handler([]() { s_aspect_invoked = true; });
    noctis::set_upscale_toggle_handler([]() { s_upscale_invoked = true; });
    noctis::set_crt_toggle_handler([]() { s_crt_invoked = true; });

    noctis::set_input_provider([]() {
        noctis::InputFrame f;
        f.toggle_fullscreen_pressed = true;
        f.toggle_aspect_pressed     = true;
        f.toggle_upscale_pressed    = true;
        f.toggle_crt_pressed        = true;
        return f;
    });
    handle_input();
    ok &= s_fullscreen_invoked;
    ok &= s_aspect_invoked;
    ok &= s_upscale_invoked;
    ok &= s_crt_invoked;
    noctis::set_fullscreen_toggle_handler(nullptr);
    noctis::set_aspect_toggle_handler(nullptr);
    noctis::set_upscale_toggle_handler(nullptr);
    noctis::set_crt_toggle_handler(nullptr);

    // Test that when mouse is unlocked, apply_input_frame rejects deltas, movement, and keys
    noctis::InputFrame unlocked_frame;
    unlocked_frame.mouse_locked = false;
    unlocked_frame.mouse_delta_x = 20.0f;
    unlocked_frame.mouse_delta_y = 20.0f;
    unlocked_frame.move_forward = true;
    unlocked_frame.move_left = true;
    unlocked_frame.space_down = true;
    unlocked_frame.mouse_left_down = true;
    unlocked_frame.text = {'x'};
    unlocked_frame.arrow_up_pressed = true;
    noctis::apply_input_frame(unlocked_frame);
    ok &= (mdltx == 0 && mdlty == 0);
    ok &= (mpul == 0);
    ok &= (!key_move_dir.forward && !key_move_dir.left);
    ok &= (!key_space_down);
    ok &= !is_key();

    // Test F10 cursor lock toggle
    noctis::reset_input_state();
    ok &= noctis::is_cursor_lock_wanted();
    noctis::set_input_provider([]() {
        noctis::InputFrame f;
        f.toggle_cursor_pressed = true;
        return f;
    });
    handle_input();
    ok &= !noctis::is_cursor_lock_wanted();
    handle_input();
    ok &= noctis::is_cursor_lock_wanted();

    // Test that handle_input with mouse_locked == false scrubs movement, deltas, and keys
    noctis::set_input_provider([]() {
        noctis::InputFrame f;
        f.mouse_locked = false;
        f.mouse_delta_x = 15.0f;
        f.move_forward = true;
        f.text = {'z'};
        return f;
    });
    handle_input();
    ok &= (mdltx == 0 && mdlty == 0);
    ok &= !key_move_dir.forward;
    ok &= !is_key();

    noctis::reset_input_provider();
    return ok ? 0 : 1;
}
