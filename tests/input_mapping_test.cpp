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
    noctis::reset_input_provider();
    return ok ? 0 : 1;
}
