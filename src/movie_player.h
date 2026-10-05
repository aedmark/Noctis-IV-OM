#pragma once

#include "display.h"
#include "input.h"
#include "video_export.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace noctis {

// Opens the in-cockpit Moviemaker projector overlay.
// If deck == 0, selects the latest recorded deck.
// Returns false if no movie decks or frames are found.
bool open_movie_player(const std::filesystem::path &movies_dir, std::uint16_t deck = 0);

bool movie_player_open();
void close_movie_player();
void shutdown_movie_player();

// Query current playback status
std::uint16_t movie_player_current_deck();
std::size_t movie_player_current_frame();
std::size_t movie_player_total_frames();
bool movie_player_is_playing();
bool movie_player_is_looping();
double movie_player_fps();

// Consumes input events while the player overlay is active.
bool movie_player_input(const InputFrame &frame);

// Renders the in-cockpit player overlay at native window resolution.
void render_movie_player(int render_width, int render_height, const DisplayViewport &viewport);

} // namespace noctis
