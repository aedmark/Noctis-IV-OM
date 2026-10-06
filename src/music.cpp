#include "music.h"
#include "audio.h"

#include <raylib.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace noctis {
namespace {

std::filesystem::path g_user_music_dir{};
std::filesystem::path g_res_music_dir{};

std::mutex g_music_mutex{};
std::vector<MusicTrackInfo> g_playlist{};
std::size_t g_current_track_index = 0;
bool g_is_playing                 = false;
bool g_is_paused                  = false;
bool g_music_initialized         = false;

Music g_active_music{};
bool g_music_stream_loaded = false;

std::atomic<float> g_music_stream_vol{0.75f};
std::atomic<int> g_active_playback_mode{static_cast<int>(MusicPlaybackMode::generative)};

bool is_supported_audio_extension(const std::filesystem::path &p) {
    auto ext = p.extension().string();
    for (char &c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return (ext == ".ogg" || ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".xm" || ext == ".mod");
}

std::string clean_track_title(const std::filesystem::path &p) {
    std::string stem = p.stem().string();
    // Replace underscores with spaces
    for (char &c : stem) {
        if (c == '_') c = ' ';
    }
    return stem;
}

void unload_current_stream_locked() {
    if (g_music_stream_loaded) {
        if (IsAudioDeviceReady() && IsMusicValid(g_active_music)) {
            StopMusicStream(g_active_music);
            UnloadMusicStream(g_active_music);
        }
        g_active_music        = {};
        g_music_stream_loaded = false;
        g_is_playing          = false;
    }
}

bool load_and_play_track_locked(std::size_t index) {
    if (index >= g_playlist.size()) {
        unload_current_stream_locked();
        return false;
    }

    unload_current_stream_locked();

    if (!IsAudioDeviceReady()) {
        // Headless fallback
        g_current_track_index = index;
        g_is_playing          = true;
        g_is_paused           = false;
        return true;
    }

    const auto &track = g_playlist[index];
    g_active_music    = LoadMusicStream(track.path.string().c_str());

    if (IsMusicValid(g_active_music)) {
        g_active_music.looping = false; // We manage looping / advancing via playlist
        PlayMusicStream(g_active_music);
        const float eff_vol = g_music_stream_vol.load() * get_master_volume_level();
        SetMusicVolume(g_active_music, is_audio_muted() ? 0.0f : eff_vol);
        g_music_stream_loaded = true;
        g_current_track_index = index;
        g_is_playing          = true;
        g_is_paused           = false;
        return true;
    }

    return false;
}

} // namespace

void initialize_music(const std::filesystem::path &user_music_dir,
                      const std::filesystem::path &res_music_dir) {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    g_user_music_dir     = user_music_dir;
    g_res_music_dir      = res_music_dir;
    g_music_initialized = true;

    // Scan initial playlist
    g_playlist.clear();
    g_current_track_index = 0;

    std::vector<std::filesystem::path> search_dirs;
    if (!g_user_music_dir.empty()) search_dirs.push_back(g_user_music_dir);
    if (!g_res_music_dir.empty() && g_res_music_dir != g_user_music_dir) search_dirs.push_back(g_res_music_dir);

    for (const auto &dir : search_dirs) {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec) || ec) continue;
        for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec) && is_supported_audio_extension(entry.path())) {
                MusicTrackInfo info{};
                info.path  = entry.path();
                info.title = clean_track_title(entry.path());
                g_playlist.push_back(info);
            }
        }
    }

    std::sort(g_playlist.begin(), g_playlist.end(), [](const MusicTrackInfo &a, const MusicTrackInfo &b) {
        return a.title < b.title;
    });

    const auto mode = static_cast<MusicPlaybackMode>(g_active_playback_mode.load());
    if ((mode == MusicPlaybackMode::recorded || mode == MusicPlaybackMode::hybrid) && !g_playlist.empty()) {
        load_and_play_track_locked(0);
    }
}

void shutdown_music() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    unload_current_stream_locked();
    g_playlist.clear();
    g_music_initialized = false;
}

bool is_music_ready() {
    return g_music_initialized;
}

void update_music() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (!g_music_initialized) return;

    const auto mode = static_cast<MusicPlaybackMode>(g_active_playback_mode.load());
    if (mode == MusicPlaybackMode::off || mode == MusicPlaybackMode::generative) {
        if (g_music_stream_loaded) {
            unload_current_stream_locked();
        }
        return;
    }

    // Recorded or hybrid mode
    if (!g_music_stream_loaded) {
        if (!g_playlist.empty() && g_is_playing) {
            load_and_play_track_locked(g_current_track_index);
        }
        return;
    }

    if (IsAudioDeviceReady() && IsMusicValid(g_active_music)) {
        UpdateMusicStream(g_active_music);

        // Update effective volume (accounting for mute, master volume, and music category volume)
        const float eff_vol = is_audio_muted() ? 0.0f : (g_music_stream_vol.load() * get_master_volume_level());
        SetMusicVolume(g_active_music, eff_vol);

        const float length = GetMusicTimeLength(g_active_music);
        const float played = GetMusicTimePlayed(g_active_music);

        // Check if track ended
        if (length > 0.0f && played >= (length - 0.05f)) {
            if (!g_playlist.empty()) {
                std::size_t next_idx = (g_current_track_index + 1) % g_playlist.size();
                load_and_play_track_locked(next_idx);
            } else {
                unload_current_stream_locked();
            }
        }
    }
}

