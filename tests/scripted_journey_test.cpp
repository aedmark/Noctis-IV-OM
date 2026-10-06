#include "input.h"
#include "input_recording.h"
#include "system_properties.h"
#include "travel.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

std::array<std::uint8_t, sc_bytes> framebuffer{};
noctis::InputReplay *active_replay = nullptr;
std::uint64_t active_tick          = 0;

noctis::InputFrame replayed_input() { return active_replay->frame_for_tick(active_tick); }

} // namespace

std::uint8_t *adapted = framebuffer.data();

#include "tdpolygs.h"

namespace {

constexpr noctis::TravelPosition launch_position{3797120.0, -4352112.0, -925018.0};
constexpr noctis::TravelPosition parent_star{-18928.0, -29680.0, -67336.0};
constexpr double star_radius         = 5.021;
constexpr std::int16_t felysia_index = 3;
constexpr std::uint64_t fnv_offset   = UINT64_C(14695981039346656037);
constexpr std::uint64_t fnv_prime    = UINT64_C(1099511628211);

enum class JourneyStage : std::uint8_t {
    launch,
    remote_selected,
    remote_flight,
    remote_arrived,
    local_selected,
    local_flight,
    local_arrived,
};

struct JourneyState {
    JourneyStage stage          = JourneyStage::launch;
    noctis::TravelPosition ship = launch_position;
    noctis::TravelPosition local_target{};
    noctis::TravelGuidance guidance{};
    PlanetSystemProperties system{};
    int power           = 20000;
    int charge          = 120;
    int simulation_tick = 0;
    int remote_steps    = 0;
    int local_steps     = 0;
    bool flight_screen  = false;
};

struct Checkpoint {
    JourneyStage stage{};
    int tick                   = 0;
    std::uint64_t state_hash   = 0;
    std::uint64_t frame_hash   = 0;
    std::size_t nonzero_pixels = 0;

    bool operator==(const Checkpoint &) const = default;
};

struct JourneyResult {
    JourneyState final_state;
    std::array<Checkpoint, 4> checkpoints{};
    bool replay_complete     = false;
    bool replay_missed_input = false;
};

void hash_bytes(std::uint64_t &hash, const void *data, std::size_t size) {
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= fnv_prime;
    }
}

template <typename Value> void hash_value(std::uint64_t &hash, Value value) { hash_bytes(hash, &value, sizeof(value)); }

std::uint64_t state_hash(const JourneyState &state) {
    std::uint64_t hash = fnv_offset;
    hash_value(hash, state.stage);
    hash_value(hash, state.simulation_tick);
    hash_value(hash, state.remote_steps);
    hash_value(hash, state.local_steps);
    hash_value(hash, state.power);
    hash_value(hash, state.charge);
    hash_value(hash, std::bit_cast<std::uint64_t>(state.ship.x));
    hash_value(hash, std::bit_cast<std::uint64_t>(state.ship.y));
    hash_value(hash, std::bit_cast<std::uint64_t>(state.ship.z));
    return hash;
}

std::uint64_t framebuffer_hash() {
    std::uint64_t hash = fnv_offset;
    hash_bytes(hash, framebuffer.data(), adapted_width * adapted_height);
    return hash;
}

void reset_renderer() {
    framebuffer.fill(0);
    cam_x = cam_y = cam_z = 0;
    alfa = beta = ngamma = 0;
    dpp                  = 200;
    flares               = 0;
    entity               = 1;
    culling_needed       = 0;
    halfscan_needed      = 0;
    H_MATRIXS = V_MATRIXS = 16;
    change_camera_lens();
    change_txm_repeating_mode();
}

void render_stage(JourneyStage stage) {
    reset_renderer();
    const auto stage_number = static_cast<std::uint8_t>(stage);
    for (std::size_t index = 0; index < 48; ++index) {
        const std::size_t x                = (index * 67 + stage_number * 19) % adapted_width;
        const std::size_t y                = (index * 43 + stage_number * 11) % adapted_height;
        framebuffer[y * adapted_width + x] = static_cast<std::uint8_t>(65 + index % 31);
    }

    const float extent = 12.0F + 15.0F * stage_number;
    std::array<float, 4> x{-extent, extent, extent, -extent};
    std::array<float, 4> y{-extent, -extent, extent, extent};
    std::array<float, 4> z{500.0F, 500.0F, 500.0F, 500.0F};
    if (stage == JourneyStage::local_arrived) {
        static std::array<std::uint8_t, TEXTURE_X_SIZE * TEXTURE_Y_SIZE> texture{};
        for (std::size_t row = 0; row < TEXTURE_Y_SIZE; ++row) {
            for (std::size_t column = 0; column < TEXTURE_X_SIZE; ++column) {
                texture[row * TEXTURE_X_SIZE + column] = static_cast<std::uint8_t>(160 + ((row / 8 + column / 8) & 15));
            }
        }
        txtr = texture.data();
        polymap(x.data(), y.data(), z.data(), 4, 0);
    } else {
        poly3d(x.data(), y.data(), z.data(), 4, static_cast<std::uint8_t>(72 + stage_number * 8));
    }
}

