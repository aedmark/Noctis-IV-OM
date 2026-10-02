#include "audio.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <mutex>

namespace noctis {

namespace {

constexpr unsigned int AUDIO_SAMPLE_RATE = 44100;
constexpr float TWO_PI                   = 6.28318530717958647692f;

// Fast pseudo-random number generator for noise
struct NoiseGenerator {
    std::uint32_t state = 123456789;

    float next_white() {
        // Xorshift32
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        // Map to [-1.0f, 1.0f]
        return (static_cast<float>(state) / 2147483648.0f) - 1.0f;
    }

    // Kellet 3-pole pink noise filter
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float next_pink() {
        float white = next_white();
        b0          = 0.99765f * b0 + white * 0.0990460f;
        b1          = 0.96300f * b1 + white * 0.2965164f;
        b2          = 0.57000f * b2 + white * 1.0526913f;
        float pink  = b0 + b1 + b2 + white * 0.1848f;
        return pink * 0.18f; // Scale to approximate [-1.0, 1.0]
    }
};

// Chamberlin 2-pole State Variable Filter
struct StateVariableFilter {
    float low  = 0.0f;
    float band = 0.0f;

    void process(float input, float cutoff_hz, float q, float sample_rate, float &out_low, float &out_band,
                 float &out_high) {
        float f    = 2.0f * std::sin(static_cast<float>(M_PI) * (cutoff_hz / sample_rate));
        f          = std::clamp(f, 0.001f, 0.85f);
        float damp = 1.0f / std::max(0.1f, q);

        low += f * band;
        float high = input - low - damp * band;
        band += f * high;

        out_low  = low;
        out_band = band;
        out_high = high;
    }
};

// Generator helper for pre-baked procedural Foley sounds
Wave make_procedural_wave(int sample_count, const std::function<float(int, float)> &gen) {
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(sample_count);
    wave.sampleRate = AUDIO_SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels   = 1;
    auto *samples   = static_cast<std::int16_t *>(std::malloc(sample_count * sizeof(std::int16_t)));
    if (!samples)
        return wave;

    for (int i = 0; i < sample_count; ++i) {
        float t    = static_cast<float>(i) / static_cast<float>(AUDIO_SAMPLE_RATE);
        float val  = gen(i, t);
        val        = std::clamp(val, -1.0f, 1.0f);
        samples[i] = static_cast<std::int16_t>(val * 32767.0f);
    }
    wave.data = samples;
    return wave;
}

// Global audio state
bool g_audio_ready = false;
std::atomic<bool> g_muted{false};
std::atomic<float> g_master_volume{0.75f};

AudioStream g_ambient_stream{};
std::mutex g_telemetry_mutex{};
AudioTelemetry g_target_telemetry{};

// One-shot sounds
Sound g_sound_torch_on{};
Sound g_sound_torch_off{};
Sound g_sound_visor{};
Sound g_sound_jetpack{};
std::array<Sound, 3> g_sound_footsteps{};

// Footstep timing
float g_footstep_timer = 0.2f;
int g_footstep_index   = 0;
std::chrono::steady_clock::time_point g_last_telemetry_time{};

// Thunder rumble trigger for weather
float g_thunder_timer = 12.0f;
float g_thunder_env   = 0.0f;

// Continuous synthesizer DSP state (runs in audio thread)
struct SynthesizerState {
    NoiseGenerator noise_l{};
    NoiseGenerator noise_r{};
    NoiseGenerator noise_aux{};

    StateVariableFilter filter_wind_l{};
    StateVariableFilter filter_wind_r{};
    StateVariableFilter filter_cabin_vent{};
    StateVariableFilter filter_jetpack_hiss{};
    StateVariableFilter filter_rain{};
    StateVariableFilter filter_thunder{};

    // Oscillators phase accumulators
    float phase_cabin_sub = 0.0f;
    float phase_cabin_f1  = 0.0f;
    float phase_cabin_f2  = 0.0f;
    float phase_cabin_f3  = 0.0f;
    float phase_cabin_hum = 0.0f;

