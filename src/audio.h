#pragma once

#include <cstdint>

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

// Audio settings & controls
void set_audio_muted(bool muted);
bool is_audio_muted();
void toggle_audio_mute();

void set_master_volume_level(float volume);
float get_master_volume_level();

} // namespace noctis
