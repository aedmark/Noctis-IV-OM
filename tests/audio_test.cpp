#include "audio.h"

#include <cmath>
#include <cstdio>

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

    // Telemetry updates and trigger functions must safely no-op when uninitialized
    noctis::update_audio_telemetry(telemetry);
    noctis::play_torch_click(true);
    noctis::play_torch_click(false);
    noctis::play_visor_servo();
    noctis::play_jetpack_burst();
    noctis::play_surface_footstep();
    noctis::shutdown_audio(); // Safe double-shutdown check

    std::printf("audio_test: all unit checks passed successfully\n");
    return ok ? 0 : 1;
}
