#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

enum class MusicPlaybackMode {
    generative, // Procedural multi-oscillator ambient soundscapes responding to stars and planets
    recorded,   // Stream music tracks from the music/ playlist
    hybrid,     // Recorded tracks with subtle procedural ambient under-layer
    off,        // Music channels disabled
};

struct MusicTrackInfo {
    std::filesystem::path path;
    std::string title;
    float duration_seconds = 0.0f;
};

// Lifecycle
void initialize_music(const std::filesystem::path &user_music_dir,
                      const std::filesystem::path &res_music_dir = {});
void shutdown_music();
bool is_music_ready();

// Per-frame update (called in simulation/render loop to stream audio chunks & manage playlist progression)
void update_music();

// Playlist scanning & inspection
void scan_music_playlist();
std::size_t get_music_track_count();
std::size_t get_current_music_track_index();
const MusicTrackInfo *get_current_music_track();
std::string get_current_music_track_title();

// Playback transport controls
bool play_music_track(std::size_t index);
bool next_music_track();
bool previous_music_track();
void stop_music_playback();
void pause_music_playback();
void resume_music_playback();
bool is_music_stream_active();
inline bool is_music_playing() { return is_music_stream_active(); }
inline std::string get_current_track_title() { return get_current_music_track_title(); }

// Mode controls
void set_music_mode(MusicPlaybackMode mode);
MusicPlaybackMode get_music_mode();
MusicPlaybackMode cycle_music_mode();
const char *music_mode_name(MusicPlaybackMode mode);

// Stream volume scaling
void set_music_stream_volume(float volume);
float get_music_stream_volume();

} // namespace noctis
