#pragma once

#include "noctis-d.h"

#include <cstdint>
#include <vector>

namespace noctis {

struct InputFrame {
    bool move_forward     = false;
    bool move_backward    = false;
    bool move_left        = false;
    bool move_right       = false;
    bool escape_down      = false;
    bool mouse_left_down  = false;
    bool mouse_right_down = false;
    float mouse_delta_x   = 0.0F;
    float mouse_delta_y   = 0.0F;
    std::vector<std::int32_t> text;
    bool arrow_up_pressed      = false;
    bool arrow_down_pressed    = false;
    bool arrow_left_pressed    = false;
    bool arrow_right_pressed   = false;
    bool backspace_pressed     = false;
    bool enter_pressed         = false;
    bool apostrophe_pressed    = false;
    bool space_pressed         = false;
    bool delete_pressed        = false;
    bool minus_pressed         = false;
    bool comma_pressed         = false;
    bool slash_pressed         = false;
    bool semicolon_pressed     = false;
    bool plus_pressed          = false;
    bool control_down          = false;
    bool f1_pressed            = false;
    bool f2_pressed            = false;
    bool f3_pressed            = false;
    bool page_up_pressed       = false;
    bool page_down_pressed     = false;
    bool home_pressed          = false;
    bool end_pressed           = false;
    bool toggle_cursor_pressed     = false;
    bool toggle_audio_pressed      = false;
    bool toggle_fullscreen_pressed = false;
    bool toggle_aspect_pressed     = false;
};

using InputProvider        = InputFrame (*)();
using AudioToggleHandler   = void (*)();
using DisplayToggleHandler = void (*)();

void apply_input_frame(const InputFrame &frame);
void set_input_provider(InputProvider provider);
void reset_input_provider();
void reset_input_state();
void set_audio_toggle_handler(AudioToggleHandler handler);
void set_fullscreen_toggle_handler(DisplayToggleHandler handler);
void set_aspect_toggle_handler(DisplayToggleHandler handler);

} // namespace noctis

extern std::int16_t mdltx, mdlty, mouse_x, mouse_y;
extern std::uint16_t mpul;
extern wasdmov key_move_dir;

std::int16_t get_key();
bool is_key();
void handle_input();