Checkpoint capture_checkpoint(const JourneyState &state) {
    render_stage(state.stage);
    return {state.stage, state.simulation_tick, state_hash(state), framebuffer_hash(),
            static_cast<std::size_t>(std::count_if(framebuffer.begin(),
                                                   framebuffer.begin() + adapted_width * adapted_height,
                                                   [](std::uint8_t pixel) { return pixel != 0; }))};
}

noctis::TravelPosition felysia_position(const PlanetBodyProperties &body) {
    const double orbital_z = body.orbit_radius * body.orbit_eccentricity;
    return {parent_star.x + orbital_z * std::sin(body.orbit_orientation), parent_star.y,
            parent_star.z + orbital_z * std::cos(body.orbit_orientation)};
}

void consume_power(JourneyState &state, std::int16_t cost) {
    state.power -= cost;
    if (state.power <= 15000 && state.charge > 0) {
        --state.charge;
        state.power = 20000;
    }
}

noctis::InputRecording make_journey_recording() {
    noctis::InputRecording recording;
    const auto append_key = [&recording](std::uint64_t tick, char key) {
        noctis::InputFrame frame;
        frame.text = {key};
        return noctis::append_input_frame(recording, tick, frame);
    };
    if (!append_key(0, 'r') || !append_key(1, '6') || !append_key(2, '7') || !append_key(399, '8') ||
        !append_key(400, '8')) {
        return {};
    }
    return recording;
}

void process_scripted_command(JourneyState &state) {
    if (!is_key()) {
        return;
    }
    const auto key = get_key();
    if (key == 'r' && state.stage == JourneyStage::launch) {
        state.flight_screen = true;
        return;
    }
    if (!state.flight_screen) {
        return;
    }
    if (key == '6' && state.stage == JourneyStage::launch) {
        state.guidance = noctis::begin_travel(state.ship, parent_star);
        state.stage    = JourneyStage::remote_selected;
    } else if (key == '7' && state.stage == JourneyStage::remote_selected) {
        state.stage = JourneyStage::remote_flight;
    } else if (key == '8' && state.stage == JourneyStage::remote_arrived) {
        state.system       = derive_planet_system(parent_star.x, parent_star.y, parent_star.z, 0, star_radius);
        state.local_target = felysia_position(state.system.bodies[felysia_index]);
        state.guidance     = noctis::begin_travel(state.ship, state.local_target);
        state.stage        = JourneyStage::local_selected;
    } else if (key == '8' && state.stage == JourneyStage::local_selected) {
        state.stage = JourneyStage::local_flight;
    }
}

JourneyResult run_journey(const noctis::InputRecording &recording, int presentation_interval) {
    JourneyResult result;
    auto &state                  = result.final_state;
    std::size_t checkpoint_index = 0;
    noctis::InputReplay replay(recording);
    active_replay = &replay;
    noctis::reset_input_state();
    noctis::set_input_provider(replayed_input);

    while (state.stage != JourneyStage::local_arrived && state.simulation_tick < 2000) {
        active_tick = static_cast<std::uint64_t>(state.simulation_tick);
        handle_input();
        process_scripted_command(state);

        if ((state.stage == JourneyStage::remote_selected || state.stage == JourneyStage::local_selected) &&
            checkpoint_index < result.checkpoints.size() &&
            (checkpoint_index == 0 || result.checkpoints[checkpoint_index - 1].stage != state.stage)) {
            result.checkpoints[checkpoint_index++] = capture_checkpoint(state);
        }

        if (state.stage == JourneyStage::remote_flight) {
            const auto step = noctis::advance_remote_travel(
                state.ship, parent_star, noctis::remote_arrival_radius(false, true, star_radius), state.guidance);
            consume_power(state, step.power_cost);
            ++state.remote_steps;
            if (step.arrived) {
                state.stage                            = JourneyStage::remote_arrived;
                result.checkpoints[checkpoint_index++] = capture_checkpoint(state);
            }
        } else if (state.stage == JourneyStage::local_flight) {
            const auto &felysia = state.system.bodies[felysia_index];
            const auto step =
                noctis::advance_local_travel(state.ship, state.local_target, felysia.radius, state.guidance);
            consume_power(state, step.power_cost);
            ++state.local_steps;
            if (step.arrived) {
                state.stage                            = JourneyStage::local_arrived;
                result.checkpoints[checkpoint_index++] = capture_checkpoint(state);
            }
        }

        if (presentation_interval > 0 && state.simulation_tick % presentation_interval == 0) {
            render_stage(state.stage);
        }
        ++state.simulation_tick;
    }

    result.replay_complete     = replay.complete();
    result.replay_missed_input = replay.missed_input();
    noctis::reset_input_provider();
    noctis::reset_input_state();
    active_replay = nullptr;
    return result;
}