void scan_music_playlist() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    g_playlist.clear();

    std::vector<std::filesystem::path> search_dirs;
    if (!g_user_music_dir.empty()) search_dirs.push_back(g_user_music_dir);
    if (!g_res_music_dir.empty() && g_res_music_dir != g_user_music_dir) search_dirs.push_back(g_res_music_dir);

    for (const auto &dir : search_dirs) {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec) || ec) continue;
        for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec) && is_supported_audio_extension(entry.path())) {
                MusicTrackInfo info{};
                info.path  = entry.path();
                info.title = clean_track_title(entry.path());
                g_playlist.push_back(info);
            }
        }
    }

    std::sort(g_playlist.begin(), g_playlist.end(), [](const MusicTrackInfo &a, const MusicTrackInfo &b) {
        return a.title < b.title;
    });

    if (g_current_track_index >= g_playlist.size()) {
        g_current_track_index = 0;
    }
}

std::size_t get_music_track_count() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    return g_playlist.size();
}

std::size_t get_current_music_track_index() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    return g_current_track_index;
}

const MusicTrackInfo *get_current_music_track() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (g_current_track_index < g_playlist.size()) {
        return &g_playlist[g_current_track_index];
    }
    return nullptr;
}

std::string get_current_music_track_title() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    const auto mode = static_cast<MusicPlaybackMode>(g_active_playback_mode.load());
    if (mode == MusicPlaybackMode::off) {
        return "OFF (MUTED)";
    }
    if (mode == MusicPlaybackMode::generative) {
        return "GENERATIVE AMBIENT";
    }
    if (g_playlist.empty()) {
        return "EMPTY (NO TRACKS)";
    }
    if (g_current_track_index < g_playlist.size()) {
        return g_playlist[g_current_track_index].title;
    }
    return "UNKNOWN";
}

bool play_music_track(std::size_t index) {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    return load_and_play_track_locked(index);
}

bool next_music_track() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (g_playlist.empty()) return false;
    std::size_t next_idx = (g_current_track_index + 1) % g_playlist.size();
    return load_and_play_track_locked(next_idx);
}

bool previous_music_track() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (g_playlist.empty()) return false;
    std::size_t prev_idx = (g_current_track_index + g_playlist.size() - 1) % g_playlist.size();
    return load_and_play_track_locked(prev_idx);
}

void stop_music_playback() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    unload_current_stream_locked();
    g_is_playing = false;
    g_is_paused  = false;
}

void pause_music_playback() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (g_music_stream_loaded && IsAudioDeviceReady() && IsMusicValid(g_active_music)) {
        PauseMusicStream(g_active_music);
    }
    g_is_paused  = true;
    g_is_playing = false;
}

void resume_music_playback() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (g_music_stream_loaded && IsAudioDeviceReady() && IsMusicValid(g_active_music)) {
        ResumeMusicStream(g_active_music);
    }
    g_is_paused  = false;
    g_is_playing = true;
}

bool is_music_stream_active() {
    std::lock_guard<std::mutex> lock(g_music_mutex);
    return g_music_stream_loaded && g_is_playing && !g_is_paused;
}

void set_music_mode(MusicPlaybackMode mode) {
    g_active_playback_mode.store(static_cast<int>(mode));
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (mode == MusicPlaybackMode::off || mode == MusicPlaybackMode::generative) {
        unload_current_stream_locked();
    } else if ((mode == MusicPlaybackMode::recorded || mode == MusicPlaybackMode::hybrid) && !g_is_playing) {
        if (!g_playlist.empty()) {
            load_and_play_track_locked(g_current_track_index);
        }
    }
}

MusicPlaybackMode get_music_mode() {
    return static_cast<MusicPlaybackMode>(g_active_playback_mode.load());
}

MusicPlaybackMode cycle_music_mode() {
    int cur  = g_active_playback_mode.load();
    int next = (cur + 1) % 4;
    auto new_mode = static_cast<MusicPlaybackMode>(next);
    set_music_mode(new_mode);
    return new_mode;
}

const char *music_mode_name(MusicPlaybackMode mode) {
    switch (mode) {
    case MusicPlaybackMode::generative: return "GENERATIVE";
    case MusicPlaybackMode::recorded:   return "RECORDED";
    case MusicPlaybackMode::hybrid:     return "HYBRID";
    case MusicPlaybackMode::off:        return "OFF";
    }
    return "UNKNOWN";
}

void set_music_stream_volume(float volume) {
    g_music_stream_vol.store(std::clamp(volume, 0.0f, 1.0f));
    std::lock_guard<std::mutex> lock(g_music_mutex);
    if (g_music_stream_loaded && IsAudioDeviceReady() && IsMusicValid(g_active_music)) {
        const float eff_vol = is_audio_muted() ? 0.0f : (g_music_stream_vol.load() * get_master_volume_level());
        SetMusicVolume(g_active_music, eff_vol);
    }
}

float get_music_stream_volume() {
    return g_music_stream_vol.load();
}

} // namespace noctis
