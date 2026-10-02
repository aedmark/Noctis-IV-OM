#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace noctis {

inline constexpr std::uint32_t maximum_snapshot_number = 99'999'999U;
inline constexpr std::int16_t delete_snapshot_key = 127;

struct SnapshotSlot {
    std::uint32_t number{};
    std::filesystem::path path;
};

std::optional<SnapshotSlot> next_snapshot_slot(const std::filesystem::path &directory,
                                               std::uint32_t last_snapshot);

enum class SnapshotAction : std::uint8_t { none, normal, raw, panorama, raw_panorama };
SnapshotAction snapshot_action(std::int16_t key, bool surface, bool label_entry,
                               bool panorama_active);

std::int8_t cycle_mouse_look(std::int8_t mode);

struct MouseControlDelta {
    float shift{};
    float step{};
    float pitch{};
    float yaw{};
};

MouseControlDelta space_mouse_control(std::int8_t mode, bool right_button, std::int16_t dx,
                                      std::int16_t dy, float user_pitch);
MouseControlDelta surface_mouse_control(std::int8_t mode, bool right_button, bool left_button,
                                        std::int16_t dx, std::int16_t dy, bool landed,
                                        std::uint8_t movement_scale);

bool should_wait_for_frame(bool on_roof, bool roof_speed);

enum class DriveRecharge : std::uint8_t { none, lithium, omega, power_loss };
DriveRecharge recharge_drive(std::int16_t &power, std::int8_t &charge);
void restore_standard_drive(std::int16_t &power, std::int8_t &charge);

struct SurfaceVerticalState {
    float gravity{};
    bool jumping{};
    bool jetpack{};
};

void apply_surface_vertical_key(std::int16_t key, bool outside_capsule, float player_y,
                                float ground_y, SurfaceVerticalState &state);

} // namespace noctis