    float phase_roof_1 = 0.0f;
    float phase_roof_2 = 0.0f;

    float phase_drive_osc     = 0.0f;
    float phase_drive_vibrato = 0.0f;

    float phase_suit_hum = 0.0f;

    // LFOs
    float lfo_cabin    = 0.0f;
    float lfo_gust_1   = 0.0f;
    float lfo_gust_2   = 0.0f;
    float lfo_roof_pan = 0.0f;

    // Smoothed parameters (interpolation to prevent clicks)
    float cur_cabin_gain     = 0.0f;
    float cur_roof_gain      = 0.0f;
    float cur_surface_gain   = 0.0f;
    float cur_drive_gain     = 0.0f;
    float cur_drive_freq     = 120.0f;
    float cur_atmo_density   = 0.0f;
    float cur_rain_intensity = 0.0f;
    float cur_jetpack_gain   = 0.0f;
    float cur_master_gain    = 0.0f;
};

SynthesizerState g_synth{};

// Audio stream callback (runs on miniaudio playback thread)
void audio_stream_callback(void *bufferData, unsigned int frames) {
    if (!bufferData || frames == 0)
        return;
    auto *out = static_cast<float *>(bufferData);

    AudioTelemetry snap{};
    {
        std::lock_guard<std::mutex> lock(g_telemetry_mutex);
        snap = g_target_telemetry;
    }

    const bool muted       = g_muted.load(std::memory_order_relaxed);
    const float target_vol = muted ? 0.0f : g_master_volume.load(std::memory_order_relaxed);

    // Target gains for scenes
    float target_cabin   = (snap.scene == AudioScene::cabin) ? 0.65f : 0.0f;
    float target_roof    = (snap.scene == AudioScene::roof) ? 0.50f : 0.0f;
    float target_surface = (snap.scene == AudioScene::surface) ? 0.70f : 0.0f;
    float target_drive   = snap.travel_active ? 0.75f : 0.0f;
    float target_jetpack = snap.jetpack_active ? 0.55f : 0.0f;

    // Vimana drive target frequency based on travel phase and progress
    float target_drive_freq = 130.0f + 120.0f * std::clamp(snap.travel_speed, 0.0f, 1.0f);
    if (snap.travel_phase == 14 || snap.travel_phase == 19) { // charging / warming_up
        target_drive_freq = 240.0f;
    }

    constexpr float dt       = 1.0f / static_cast<float>(AUDIO_SAMPLE_RATE);
    constexpr float smooth_k = 0.003f; // Parameter smoothing speed per sample

    for (unsigned int i = 0; i < frames; ++i) {
        // Parameter smoothing
        g_synth.cur_cabin_gain += smooth_k * (target_cabin - g_synth.cur_cabin_gain);
        g_synth.cur_roof_gain += smooth_k * (target_roof - g_synth.cur_roof_gain);
        g_synth.cur_surface_gain += smooth_k * (target_surface - g_synth.cur_surface_gain);
        g_synth.cur_drive_gain += smooth_k * (target_drive - g_synth.cur_drive_gain);
        g_synth.cur_drive_freq += smooth_k * (target_drive_freq - g_synth.cur_drive_freq);
        g_synth.cur_atmo_density += smooth_k * (snap.atmosphere_density - g_synth.cur_atmo_density);
        g_synth.cur_rain_intensity += smooth_k * (snap.weather_rain - g_synth.cur_rain_intensity);
        g_synth.cur_jetpack_gain += smooth_k * (target_jetpack - g_synth.cur_jetpack_gain);
        g_synth.cur_master_gain += smooth_k * (target_vol - g_synth.cur_master_gain);

        // Advance LFOs
        g_synth.lfo_cabin += TWO_PI * 0.06f * dt;
        if (g_synth.lfo_cabin >= TWO_PI)
            g_synth.lfo_cabin -= TWO_PI;

        g_synth.lfo_gust_1 += TWO_PI * 0.11f * dt;
        if (g_synth.lfo_gust_1 >= TWO_PI)
            g_synth.lfo_gust_1 -= TWO_PI;

        g_synth.lfo_gust_2 += TWO_PI * 0.043f * dt;
        if (g_synth.lfo_gust_2 >= TWO_PI)
            g_synth.lfo_gust_2 -= TWO_PI;

        g_synth.lfo_roof_pan += TWO_PI * 0.035f * dt;
        if (g_synth.lfo_roof_pan >= TWO_PI)
            g_synth.lfo_roof_pan -= TWO_PI;

        // -----------------------------------------------------------------
        // 1. Cabin Drone Synthesis
        // -----------------------------------------------------------------
        float cabin_l = 0.0f;
        float cabin_r = 0.0f;
        if (g_synth.cur_cabin_gain > 0.001f) {
            // Oscillators
            g_synth.phase_cabin_sub += TWO_PI * 27.5f * dt;
            if (g_synth.phase_cabin_sub >= TWO_PI)
                g_synth.phase_cabin_sub -= TWO_PI;

            g_synth.phase_cabin_f1 += TWO_PI * 55.0f * dt;
            if (g_synth.phase_cabin_f1 >= TWO_PI)
                g_synth.phase_cabin_f1 -= TWO_PI;

            g_synth.phase_cabin_f2 += TWO_PI * 110.0f * dt;
            if (g_synth.phase_cabin_f2 >= TWO_PI)
                g_synth.phase_cabin_f2 -= TWO_PI;

            g_synth.phase_cabin_f3 += TWO_PI * 165.0f * dt;
            if (g_synth.phase_cabin_f3 >= TWO_PI)
                g_synth.phase_cabin_f3 -= TWO_PI;

            g_synth.phase_cabin_hum += TWO_PI * 440.0f * dt;
            if (g_synth.phase_cabin_hum >= TWO_PI)
                g_synth.phase_cabin_hum -= TWO_PI;

            float sub     = std::sin(g_synth.phase_cabin_sub) * 0.35f;
            float f1      = std::sin(g_synth.phase_cabin_f1) * 0.45f;
            float lfo_mod = 0.8f + 0.2f * std::sin(g_synth.lfo_cabin);
            float f2      = std::sin(g_synth.phase_cabin_f2) * (0.22f * lfo_mod);
            float f3      = std::sin(g_synth.phase_cabin_f3) * (0.10f * lfo_mod);

            // Computer monitor purr
            float hum_l = std::sin(g_synth.phase_cabin_hum) * 0.015f;
            float hum_r = std::sin(g_synth.phase_cabin_hum + 0.7f) * 0.015f;

            // Reactor air ventilation noise
            float vent_noise = g_synth.noise_l.next_pink();
            float vent_l = 0.0f, vent_b = 0.0f, vent_h = 0.0f;
            g_synth.filter_cabin_vent.process(vent_noise, 180.0f, 0.7f, AUDIO_SAMPLE_RATE, vent_l, vent_b, vent_h);

            float mono_drone = sub + f1 + f2 + f3 + vent_l * 0.12f;
            cabin_l          = mono_drone + hum_l;
            cabin_r          = mono_drone + hum_r;
        }

        // -----------------------------------------------------------------
        // 2. Roof Deep Space Synthesis
        // -----------------------------------------------------------------
        float roof_l = 0.0f;
        float roof_r = 0.0f;
        if (g_synth.cur_roof_gain > 0.001f) {
            g_synth.phase_roof_1 += TWO_PI * 36.0f * dt;
            if (g_synth.phase_roof_1 >= TWO_PI)
                g_synth.phase_roof_1 -= TWO_PI;

            g_synth.phase_roof_2 += TWO_PI * 54.0f * dt;
            if (g_synth.phase_roof_2 >= TWO_PI)
                g_synth.phase_roof_2 -= TWO_PI;

            float cosmic_sub = std::sin(g_synth.phase_roof_1) * 0.40f + std::sin(g_synth.phase_roof_2) * 0.25f;

            // Diffuse solar wind background
            float p_left  = g_synth.noise_l.next_pink();
            float p_right = g_synth.noise_r.next_pink();

            float wl = 0.0f, wb = 0.0f, wh = 0.0f;
            g_synth.filter_wind_l.process(p_left, 160.0f, 1.2f, AUDIO_SAMPLE_RATE, wl, wb, wh);

            float wr_l = 0.0f, wr_b = 0.0f, wr_h = 0.0f;
            g_synth.filter_wind_r.process(p_right, 180.0f, 1.2f, AUDIO_SAMPLE_RATE, wr_l, wr_b, wr_h);

            float pan = 0.5f + 0.3f * std::sin(g_synth.lfo_roof_pan);
            roof_l    = cosmic_sub + wb * 0.20f * pan;
            roof_r    = cosmic_sub + wr_b * 0.20f * (1.0f - pan);
        }

        // -----------------------------------------------------------------
        // 3. Vimana Drive Propulsion Acoustics
        // -----------------------------------------------------------------
        float drive_l = 0.0f;
        float drive_r = 0.0f;
        if (g_synth.cur_drive_gain > 0.001f) {
            g_synth.phase_drive_vibrato += TWO_PI * 6.2f * dt;
            if (g_synth.phase_drive_vibrato >= TWO_PI)
                g_synth.phase_drive_vibrato -= TWO_PI;

            float vibrato = std::sin(g_synth.phase_drive_vibrato) * 3.5f;
            float freq    = g_synth.cur_drive_freq + vibrato;

            g_synth.phase_drive_osc += TWO_PI * freq * dt;
            if (g_synth.phase_drive_osc >= TWO_PI)
                g_synth.phase_drive_osc -= TWO_PI;

            // Saturated tone with harmonics
            float raw_osc     = std::sin(g_synth.phase_drive_osc) + 0.35f * std::sin(g_synth.phase_drive_osc * 2.0f);
            float drive_sound = std::tanh(raw_osc * 1.4f) * 0.45f;

            // Low-end thrust rumble
            float thrust_noise = g_synth.noise_aux.next_pink();
            float tl = 0.0f, tb = 0.0f, th = 0.0f;
            g_synth.filter_cabin_vent.process(thrust_noise, 75.0f, 1.5f, AUDIO_SAMPLE_RATE, tl, tb, th);

            drive_l = drive_sound + tb * 0.25f;
            drive_r = drive_sound + tb * 0.25f;
        }

        // -----------------------------------------------------------------
        // 4. Planetary Surface Wind & Weather
        // -----------------------------------------------------------------
        float surface_l = 0.0f;
        float surface_r = 0.0f;
        if (g_synth.cur_surface_gain > 0.001f) {
            if (g_synth.cur_atmo_density <= 0.001f) {
                // VACUUM (Airless Moon / Asteroid): Dead silence outside!
                // Only gentle internal spacesuit life support hum
                g_synth.phase_suit_hum += TWO_PI * 92.0f * dt;
                if (g_synth.phase_suit_hum >= TWO_PI)
                    g_synth.phase_suit_hum -= TWO_PI;

                float suit_tone = std::sin(g_synth.phase_suit_hum) * 0.04f;
                float suit_air  = g_synth.noise_l.next_pink() * 0.025f;
                surface_l       = suit_tone + suit_air;
                surface_r       = suit_tone + suit_air;
            } else {
                // ATMOSPHERIC WORLD: Dynamic wind, gusts, and weather
                float gust = 0.5f + 0.35f * std::sin(g_synth.lfo_gust_1) + 0.15f * std::sin(g_synth.lfo_gust_2);

                float density     = g_synth.cur_atmo_density;
                float base_cutoff = 100.0f + 650.0f * (density / (1.0f + density));
                float wind_cutoff = base_cutoff * (0.65f + 0.70f * gust);
                float wind_q      = 1.0f + 1.5f * (1.0f / (0.5f + density));

                float nl = g_synth.noise_l.next_pink();
                float nr = g_synth.noise_r.next_pink();

                float wl_low = 0.0f, wl_band = 0.0f, wl_high = 0.0f;
                g_synth.filter_wind_l.process(nl, wind_cutoff, wind_q, AUDIO_SAMPLE_RATE, wl_low, wl_band, wl_high);

                float wr_low = 0.0f, wr_band = 0.0f, wr_high = 0.0f;
                g_synth.filter_wind_r.process(nr, wind_cutoff * 1.05f, wind_q, AUDIO_SAMPLE_RATE, wr_low, wr_band,
                                              wr_high);

                float wind_vol = std::clamp(std::sqrt(density), 0.1f, 1.0f) * (0.45f + 0.55f * gust);
                surface_l      = (wl_low * 0.6f + wl_band * 0.4f) * wind_vol;
                surface_r      = (wr_low * 0.6f + wr_band * 0.4f) * wind_vol;

                // Rain & weather
                if (g_synth.cur_rain_intensity > 0.05f) {
                    float rain_noise = g_synth.noise_aux.next_white();
                    float rl = 0.0f, rb = 0.0f, rh = 0.0f;
                    g_synth.filter_rain.process(rain_noise, 2600.0f, 0.7f, AUDIO_SAMPLE_RATE, rl, rb, rh);
                    float rain_vol = (g_synth.cur_rain_intensity / 5.0f) * 0.18f;
                    surface_l += rh * rain_vol;
                    surface_r += rh * rain_vol;

                    // Distant thunder rumble
                    if (g_thunder_env > 0.001f) {
                        float thunder_noise = g_synth.noise_l.next_pink();
                        float t_low = 0.0f, t_band = 0.0f, t_high = 0.0f;
                        g_synth.filter_thunder.process(thunder_noise, 50.0f, 2.0f, AUDIO_SAMPLE_RATE, t_low, t_band,
                                                       t_high);
                        float thunder = t_low * g_thunder_env * 0.35f;
                        surface_l += thunder;
                        surface_r += thunder;
                        g_thunder_env -= 0.00003f; // Gradual decay
                    }
                }
            }

            // Jetpack thruster sustain hiss
            if (g_synth.cur_jetpack_gain > 0.001f) {
                float thruster_noise = g_synth.noise_aux.next_white();
                float jl = 0.0f, jb = 0.0f, jh = 0.0f;
                g_synth.filter_jetpack_hiss.process(thruster_noise, 1350.0f, 1.2f, AUDIO_SAMPLE_RATE, jl, jb, jh);
                float hiss = (jb * 0.7f + jh * 0.3f) * g_synth.cur_jetpack_gain * 0.40f;
                surface_l += hiss;
                surface_r += hiss;
            }
        }

        // -----------------------------------------------------------------
        // 5. Final Stereo Mix & Soft Limiter
        // -----------------------------------------------------------------
        float mix_l = cabin_l * g_synth.cur_cabin_gain + roof_l * g_synth.cur_roof_gain +
                      surface_l * g_synth.cur_surface_gain + drive_l * g_synth.cur_drive_gain;

        float mix_r = cabin_r * g_synth.cur_cabin_gain + roof_r * g_synth.cur_roof_gain +
                      surface_r * g_synth.cur_surface_gain + drive_r * g_synth.cur_drive_gain;

        mix_l *= g_synth.cur_master_gain;
        mix_r *= g_synth.cur_master_gain;

        // Limiter
        out[i * 2 + 0] = std::tanh(mix_l);
        out[i * 2 + 1] = std::tanh(mix_r);
    }
}

} // namespace

void initialize_audio() {
    if (g_audio_ready)
        return;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        g_audio_ready = false;
        return;
    }

