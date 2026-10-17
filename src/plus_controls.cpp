#include "plus_controls.h"

#include <cstdio>

namespace noctis {

std::optional<SnapshotSlot> next_snapshot_slot(const std::filesystem::path &directory,
                                               std::uint32_t last_snapshot) {
    std::uint32_t candidate = last_snapshot >= maximum_snapshot_number ? 0U : last_snapshot + 1U;
    const auto first = candidate;
    do {
        char filename[13];
        std::snprintf(filename, sizeof(filename), "%08u.BMP", candidate);
        std::error_code error;
        const auto path = directory / filename;
        const bool occupied = std::filesystem::exists(path, error);
        if (error) return std::nullopt;
        if (!occupied) return SnapshotSlot{candidate, path};
        candidate = candidate == maximum_snapshot_number ? 0U : candidate + 1U;
    } while (candidate != first);
    return std::nullopt;
}

SnapshotAction snapshot_action(std::int16_t key, bool surface, bool label_entry,
                               bool panorama_active) {
    if (panorama_active) return SnapshotAction::none;
    if (key == '*' || (key == 'm' && !label_entry)) return SnapshotAction::normal;
    if ((key == 'b' || key == delete_snapshot_key) && !label_entry) return SnapshotAction::raw;
    if (surface && (key == '/' || key == 'n')) return SnapshotAction::panorama;
    if (surface && (key == 'v' || key == '.')) return SnapshotAction::raw_panorama;
    return SnapshotAction::none;
}

MouseLookCycle cycle_mouse_look(std::int8_t mode, bool pitch_inverted) {
    if (mode == 0) return {1, false};
    if (!pitch_inverted) return {1, true};
    return {0, false};
}

MouseControlDelta space_mouse_control(std::int8_t mode, bool right_button, std::int16_t dx,
                                      std::int16_t dy, float user_pitch, bool pitch_inverted) {
    MouseControlDelta result;
    // Undo the adapter's global Y inversion for walking only, mirroring the
    // surface handler: forward means forward at any pitch setting.
    const float dy_raw = pitch_inverted ? static_cast<float>(dy) : -static_cast<float>(dy);
    if (right_button) {
        result.shift = 3.0F * dx;
        if (mode == 0) {
            result.pitch = -static_cast<float>(dy) / 8.0F;
        } else {
            result.step = -3.0F * dy_raw;
            if (dy > 7 || dy < -7) result.pitch = -user_pitch / 6.0F;
        }
    } else {
        // Look direction follows the label: with Mouse Pitch Normal, a forward
        // push looks up. (Mode 2 folded into mode 1 + pitch inversion.)
        if (mode == 1) result.pitch = -static_cast<float>(dy) / 8.0F;
        else {
            result.step = -3.0F * dy_raw;
            if (dy > 7 || dy < -7) result.pitch = -user_pitch / 6.0F;
        }
        result.yaw = -static_cast<float>(dx) / 3.0F;
    }
    return result;
}

MouseControlDelta surface_mouse_control(std::int8_t mode, bool right_button, bool left_button,
                                        std::int16_t dx, std::int16_t dy, bool landed,
                                        std::uint8_t movement_scale,
                                        bool pitch_inverted) {
    MouseControlDelta result;
    const bool mouselook = right_button != (mode > 0);
    if (right_button) result.shift = dx;
    else result.yaw = -static_cast<float>(dx) / 6.0F;
    if (mouselook) {
        // Single camera convention: the pitch flag flows through the input
        // adapter; mouselook itself only gates look-vs-walk.
        result.pitch = -static_cast<float>(dy) / 8.0F;
    }
    if (landed) {
        if (left_button) result.step += 15.0F * movement_scale;
        if (!mouselook) {
            // Undo the adapter's global Y inversion so walking follows the raw
            // DOS axis (mouse up = negative raw = forward), decoupled from the
            // camera pitch inversion flag.
            const float dy_raw = pitch_inverted ? static_cast<float>(dy) : -static_cast<float>(dy);
            result.step -= movement_scale * dy_raw;
        }
    }
    return result;
}

bool should_wait_for_frame(bool on_roof, bool roof_speed) { return !on_roof || !roof_speed; }
bool is_roof_speed_key(std::int16_t key) { return key == 't' || key == 'T' || key == 'S'; }

DriveRecharge recharge_drive(std::int16_t &power, std::int8_t &charge) {
    if (power > 15000) return DriveRecharge::none;
    if (charge > 0) {
        --charge;
        power = 20000;
        return DriveRecharge::lithium;
    }
    if (charge < 0) {
        power = 20000;
        return DriveRecharge::omega;
    }
    power = 15000;
    return DriveRecharge::power_loss;
}

void restore_standard_drive(std::int16_t &power, std::int8_t &charge) {
    power = 20000;
    charge = 120;
}

void apply_surface_vertical_key(std::int16_t key, bool outside_capsule, float player_y,
                                float ground_y, SurfaceVerticalState &state) {
    if (key == 'c') {
        state.jetpack = false;
        return;
    }
    if (key == 'j' && player_y > ground_y - 10.0F) {
        state.gravity = -500.0F;
        state.jumping = true;
        return;
    }
    if (key != ' ' || !outside_capsule) return;
    if (player_y > ground_y - 150.0F) {
        state.gravity = -500.0F;
        state.jumping = true;
    } else {
        state.gravity -= 50.0F;
        state.jumping = true;
        state.jetpack = true;
    }
}

} // namespace noctis
