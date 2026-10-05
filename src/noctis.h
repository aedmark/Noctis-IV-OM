#pragma once

#include "noctis-d.h"
#include "movie_capture.h"
#include <raylib.h>
#include <cstdint>

// Definitions for noctis.cpp
extern RenderTexture2D temp_texture;
extern void swapBuffers();
extern bool surface_fixture_mode;
extern bool landing_fixture_mode;
extern bool environment_fixture_mode;
extern bool content_fixture_mode;
extern bool orbit_surface_fixture_mode;
extern bool oakenshield_fixture_mode;
extern const char *surface_fixture_name;
extern std::uint32_t last_snapshot;
extern std::int8_t option_mouse_look;
extern std::int16_t roof_speed;
extern std::int8_t draw_hud;
extern std::int8_t suit_torch;
extern std::int8_t lens_flare_mode;
extern std::int8_t seamless_border;
extern std::int8_t graphics_menu_status;
extern std::int8_t about;
extern noctis::MovieRecorder movie_recorder;
void handle_movie_extended_key(std::int16_t key);
bool handle_movie_key(std::int16_t key, bool label_entry);
void advance_movie_capture(bool ascending_from_surface);