    SetMasterVolume(g_master_volume.load());

    // 1. Procedural Foley Sound Effects:
    // Torch Switch On: sharp 3200Hz contact transient followed by 2200Hz mechanical latch click
    auto wave_torch_on = make_procedural_wave(1102, [](int /*i*/, float t) {
        float pulse1 = std::exp(-t / 0.0018f) * std::sin(TWO_PI * 3200.0f * t);
        float pulse2 = 0.0f;
        if (t >= 0.007f) {
            float dt = t - 0.007f;
            pulse2   = 0.85f * std::exp(-dt / 0.0025f) * std::sin(TWO_PI * 2200.0f * dt);
        }
        return (pulse1 + pulse2) * 0.85f;
    });
    g_sound_torch_on   = LoadSoundFromWave(wave_torch_on);
    UnloadWave(wave_torch_on);
    SetSoundVolume(g_sound_torch_on, 0.70f);

    // Torch Switch Off: slightly lower 2800Hz / 1750Hz mechanical release click
    auto wave_torch_off = make_procedural_wave(1102, [](int /*i*/, float t) {
        float pulse1 = std::exp(-t / 0.0020f) * std::sin(TWO_PI * 2800.0f * t);
        float pulse2 = 0.0f;
        if (t >= 0.008f) {
            float dt = t - 0.008f;
            pulse2   = 0.80f * std::exp(-dt / 0.0032f) * std::sin(TWO_PI * 1750.0f * dt);
        }
        return (pulse1 + pulse2) * 0.80f;
    });
    g_sound_torch_off   = LoadSoundFromWave(wave_torch_off);
    UnloadWave(wave_torch_off);
    SetSoundVolume(g_sound_torch_off, 0.70f);

