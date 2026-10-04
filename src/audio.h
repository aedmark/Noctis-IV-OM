#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace noctis {

enum class AudioScene {
    cabin,   // Inside Stardrifter cabin in space
    roof,    // Outside Stardrifter on roof in space
    surface, // On planetary surface
};

struct AudioTelemetry {
    AudioScene scene         = AudioScene::cabin;
    float atmosphere_density = 0.0f;  // 0.0 = vacuum (airless), >0 = atmosphere
    float weather_rain       = 0.0f;  // 0.0 to 5.0
    bool travel_active       = false; // Vimana propulsion active
    int travel_phase         = 0;     // TravelPhase enum value
    float travel_speed       = 0.0f;  // Propulsion speed / progress
    bool player_walking      = false; // Walking on planetary surface
    bool jetpack_active      = false; // Thruster firing
    bool rcs_active          = false; // Sublight RCS attitude thruster firing
    float entry_buffeting    = 0.0f;  // Atmospheric descent buffeting turbulence (0.0 to 1.0)
};

// Subsystem lifecycle
void initialize_audio();
void shutdown_audio();
bool is_audio_ready();

// Telemetry update (called once per simulation frame)
void update_audio_telemetry(const AudioTelemetry &telemetry);

// Exploration & Flight Foley triggers
void play_torch_click(bool turning_on);
void play_visor_servo();
void play_jetpack_burst();
void play_surface_footstep();
void play_rcs_burst();
void play_touchdown_clunk();

// Cockpit & GOESnet Foley triggers
void play_cockpit_button();
void play_terminal_keystroke();
void play_goesnet_transmit();
void play_goesnet_chime(bool positive = true);
void play_terminal_scroll();
void play_deck_lift();

enum class AudioCategory {
    master,
    cabin,
    propulsion,
    weather,
    foley,
};

struct AudioSettings {
    bool muted              = false;
    float master_volume     = 0.80f;
    float cabin_volume      = 1.00f;
    float propulsion_volume = 1.00f;
    float weather_volume    = 1.00f;
    float foley_volume      = 1.00f;
};

// Audio settings & controls
void set_audio_muted(bool muted);
bool is_audio_muted();
void toggle_audio_mute();

void set_master_volume_level(float volume);
float get_master_volume_level();

void set_audio_category_volume(AudioCategory category, float volume);
float get_audio_category_volume(AudioCategory category);
float step_audio_category_volume(AudioCategory category, float delta);
const char *audio_category_name(AudioCategory category);

void set_selected_audio_category(AudioCategory category);
AudioCategory get_selected_audio_category();
int get_selected_audio_category_index();
void select_next_audio_category();
void select_previous_audio_category();
float step_selected_audio_category_volume(float delta);

AudioSettings capture_audio_settings();
void apply_audio_settings(const AudioSettings &settings);

bool save_audio_settings(const std::filesystem::path &config_dir);
bool load_audio_settings(const std::filesystem::path &config_dir);

} // namespace noctis