bool same_result(const JourneyResult &left, const JourneyResult &right) {
    return state_hash(left.final_state) == state_hash(right.final_state) &&
           left.final_state.local_target.x == right.final_state.local_target.x &&
           left.final_state.local_target.y == right.final_state.local_target.y &&
           left.final_state.local_target.z == right.final_state.local_target.z && left.checkpoints == right.checkpoints;
}

void print_result(const JourneyResult &result) {
    std::fprintf(stderr, "journey: stage=%u tick=%d remote=%d local=%d power=%d charge=%d\n",
                 static_cast<unsigned>(result.final_state.stage), result.final_state.simulation_tick,
                 result.final_state.remote_steps, result.final_state.local_steps, result.final_state.power,
                 result.final_state.charge);
    for (const auto &checkpoint : result.checkpoints) {
        std::fprintf(stderr, "  checkpoint stage=%u tick=%d state=%016llx frame=%016llx pixels=%zu\n",
                     static_cast<unsigned>(checkpoint.stage), checkpoint.tick,
                     static_cast<unsigned long long>(checkpoint.state_hash),
                     static_cast<unsigned long long>(checkpoint.frame_hash), checkpoint.nonzero_pixels);
    }
}

} // namespace

int main() {
    const auto source_recording = make_journey_recording();
    const auto encoded          = noctis::encode_input_recording(source_recording);
    noctis::InputRecording recording;
    const auto decoded = noctis::decode_input_recording(encoded, recording);
    if (!decoded || noctis::encode_input_recording(recording) != encoded) {
        return 1;
    }
    const auto every_frame        = run_journey(recording, 1);
    const auto every_fourth_frame = run_journey(recording, 4);
    constexpr std::array<Checkpoint, 4> expected_checkpoints{{
        {JourneyStage::remote_selected, 1, UINT64_C(0x13c978f4805771f8), UINT64_C(0x1737cdd80c6f921d), 576},
        {JourneyStage::remote_arrived, 398, UINT64_C(0xcdf5411db5d9f60d), UINT64_C(0x5b8269d030b8f9b3), 2253},
        {JourneyStage::local_selected, 399, UINT64_C(0xa14749834f71048d), UINT64_C(0x1b1bc9d7e0399397), 3523},
        {JourneyStage::local_arrived, 792, UINT64_C(0x0502685c30c189ac), UINT64_C(0x30629965a585364d), 6844},
    }};
    bool ok = same_result(every_frame, every_fourth_frame);
    ok &= every_frame.replay_complete && !every_frame.replay_missed_input;
    ok &= every_fourth_frame.replay_complete && !every_fourth_frame.replay_missed_input;
    ok &= every_frame.checkpoints == expected_checkpoints;
    ok &= every_frame.final_state.stage == JourneyStage::local_arrived;
    ok &= every_frame.final_state.remote_steps == 397;
    ok &= every_frame.final_state.local_steps == 393;
    ok &= every_frame.final_state.power == 19788;
    ok &= every_frame.final_state.charge == 117;
    ok &= noctis::travel_distance(every_frame.final_state.ship, every_frame.final_state.local_target) <
          2.0 * every_frame.final_state.system.bodies[felysia_index].radius;

    if (!ok) {
        print_result(every_frame);
        print_result(every_fourth_frame);
        return 1;
    }
    print_result(every_frame);
    return 0;
}