    // Visor Servo Motor: 520Hz -> 720Hz micro-stepper motor sweep with 60Hz PWM pulse modulation
    auto wave_visor = make_procedural_wave(6174, [](int /*i*/, float t) {
        constexpr float duration = 0.14f;
        float progress           = t / duration;
        float f_start            = 520.0f;
        float f_end              = 720.0f;
        // Phase integral of linear frequency sweep
        float phase = TWO_PI * (f_start * t + 0.5f * (f_end - f_start) * t * progress);
        float tone  = std::sin(phase) + 0.35f * std::sin(phase * 2.0f);
        // PWM motor pulse modulation
        float pwm = 0.80f + 0.20f * (std::sin(TWO_PI * 60.0f * t) >= 0.0f ? 1.0f : -1.0f);
        // Envelope: 12ms attack, 25ms decay
        float env = 1.0f;
        if (t < 0.012f)
            env = t / 0.012f;
        else if (t > duration - 0.025f)
            env = (duration - t) / 0.025f;
        return tone * pwm * env * 0.65f;
    });
    g_sound_visor   = LoadSoundFromWave(wave_visor);
    UnloadWave(wave_visor);
    SetSoundVolume(g_sound_visor, 0.60f);

    // Jetpack Gas Burst: initial pressure valve pop + bandpass filtered gas rush
    NoiseGenerator jetpack_noise{424242};
    StateVariableFilter jetpack_filter{};
    auto wave_jetpack = make_procedural_wave(10584, [&jetpack_noise, &jetpack_filter](int /*i*/, float t) {
        constexpr float duration = 0.24f;
        float pop                = std::exp(-t / 0.020f) * std::sin(TWO_PI * 120.0f * t) * 0.6f;
        float noise              = jetpack_noise.next_white();
        float l = 0.0f, b = 0.0f, h = 0.0f;
        jetpack_filter.process(noise, 1400.0f, 1.2f, AUDIO_SAMPLE_RATE, l, b, h);
        float gas_env = std::exp(-t / 0.085f);
        if (t < 0.015f)
            gas_env *= (t / 0.015f);
        return (pop + b * gas_env * 0.75f) * 0.85f;
    });
    g_sound_jetpack   = LoadSoundFromWave(wave_jetpack);
    UnloadWave(wave_jetpack);
    SetSoundVolume(g_sound_jetpack, 0.75f);

