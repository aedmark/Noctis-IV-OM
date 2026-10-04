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
Sound g_sound_rcs_burst{};
Sound g_sound_touchdown{};
Sound g_sound_cockpit_button{};
std::array<Sound, 3> g_sound_terminal_keys{};
Sound g_sound_goes_transmit{};
Sound g_sound_goes_ack{};
Sound g_sound_goes_nack{};
Sound g_sound_terminal_scroll{};
Sound g_sound_deck_lift{};
std::array<Sound, 3> g_sound_footsteps{};

// Footstep, terminal, and RCS timing
float g_footstep_timer   = 0.2f;
int g_footstep_index     = 0;
int g_terminal_key_index = 0;
bool g_last_rcs_active   = false;
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
    StateVariableFilter filter_cabin_vent_l{};
    StateVariableFilter filter_cabin_vent_r{};
    StateVariableFilter filter_drive_l{};
    StateVariableFilter filter_drive_r{};
    StateVariableFilter filter_drive_rumble{};
    StateVariableFilter filter_jetpack_hiss{};
    StateVariableFilter filter_rcs_hiss_l{};
    StateVariableFilter filter_rcs_hiss_r{};
    StateVariableFilter filter_entry_buffet_l{};
    StateVariableFilter filter_entry_buffet_r{};
    StateVariableFilter filter_rain{};
    StateVariableFilter filter_thunder{};
    StateVariableFilter filter_suit_vent_l{};
    StateVariableFilter filter_suit_vent_r{};

    // Oscillators phase accumulators
    float phase_cabin_sub = 0.0f;
    float phase_cabin_f1  = 0.0f;
    float phase_cabin_f2  = 0.0f;

    float phase_roof_1 = 0.0f;
    float phase_roof_2 = 0.0f;

    float phase_drive_sub    = 0.0f;
    float phase_drive_root   = 0.0f;
    float phase_drive_chorus = 0.0f;
    float phase_drive_fifth  = 0.0f;
    float phase_drive_pulse  = 0.0f;

    float phase_suit_hum = 0.0f;

    // LFOs
    float lfo_cabin    = 0.0f;
    float lfo_gust_1   = 0.0f;
    float lfo_gust_2   = 0.0f;
    float lfo_roof_pan = 0.0f;
    float lfo_buffet   = 0.0f;

    // Arrival / deceleration spool-down
    bool was_travel_active = false;
    float spool_down_timer = 0.0f;

    // Smoothed parameters (interpolation to prevent clicks)
    float cur_cabin_gain     = 0.0f;
    float cur_roof_gain      = 0.0f;
    float cur_surface_gain   = 0.0f;
    float cur_drive_gain     = 0.0f;
    float cur_drive_freq     = 44.0f;
    float cur_drive_speed    = 0.0f;
    float cur_atmo_density   = 0.0f;
    float cur_rain_intensity = 0.0f;
    float cur_jetpack_gain   = 0.0f;
    float cur_rcs_gain       = 0.0f;
    float cur_buffet_gain    = 0.0f;
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

    constexpr float dt                  = 1.0f / static_cast<float>(AUDIO_SAMPLE_RATE);
    constexpr float smooth_k            = 0.003f; // Parameter smoothing speed per sample
    constexpr float SPOOL_DOWN_DURATION = 1.8f;   // 1.8s smooth field dissipation on arrival / disengage

    // Detect travel shutdown / arrival transition
    if (g_synth.was_travel_active && !snap.travel_active) {
        g_synth.spool_down_timer = SPOOL_DOWN_DURATION;
    } else if (snap.travel_active) {
        g_synth.spool_down_timer = 0.0f;
    }
    g_synth.was_travel_active = snap.travel_active;

    for (unsigned int i = 0; i < frames; ++i) {
        // Advance spool-down dissipation envelope
        float spool_gain = 0.0f;
        if (g_synth.spool_down_timer > 0.0f) {
            g_synth.spool_down_timer -= dt;
            if (g_synth.spool_down_timer < 0.0f)
                g_synth.spool_down_timer = 0.0f;
            float progress = g_synth.spool_down_timer / SPOOL_DOWN_DURATION;
            // Smooth cosine S-curve decay
            spool_gain     = 0.5f * (1.0f - std::cos(progress * static_cast<float>(M_PI)));
        }

        // Target drive gain: full sustain while traveling, smooth dissipation on arrival (no hard cut)
        float target_drive = 0.0f;
        if (snap.travel_active) {
            target_drive = 0.30f;
        } else if (spool_gain > 0.001f) {
            target_drive = 0.30f * spool_gain;
        }

        // Target cabin gain: ducked during warp, smoothly recovers to 0.20 as drive spools down
        float target_cabin = 0.0f;
        if (snap.scene == AudioScene::cabin) {
            if (snap.travel_active) {
                target_cabin = 0.10f;
            } else if (spool_gain > 0.001f) {
                target_cabin = 0.20f - 0.10f * spool_gain;
            } else {
                target_cabin = 0.20f;
            }
        }
        float target_roof    = (snap.scene == AudioScene::roof) ? 0.28f : 0.0f;
        float target_surface = (snap.scene == AudioScene::surface) ? 0.45f : 0.0f;
        float target_jetpack = snap.jetpack_active ? 0.32f : 0.0f;
        float target_rcs     = (snap.rcs_active && !snap.travel_active &&
                               (snap.scene == AudioScene::cabin || snap.scene == AudioScene::roof))
                                   ? 0.22f
                                   : 0.0f;
        float target_buffet  = std::clamp(snap.entry_buffeting, 0.0f, 1.0f) * 0.55f;

        // Drive frequency: starts low at ignition (44Hz), rises with speed to 96Hz, drops as travel slows to stop
        float target_drive_freq = 44.0f + 52.0f * snap.travel_speed;
        if (!snap.travel_active && spool_gain > 0.001f) {
            // During arrival spool-down, pitch dissipates smoothly from 44Hz down to 28Hz
            target_drive_freq = 28.0f + 16.0f * spool_gain;
        }

        // Parameter smoothing
        g_synth.cur_cabin_gain += smooth_k * (target_cabin - g_synth.cur_cabin_gain);
        g_synth.cur_roof_gain += smooth_k * (target_roof - g_synth.cur_roof_gain);
        g_synth.cur_surface_gain += smooth_k * (target_surface - g_synth.cur_surface_gain);
        g_synth.cur_drive_gain += smooth_k * (target_drive - g_synth.cur_drive_gain);
        g_synth.cur_drive_freq += smooth_k * (target_drive_freq - g_synth.cur_drive_freq);
        g_synth.cur_drive_speed += smooth_k * (snap.travel_speed - g_synth.cur_drive_speed);
        g_synth.cur_atmo_density += smooth_k * (snap.atmosphere_density - g_synth.cur_atmo_density);
        g_synth.cur_rain_intensity += smooth_k * (snap.weather_rain - g_synth.cur_rain_intensity);
        g_synth.cur_jetpack_gain += smooth_k * (target_jetpack - g_synth.cur_jetpack_gain);
        g_synth.cur_rcs_gain += smooth_k * (target_rcs - g_synth.cur_rcs_gain);
        g_synth.cur_buffet_gain += smooth_k * (target_buffet - g_synth.cur_buffet_gain);
        g_synth.cur_master_gain += smooth_k * (target_vol - g_synth.cur_master_gain);

        // Advance LFOs
        g_synth.lfo_cabin += TWO_PI * 0.04f * dt; // Slow 25s life-support cycle
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

        g_synth.lfo_buffet += TWO_PI * 6.5f * dt; // 6.5 Hz turbulent buffeting cycle
        if (g_synth.lfo_buffet >= TWO_PI)
            g_synth.lfo_buffet -= TWO_PI;

        // -----------------------------------------------------------------
        // 1. Cabin Drone Synthesis (Warm, Soothing Stardrifter Interior)
        // -----------------------------------------------------------------
        float cabin_l = 0.0f;
        float cabin_r = 0.0f;
        if (g_synth.cur_cabin_gain > 0.001f) {
            // Very slow, soothing ventilation breath (0.04 Hz)
            float vent_breath = 0.82f + 0.18f * std::sin(g_synth.lfo_cabin);

            // Deep foundation sub-rumble (32.0 Hz)
            g_synth.phase_cabin_sub += TWO_PI * 32.0f * dt;
            if (g_synth.phase_cabin_sub >= TWO_PI)
                g_synth.phase_cabin_sub -= TWO_PI;

            // Warm fundamental tone (55.0 Hz)
            g_synth.phase_cabin_f1 += TWO_PI * 55.0f * dt;
            if (g_synth.phase_cabin_f1 >= TWO_PI)
                g_synth.phase_cabin_f1 -= TWO_PI;

            // Soft second harmonic (110.0 Hz) - gentle, non-fatiguing
            g_synth.phase_cabin_f2 += TWO_PI * 110.0f * dt;
            if (g_synth.phase_cabin_f2 >= TWO_PI)
                g_synth.phase_cabin_f2 -= TWO_PI;

            float sub = std::sin(g_synth.phase_cabin_sub) * 0.22f;
            float f1  = std::sin(g_synth.phase_cabin_f1) * 0.12f;
            float f2  = std::sin(g_synth.phase_cabin_f2) * 0.04f;

            // Stereo life support ventilation air noise (pink noise warmly lowpassed at 110 Hz)
            float vent_in_l = g_synth.noise_l.next_pink();
            float vent_in_r = g_synth.noise_r.next_pink();

            float vl_low = 0.0f, vl_band = 0.0f, vl_high = 0.0f;
            float vr_low = 0.0f, vr_band = 0.0f, vr_high = 0.0f;
            g_synth.filter_cabin_vent_l.process(vent_in_l, 110.0f, 0.6f, AUDIO_SAMPLE_RATE, vl_low, vl_band, vl_high);
            g_synth.filter_cabin_vent_r.process(vent_in_r, 110.0f, 0.6f, AUDIO_SAMPLE_RATE, vr_low, vr_band, vr_high);

            float vent_l = vl_low * 0.15f * vent_breath;
            float vent_r = vr_low * 0.15f * vent_breath;

            cabin_l = sub + f1 + f2 + vent_l;
            cabin_r = sub + f1 + f2 + vent_r;
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
        // 3. Vimana Drive Propulsion Acoustics (Majestic Gravitic Warp)
        // -----------------------------------------------------------------
        float drive_l = 0.0f;
        float drive_r = 0.0f;
        if (g_synth.cur_drive_gain > 0.001f) {
            // Gravitic warp pulse LFO:
            // Starts slow at ignition (~0.70 Hz), oscillates faster as speed increases (up to ~3.80 Hz at top warp),
            // and slows down again as travel decelerates to a stop.
            float warp_pulse_rate = 0.70f + 3.10f * g_synth.cur_drive_speed;
            if (!snap.travel_active && spool_gain > 0.001f) {
                warp_pulse_rate = 0.70f * spool_gain;
            }
            g_synth.phase_drive_pulse += TWO_PI * warp_pulse_rate * dt;
            if (g_synth.phase_drive_pulse >= TWO_PI)
                g_synth.phase_drive_pulse -= TWO_PI;

            // Modulation depth increases with speed: subtle at low speed, rhythmic throb at warp
            float pulse_depth = 0.15f + 0.15f * g_synth.cur_drive_speed;
            float warp_pulse  = (1.0f - pulse_depth) + pulse_depth * std::sin(g_synth.phase_drive_pulse);

            float freq = g_synth.cur_drive_freq;

            // Sub-bass gravitic field (half frequency, ~14 to 48 Hz)
            g_synth.phase_drive_sub += TWO_PI * (freq * 0.5f) * dt;
            if (g_synth.phase_drive_sub >= TWO_PI)
                g_synth.phase_drive_sub -= TWO_PI;

            // Root warp oscillator
            g_synth.phase_drive_root += TWO_PI * freq * dt;
            if (g_synth.phase_drive_root >= TWO_PI)
                g_synth.phase_drive_root -= TWO_PI;

            // Chorused detuned oscillator for lush stereo depth
            g_synth.phase_drive_chorus += TWO_PI * (freq * 1.006f) * dt;
            if (g_synth.phase_drive_chorus >= TWO_PI)
                g_synth.phase_drive_chorus -= TWO_PI;

            // Gentle musical fifth harmonic overtone (quiet, warm)
            g_synth.phase_drive_fifth += TWO_PI * (freq * 1.498f) * dt;
            if (g_synth.phase_drive_fifth >= TWO_PI)
                g_synth.phase_drive_fifth -= TWO_PI;

            float sub_tone   = std::sin(g_synth.phase_drive_sub) * 0.32f;
            float osc_left   = std::sin(g_synth.phase_drive_root);
            float osc_right  = std::sin(g_synth.phase_drive_chorus);
            float fifth_tone = std::sin(g_synth.phase_drive_fifth) * 0.12f;

            // Warm, rounded resonance without harsh clipping
            float core_l = (sub_tone + osc_left * 0.35f + fifth_tone) * warp_pulse;
            float core_r = (sub_tone + osc_right * 0.35f + fifth_tone) * warp_pulse;

            // Deep hull rumble (pink noise lowpassed at 70 Hz)
            float hull_noise = g_synth.noise_aux.next_pink();
            float tl = 0.0f, tb = 0.0f, th = 0.0f;
            g_synth.filter_drive_rumble.process(hull_noise, 70.0f, 0.8f, AUDIO_SAMPLE_RATE, tl, tb, th);

            // Filter drive core through lowpass filter to ensure zero high-frequency harshness
            float fl_l = 0.0f, fl_b = 0.0f, fl_h = 0.0f;
            float fr_l = 0.0f, fr_b = 0.0f, fr_h = 0.0f;
            g_synth.filter_drive_l.process(core_l, 220.0f, 0.7f, AUDIO_SAMPLE_RATE, fl_l, fl_b, fl_h);
            g_synth.filter_drive_r.process(core_r, 220.0f, 0.7f, AUDIO_SAMPLE_RATE, fr_l, fr_b, fr_h);

            drive_l = fl_l + tl * 0.20f;
            drive_r = fr_l + tl * 0.20f;
        }

        // -----------------------------------------------------------------
        // 4. Planetary Surface Wind & Weather
        // -----------------------------------------------------------------
        float surface_l = 0.0f;
        float surface_r = 0.0f;
        if (g_synth.cur_surface_gain > 0.001f) {
            if (g_synth.cur_atmo_density <= 0.001f) {
                // VACUUM (Airless Moon / Asteroid): Silence outside;
                // gentle internal spacesuit life support ventilation
                g_synth.phase_suit_hum += TWO_PI * 36.0f * dt;
                if (g_synth.phase_suit_hum >= TWO_PI)
                    g_synth.phase_suit_hum -= TWO_PI;

                float suit_sub = std::sin(g_synth.phase_suit_hum) * 0.012f;

                // Soft spacesuit ventilation air noise (pink noise smoothly lowpassed at 120 Hz)
                float suit_pink_l = g_synth.noise_l.next_pink();
                float suit_pink_r = g_synth.noise_r.next_pink();

                float sv_l_low = 0.0f, sv_l_band = 0.0f, sv_l_high = 0.0f;
                float sv_r_low = 0.0f, sv_r_band = 0.0f, sv_r_high = 0.0f;
                g_synth.filter_suit_vent_l.process(suit_pink_l, 120.0f, 0.6f, AUDIO_SAMPLE_RATE, sv_l_low, sv_l_band, sv_l_high);
                g_synth.filter_suit_vent_r.process(suit_pink_r, 120.0f, 0.6f, AUDIO_SAMPLE_RATE, sv_r_low, sv_r_band, sv_r_high);

                float suit_air_l = sv_l_low * 0.08f;
                float suit_air_r = sv_r_low * 0.08f;

                surface_l = suit_sub + suit_air_l;
                surface_r = suit_sub + suit_air_r;
            } else {
                // ATMOSPHERIC WORLD: Dynamic wind, gusts, and weather
                float gust = 0.5f + 0.35f * std::sin(g_synth.lfo_gust_1) + 0.15f * std::sin(g_synth.lfo_gust_2);

                float density     = g_synth.cur_atmo_density;
                float base_cutoff = 100.0f + 650.0f * (density / (1.0f + density));
                float wind_cutoff = base_cutoff * (0.65f + 0.70f * gust);
                // Clamp wind_q to prevent harsh resonance ringing / whistling at low density
                float wind_q      = std::clamp(0.65f + 0.40f * (1.0f / (1.0f + density)), 0.65f, 1.10f);

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
        // 5. Sublight RCS Attitude Thruster Continuous Hiss
        // -----------------------------------------------------------------
        float rcs_l = 0.0f;
        float rcs_r = 0.0f;
        if (g_synth.cur_rcs_gain > 0.001f) {
            float rcs_nl = g_synth.noise_l.next_white();
            float rcs_nr = g_synth.noise_r.next_white();
            float rcs_ll = 0.0f, rcs_lb = 0.0f, rcs_lh = 0.0f;
            float rcs_rl = 0.0f, rcs_rb = 0.0f, rcs_rh = 0.0f;
            g_synth.filter_rcs_hiss_l.process(rcs_nl, 1900.0f, 1.3f, AUDIO_SAMPLE_RATE, rcs_ll, rcs_lb, rcs_lh);
            g_synth.filter_rcs_hiss_r.process(rcs_nr, 1950.0f, 1.3f, AUDIO_SAMPLE_RATE, rcs_rl, rcs_rb, rcs_rh);
            rcs_l = (rcs_lb * 0.75f + rcs_lh * 0.25f) * g_synth.cur_rcs_gain;
            rcs_r = (rcs_rb * 0.75f + rcs_rh * 0.25f) * g_synth.cur_rcs_gain;
        }

        // -----------------------------------------------------------------
        // 6. Atmospheric Entry Buffeting Turbulence
        // -----------------------------------------------------------------
        float buffet_l = 0.0f;
        float buffet_r = 0.0f;
        if (g_synth.cur_buffet_gain > 0.001f) {
            float buffet_turb = 0.70f + 0.30f * std::sin(g_synth.lfo_buffet);
            float b_noise_l   = g_synth.noise_l.next_pink();
            float b_noise_r   = g_synth.noise_r.next_pink();
            float b_cutoff    = 70.0f + 60.0f * (g_synth.cur_buffet_gain / 0.55f);
            float bl_low = 0.0f, bl_band = 0.0f, bl_high = 0.0f;
            float br_low = 0.0f, br_band = 0.0f, br_high = 0.0f;
            g_synth.filter_entry_buffet_l.process(b_noise_l, b_cutoff, 1.8f, AUDIO_SAMPLE_RATE, bl_low, bl_band, bl_high);
            g_synth.filter_entry_buffet_r.process(b_noise_r, b_cutoff * 1.05f, 1.8f, AUDIO_SAMPLE_RATE, br_low, br_band, br_high);
            buffet_l = (bl_low * 0.80f + bl_band * 0.20f) * buffet_turb * g_synth.cur_buffet_gain;
            buffet_r = (br_low * 0.80f + br_band * 0.20f) * buffet_turb * g_synth.cur_buffet_gain;
        }

        // -----------------------------------------------------------------
        // 7. Final Stereo Mix & Soft Limiter
        // -----------------------------------------------------------------
        float mix_l = cabin_l * g_synth.cur_cabin_gain + roof_l * g_synth.cur_roof_gain +
                      surface_l * g_synth.cur_surface_gain + drive_l * g_synth.cur_drive_gain +
                      rcs_l + buffet_l;

        float mix_r = cabin_r * g_synth.cur_cabin_gain + roof_r * g_synth.cur_roof_gain +
                      surface_r * g_synth.cur_surface_gain + drive_r * g_synth.cur_drive_gain +
                      rcs_r + buffet_r;

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
    SetSoundVolume(g_sound_jetpack, 0.55f);

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

    // Sublight RCS Valve Burst: crisp cold-gas valve pop + 2100Hz bandpass rush
    NoiseGenerator rcs_noise{98765};
    StateVariableFilter rcs_filter{};
    auto wave_rcs = make_procedural_wave(5292, [&rcs_noise, &rcs_filter](int /*i*/, float t) {
        constexpr float duration = 0.12f;
        float pop   = std::exp(-t / 0.008f) * std::sin(TWO_PI * 340.0f * t) * 0.45f;
        float noise = rcs_noise.next_white();
        float l = 0.0f, b = 0.0f, h = 0.0f;
        rcs_filter.process(noise, 2100.0f, 1.4f, AUDIO_SAMPLE_RATE, l, b, h);
        float gas_env = std::exp(-t / 0.035f);
        if (t < 0.008f)
            gas_env *= (t / 0.008f);
        return (pop + b * gas_env * 0.85f) * 0.70f;
    });
    g_sound_rcs_burst = LoadSoundFromWave(wave_rcs);
    UnloadWave(wave_rcs);
    SetSoundVolume(g_sound_rcs_burst, 0.45f);

    // Touchdown Mechanical Clunk: dual-stage heavy metallic thud + latch ring + regolith crunch
    NoiseGenerator td_noise{54321};
    auto wave_touchdown = make_procedural_wave(17640, [&td_noise](int /*i*/, float t) {
        constexpr float duration = 0.40f;
        // 1. Heavy low-frequency hull thud with pitch dropping 85Hz down to 35Hz
        float phase_thud = TWO_PI * (85.0f * t - 25.0f * (t * t / duration));
        float thud       = std::exp(-t / 0.075f) * std::sin(phase_thud) * 0.85f;

        // 2. Dual damped metallic latch rings (720 Hz and 1150 Hz)
        float click = 0.0f;
        if (t >= 0.008f) {
            float dt_click  = t - 0.008f;
            float env_click = std::exp(-dt_click / 0.030f);
            click = env_click * (std::sin(TWO_PI * 720.0f * dt_click) * 0.40f +
                                 std::sin(TWO_PI * 1150.0f * dt_click) * 0.25f);
        }

        // 3. Regolith compression surface crunch
        float crunch = std::exp(-t / 0.045f) * td_noise.next_pink() * 0.35f;

        return (thud + click + crunch) * 0.75f;
    });
    g_sound_touchdown = LoadSoundFromWave(wave_touchdown);
    UnloadWave(wave_touchdown);
    SetSoundVolume(g_sound_touchdown, 0.70f);

    // Cockpit Push-Button: crisp contact transient + dashboard body thud
    auto wave_btn = make_procedural_wave(1764, [](int /*i*/, float t) {
        float click = std::exp(-t / 0.003f) * std::sin(TWO_PI * 1400.0f * t) * 0.7f;
        float body  = std::exp(-t / 0.018f) * std::sin(TWO_PI * 220.0f * t) * 0.5f;
        return (click + body) * 0.75f;
    });
    g_sound_cockpit_button = LoadSoundFromWave(wave_btn);
    UnloadWave(wave_btn);
    SetSoundVolume(g_sound_cockpit_button, 0.55f);

    // Terminal Keyboard Keystrokes: 3 subtle mechanical variations
    constexpr std::array<float, 3> key_high_freqs{2400.0f, 2750.0f, 2550.0f};
    constexpr std::array<float, 3> key_low_freqs{360.0f, 390.0f, 340.0f};
    for (std::size_t k = 0; k < 3; ++k) {
        float f_high = key_high_freqs[k];
        float f_low  = key_low_freqs[k];
        auto wave_key = make_procedural_wave(1543, [f_high, f_low](int /*i*/, float t) {
            float snap = std::exp(-t / 0.0022f) * std::sin(TWO_PI * f_high * t) * 0.75f;
            float thud = std::exp(-t / 0.012f) * std::sin(TWO_PI * f_low * t) * 0.45f;
            return (snap + thud) * 0.55f;
        });
        g_sound_terminal_keys[k] = LoadSoundFromWave(wave_key);
        UnloadWave(wave_key);
        SetSoundVolume(g_sound_terminal_keys[k], 0.45f);
    }

    // GOESnet Transmit Burst: stepped frequency packet chirp (1050 -> 1680 -> 2520 Hz)
    auto wave_xmit = make_procedural_wave(3748, [](int /*i*/, float t) {
        float freq = 1050.0f;
        if (t >= 0.050f) {
            freq = 2520.0f;
        } else if (t >= 0.025f) {
            freq = 1680.0f;
        }
        float env = 1.0f;
        if (t < 0.005f) {
            env = t / 0.005f;
        } else if (t > 0.080f) {
            env = (0.085f - t) / 0.005f;
        }
        float tone = std::sin(TWO_PI * freq * t) + 0.3f * std::sin(TWO_PI * freq * 2.0f * t);
        return tone * env * 0.45f;
    });
    g_sound_goes_transmit = LoadSoundFromWave(wave_xmit);
    UnloadWave(wave_xmit);
    SetSoundVolume(g_sound_goes_transmit, 0.50f);

    // GOESnet Acknowledge Chime: dual-harmonic pleasant chime (880 Hz + 1320 Hz)
    auto wave_ack = make_procedural_wave(5292, [](int /*i*/, float t) {
        float env   = std::exp(-t / 0.035f);
        float tone1 = std::sin(TWO_PI * 880.0f * t);
        float tone2 = 0.45f * std::sin(TWO_PI * 1320.0f * t);
        return (tone1 + tone2) * env * 0.45f;
    });
    g_sound_goes_ack = LoadSoundFromWave(wave_ack);
    UnloadWave(wave_ack);
    SetSoundVolume(g_sound_goes_ack, 0.45f);

    // GOESnet Error / Reject Buzzer: dual lower square/sine buzz (185 Hz + 245 Hz)
    auto wave_nack = make_procedural_wave(6174, [](int /*i*/, float t) {
        float env   = std::exp(-t / 0.045f);
        float tone1 = std::sin(TWO_PI * 185.0f * t);
        float tone2 = std::sin(TWO_PI * 245.0f * t);
        float buzz  = (tone1 + tone2 >= 0.0f ? 0.7f : -0.7f) * 0.5f + (tone1 + tone2) * 0.5f;
        return buzz * env * 0.50f;
    });
    g_sound_goes_nack = LoadSoundFromWave(wave_nack);
    UnloadWave(wave_nack);
    SetSoundVolume(g_sound_goes_nack, 0.45f);

    // Terminal Linefeed / Scroll Click: subtle 2800 Hz transient + 550 Hz body
    auto wave_scroll = make_procedural_wave(661, [](int /*i*/, float t) {
        float tick = std::exp(-t / 0.0018f) * std::sin(TWO_PI * 2800.0f * t);
        float tap  = std::exp(-t / 0.006f) * std::sin(TWO_PI * 550.0f * t) * 0.5f;
        return (tick + tap) * 0.35f;
    });
    g_sound_terminal_scroll = LoadSoundFromWave(wave_scroll);
    UnloadWave(wave_scroll);
    SetSoundVolume(g_sound_terminal_scroll, 0.30f);

    // Observation Deck Lifter Servo: hydraulic motor whine (180 -> 240 Hz with 40 Hz PWM)
    auto wave_lift = make_procedural_wave(15435, [](int /*i*/, float t) {
        constexpr float duration = 0.35f;
        float progress           = t / duration;
        float phase              = TWO_PI * (180.0f * t + 30.0f * t * progress);
        float mod                = 0.75f + 0.25f * std::sin(TWO_PI * 40.0f * t);
        float env                = 1.0f;
        if (t < 0.03f) {
            env = t / 0.03f;
        } else if (t > duration - 0.05f) {
            env = (duration - t) / 0.05f;
        }
        float tone = (std::sin(phase) + 0.3f * std::sin(phase * 2.0f)) * mod;
        return tone * env * 0.50f;
    });
    g_sound_deck_lift = LoadSoundFromWave(wave_lift);
    UnloadWave(wave_lift);
    SetSoundVolume(g_sound_deck_lift, 0.45f);

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
    if (IsSoundValid(g_sound_rcs_burst))
        UnloadSound(g_sound_rcs_burst);
    if (IsSoundValid(g_sound_touchdown))
        UnloadSound(g_sound_touchdown);
    if (IsSoundValid(g_sound_cockpit_button))
        UnloadSound(g_sound_cockpit_button);
    for (auto &snd : g_sound_terminal_keys) {
        if (IsSoundValid(snd))
            UnloadSound(snd);
    }
    if (IsSoundValid(g_sound_goes_transmit))
        UnloadSound(g_sound_goes_transmit);
    if (IsSoundValid(g_sound_goes_ack))
        UnloadSound(g_sound_goes_ack);
    if (IsSoundValid(g_sound_goes_nack))
        UnloadSound(g_sound_goes_nack);
    if (IsSoundValid(g_sound_terminal_scroll))
        UnloadSound(g_sound_terminal_scroll);
    if (IsSoundValid(g_sound_deck_lift))
        UnloadSound(g_sound_deck_lift);
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

    // Sublight RCS onset burst trigger
    if (telemetry.rcs_active && !g_last_rcs_active) {
        play_rcs_burst();
    }
    g_last_rcs_active = telemetry.rcs_active;

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

void play_rcs_burst() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_rcs_burst);
}

void play_touchdown_clunk() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_touchdown);
}

void play_cockpit_button() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_cockpit_button);
}

void play_terminal_keystroke() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_terminal_keys[g_terminal_key_index]);
    g_terminal_key_index = (g_terminal_key_index + 1) % 3;
}

void play_goesnet_transmit() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_goes_transmit);
}

void play_goesnet_chime(bool positive) {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(positive ? g_sound_goes_ack : g_sound_goes_nack);
}

void play_terminal_scroll() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_terminal_scroll);
}

void play_deck_lift() {
    if (!g_audio_ready || g_muted.load())
        return;
    PlaySound(g_sound_deck_lift);
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
