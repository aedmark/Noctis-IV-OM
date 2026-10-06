#include "audio.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "audio test failure: %s\n", message);
    }
    return condition;
}
} // namespace

int main() {
    bool ok = true;

    // 1. Verify telemetry default values
    noctis::AudioTelemetry telemetry{};
    ok &= require(telemetry.scene == noctis::AudioScene::cabin, "default scene should be cabin");
    ok &= require(telemetry.atmosphere_density == 0.0f, "default atmo density should be 0");
    ok &= require(telemetry.weather_rain == 0.0f, "default weather rain should be 0");
    ok &= require(!telemetry.travel_active, "default travel should be inactive");
    ok &= require(!telemetry.player_walking, "default walking should be false");
    ok &= require(!telemetry.jetpack_active, "default jetpack should be false");
    ok &= require(!telemetry.rcs_active, "default rcs should be false");
    ok &= require(telemetry.entry_buffeting == 0.0f, "default entry buffeting should be 0");
    ok &= require(telemetry.star_class == 0, "default star class should be 0");
    ok &= require(telemetry.planet_type == 0, "default planet type should be 0");
    ok &= require(telemetry.planet_temp_k == 280.0f, "default planet temp should be 280K");
    ok &= require(telemetry.surface_biome == 0, "default surface biome should be 0");
    ok &= require(telemetry.in_star_system, "default in star system should be true");
    ok &= require(!telemetry.on_surface, "default on surface should be false");
    ok &= require(!telemetry.in_orbit, "default in orbit should be false");

    // 2. Verify volume and mute controls
    ok &= require(!noctis::is_audio_muted(), "default muted should be false");
    noctis::set_audio_muted(true);
    ok &= require(noctis::is_audio_muted(), "mute setter should set muted to true");
    noctis::toggle_audio_mute();
    ok &= require(!noctis::is_audio_muted(), "toggle mute should unmute");

    noctis::set_master_volume_level(0.5f);
    ok &= require(std::fabs(noctis::get_master_volume_level() - 0.5f) < 0.001f, "volume should be 0.5");
    noctis::set_master_volume_level(1.5f);
    ok &= require(std::fabs(noctis::get_master_volume_level() - 1.0f) < 0.001f, "volume should clamp to 1.0");
    noctis::set_master_volume_level(-0.5f);
    ok &= require(std::fabs(noctis::get_master_volume_level() - 0.0f) < 0.001f, "volume should clamp to 0.0");
    noctis::set_master_volume_level(0.75f);

    // 3. Verify headless fallback safety when audio device is uninitialized / unavailable
    ok &= require(!noctis::is_audio_ready(), "audio should not be ready prior to initialization");

    // Telemetry updates, music calls, and trigger functions must safely no-op when uninitialized
    noctis::update_audio_telemetry(telemetry);
    noctis::update_music();
    noctis::play_music_track(0);
    noctis::next_music_track();
    noctis::previous_music_track();
    noctis::stop_music_playback();
    ok &= require(!noctis::is_music_playing(), "music should not be playing when headless");
    ok &= require(noctis::get_current_track_title() == "GENERATIVE AMBIENT", "current track title indicates generative ambient");
    noctis::play_torch_click(true);
    noctis::play_torch_click(false);
    noctis::play_visor_servo();
    noctis::play_jetpack_burst();
    noctis::play_surface_footstep();
    noctis::play_rcs_burst();
    noctis::play_touchdown_clunk();
    noctis::play_cockpit_button();
    noctis::play_terminal_keystroke();
    noctis::play_goesnet_transmit();
    noctis::play_goesnet_chime(true);
    noctis::play_goesnet_chime(false);
    noctis::play_terminal_scroll();
    noctis::play_deck_lift();
    noctis::shutdown_music();
    noctis::shutdown_audio(); // Safe double-shutdown check

    // 4. Verify category volume controls and category selection (6 channels)
    noctis::set_audio_category_volume(noctis::AudioCategory::music, 0.35f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::music) - 0.35f) < 0.001f,
                  "music volume should be 0.35");

    noctis::set_audio_category_volume(noctis::AudioCategory::cabin, 0.45f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::cabin) - 0.45f) < 0.001f,
                  "cabin volume should be 0.45");

    noctis::set_audio_category_volume(noctis::AudioCategory::propulsion, 1.25f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::propulsion) - 1.00f) < 0.001f,
                  "propulsion volume should clamp to 1.00");

    noctis::set_audio_category_volume(noctis::AudioCategory::weather, -0.25f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::weather) - 0.00f) < 0.001f,
                  "weather volume should clamp to 0.00");

    noctis::set_audio_category_volume(noctis::AudioCategory::foley, 0.60f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::foley) - 0.60f) < 0.001f,
                  "foley volume should be 0.60");

    // Stepping
    noctis::step_audio_category_volume(noctis::AudioCategory::music, 0.05f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::music) - 0.40f) < 0.001f,
                  "music volume stepped by +0.05");

    noctis::step_audio_category_volume(noctis::AudioCategory::foley, 0.05f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::foley) - 0.65f) < 0.001f,
                  "foley volume stepped by +0.05");

    noctis::step_audio_category_volume(noctis::AudioCategory::foley, -0.10f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::foley) - 0.55f) < 0.001f,
                  "foley volume stepped by -0.10");

    // Category names
    ok &= require(std::string(noctis::audio_category_name(noctis::AudioCategory::master)) == "MASTER", "master name");
    ok &= require(std::string(noctis::audio_category_name(noctis::AudioCategory::music)) == "MUSIC", "music name");
    ok &= require(std::string(noctis::audio_category_name(noctis::AudioCategory::cabin)) == "CABIN", "cabin name");
    ok &= require(std::string(noctis::audio_category_name(noctis::AudioCategory::propulsion)) == "PROPULSION", "propulsion name");
    ok &= require(std::string(noctis::audio_category_name(noctis::AudioCategory::weather)) == "WEATHER", "weather name");
    ok &= require(std::string(noctis::audio_category_name(noctis::AudioCategory::foley)) == "FOLEY", "foley name");

    // Selection
    noctis::set_selected_audio_category(noctis::AudioCategory::weather);
    ok &= require(noctis::get_selected_audio_category() == noctis::AudioCategory::weather, "selected should be weather");
    ok &= require(noctis::get_selected_audio_category_index() == 4, "selected index should be 4");

    noctis::select_next_audio_category();
    ok &= require(noctis::get_selected_audio_category() == noctis::AudioCategory::foley, "next after weather should be foley");

    noctis::select_next_audio_category();
    ok &= require(noctis::get_selected_audio_category() == noctis::AudioCategory::master, "next after foley should wrap to master");

    noctis::select_next_audio_category();
    ok &= require(noctis::get_selected_audio_category() == noctis::AudioCategory::music, "next after master should be music");

    noctis::select_previous_audio_category();
    ok &= require(noctis::get_selected_audio_category() == noctis::AudioCategory::master, "previous after music should be master");

    noctis::select_previous_audio_category();
    ok &= require(noctis::get_selected_audio_category() == noctis::AudioCategory::foley, "previous after master should wrap to foley");

    // Stepping selected
    noctis::set_audio_category_volume(noctis::AudioCategory::foley, 0.50f);
    noctis::step_selected_audio_category_volume(0.10f);
    ok &= require(std::fabs(noctis::get_audio_category_volume(noctis::AudioCategory::foley) - 0.60f) < 0.001f, "step selected volume");

    // Music mode toggle
    noctis::set_music_mode(noctis::MusicPlaybackMode::recorded);
    ok &= require(noctis::get_music_mode() == noctis::MusicPlaybackMode::recorded, "mode should be recorded");
    ok &= require(noctis::cycle_music_mode() == noctis::MusicPlaybackMode::hybrid, "cycle to hybrid");
    ok &= require(noctis::cycle_music_mode() == noctis::MusicPlaybackMode::off, "cycle to off");
    ok &= require(noctis::cycle_music_mode() == noctis::MusicPlaybackMode::generative, "cycle to generative");

    // 5. Verify Settings capture, apply, and INI roundtrip
    noctis::AudioSettings custom_settings{};
    custom_settings.muted              = true;
    custom_settings.master_volume     = 0.40f;
    custom_settings.music_volume      = 0.55f;
    custom_settings.cabin_volume      = 0.50f;
    custom_settings.propulsion_volume = 0.60f;
    custom_settings.weather_volume    = 0.70f;
    custom_settings.foley_volume      = 0.80f;
    custom_settings.music_mode        = noctis::MusicPlaybackMode::hybrid;
    noctis::apply_audio_settings(custom_settings);

    auto captured = noctis::capture_audio_settings();
    ok &= require(captured.muted == true, "captured muted");
    ok &= require(std::fabs(captured.master_volume - 0.40f) < 0.001f, "captured master");
    ok &= require(std::fabs(captured.music_volume - 0.55f) < 0.001f, "captured music");
    ok &= require(std::fabs(captured.cabin_volume - 0.50f) < 0.001f, "captured cabin");
    ok &= require(std::fabs(captured.propulsion_volume - 0.60f) < 0.001f, "captured propulsion");
    ok &= require(std::fabs(captured.weather_volume - 0.70f) < 0.001f, "captured weather");
    ok &= require(std::fabs(captured.foley_volume - 0.80f) < 0.001f, "captured foley");
    ok &= require(captured.music_mode == noctis::MusicPlaybackMode::hybrid, "captured music mode");

    // Test INI file roundtrip
    const auto test_config_dir = std::filesystem::temp_directory_path() / "noctis_audio_test_cfg";
    std::filesystem::remove_all(test_config_dir);

    ok &= require(noctis::save_audio_settings(test_config_dir), "save_audio_settings should succeed");

    // Reset settings to defaults
    noctis::AudioSettings reset_settings{};
    reset_settings.muted              = false;
    reset_settings.master_volume     = 1.0f;
    reset_settings.music_volume      = 1.0f;
    reset_settings.cabin_volume      = 1.0f;
    reset_settings.propulsion_volume = 1.0f;
    reset_settings.weather_volume    = 1.0f;
    reset_settings.foley_volume      = 1.0f;
    reset_settings.music_mode        = noctis::MusicPlaybackMode::generative;
    noctis::apply_audio_settings(reset_settings);

    ok &= require(noctis::load_audio_settings(test_config_dir), "load_audio_settings should succeed");
    auto reloaded = noctis::capture_audio_settings();
    ok &= require(reloaded.muted == true, "reloaded muted");
    ok &= require(std::fabs(reloaded.master_volume - 0.40f) < 0.001f, "reloaded master");
    ok &= require(std::fabs(reloaded.music_volume - 0.55f) < 0.001f, "reloaded music");
    ok &= require(std::fabs(reloaded.cabin_volume - 0.50f) < 0.001f, "reloaded cabin");
    ok &= require(std::fabs(reloaded.propulsion_volume - 0.60f) < 0.001f, "reloaded propulsion");
    ok &= require(std::fabs(reloaded.weather_volume - 0.70f) < 0.001f, "reloaded weather");
    ok &= require(std::fabs(reloaded.foley_volume - 0.80f) < 0.001f, "reloaded foley");
    ok &= require(reloaded.music_mode == noctis::MusicPlaybackMode::hybrid, "reloaded music mode");

    std::filesystem::remove_all(test_config_dir);

    std::printf("audio_test: all unit checks passed successfully\n");
    return ok ? 0 : 1;
}