    // Footsteps: 3 subtle pitch/regolith crunch variations
    constexpr std::array<float, 3> step_freqs{68.0f, 82.0f, 74.0f};
    for (std::size_t k = 0; k < 3; ++k) {
        NoiseGenerator step_noise{static_cast<std::uint32_t>(7777 + k * 1337)};
        float f              = step_freqs[k];
        auto wave_step       = make_procedural_wave(2866, [f, &step_noise](int /*i*/, float t) {
            float heel   = std::exp(-t / 0.022f) * std::sin(TWO_PI * f * t) * 0.75f;
            float crunch = std::exp(-t / 0.028f) * step_noise.next_pink() * 0.35f;
            return (heel + crunch) * 0.40f;
        });
        g_sound_footsteps[k] = LoadSoundFromWave(wave_step);
        UnloadWave(wave_step);
        SetSoundVolume(g_sound_footsteps[k], 0.35f);
    }

    // 2. Continuous Procedural Ambient Stream:
    g_ambient_stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 32, 2);
    if (IsAudioStreamValid(g_ambient_stream)) {
        SetAudioStreamCallback(g_ambient_stream, audio_stream_callback);
        PlayAudioStream(g_ambient_stream);
    }

    g_last_telemetry_time = std::chrono::steady_clock::now();
    g_audio_ready         = true;
}

void shutdown_audio() {
    if (!g_audio_ready)
        return;

    if (IsAudioStreamValid(g_ambient_stream)) {
        StopAudioStream(g_ambient_stream);
        UnloadAudioStream(g_ambient_stream);
        g_ambient_stream = {};
    }

    if (IsSoundValid(g_sound_torch_on))
        UnloadSound(g_sound_torch_on);
    if (IsSoundValid(g_sound_torch_off))
        UnloadSound(g_sound_torch_off);
    if (IsSoundValid(g_sound_visor))
        UnloadSound(g_sound_visor);
    if (IsSoundValid(g_sound_jetpack))
        UnloadSound(g_sound_jetpack);
    for (auto &snd : g_sound_footsteps) {
        if (IsSoundValid(snd))
            UnloadSound(snd);
    }

    CloseAudioDevice();
    g_audio_ready = false;
}

bool is_audio_ready() { return g_audio_ready; }

void update_audio_telemetry(const AudioTelemetry &telemetry) {
    if (!g_audio_ready)
        return;

    auto now              = std::chrono::steady_clock::now();
    float dt              = std::chrono::duration<float>(now - g_last_telemetry_time).count();
    g_last_telemetry_time = now;
    if (dt > 0.2f)
        dt = 0.055f; // Clamp large pauses / initial dt

    // Footstep pacing logic when walking on planetary terrain
    if (telemetry.scene == AudioScene::surface && telemetry.player_walking) {
        g_footstep_timer += dt;
        if (g_footstep_timer >= 0.38f) { // ~380ms per stride
            g_footstep_timer = 0.0f;
            play_surface_footstep();
        }
    } else {
        g_footstep_timer = 0.2f;
    }

    // Weather thunder trigger
    if (telemetry.scene == AudioScene::surface && telemetry.weather_rain >= 2.0f) {
        g_thunder_timer -= dt;
        if (g_thunder_timer <= 0.0f) {
            g_thunder_env = 1.0f;
            // Next thunder in 12-25 seconds
            g_thunder_timer = 12.0f + static_cast<float>(std::rand() % 13);
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_telemetry_mutex);
        g_target_telemetry = telemetry;
    }
}

void play_torch_click(bool turning_on) {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(turning_on ? g_sound_torch_on : g_sound_torch_off);
}

void play_visor_servo() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_visor);
}

void play_jetpack_burst() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_jetpack);
}

void play_surface_footstep() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_footsteps[g_footstep_index]);
    g_footstep_index = (g_footstep_index + 1) % 3;
}

void set_audio_muted(bool muted) { g_muted.store(muted); }

bool is_audio_muted() { return g_muted.load(); }

void toggle_audio_mute() { g_muted.store(!g_muted.load()); }

void set_master_volume_level(float volume) {
    float clamped = std::clamp(volume, 0.0f, 1.0f);
    g_master_volume.store(clamped);
    if (g_audio_ready && IsAudioDeviceReady()) {
        SetMasterVolume(clamped);
    }
}

float get_master_volume_level() { return g_master_volume.load(); }

} // namespace noctis
