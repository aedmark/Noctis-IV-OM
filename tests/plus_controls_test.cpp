#include "plus_controls.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::fprintf(stderr, "plus controls: %s\n", message);
    return condition;
}
}

int main() {
    bool ok = true;
    const auto root = std::filesystem::current_path() / "plus-controls-test";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directory(root);
    std::ofstream(root / "00000000.BMP").put('x');

    auto slot = noctis::next_snapshot_slot(root, UINT32_MAX);
    ok &= require(slot && slot->number == 1 && slot->path.filename() == "00000001.BMP",
                  "NICE sequence did not skip an occupied first image");
    slot = noctis::next_snapshot_slot(root, noctis::maximum_snapshot_number);
    ok &= require(slot && slot->number == 1, "snapshot sequence did not wrap at 99,999,999");
    using SA = noctis::SnapshotAction;
    ok &= require(noctis::snapshot_action('m', false, false, false) == SA::normal
                      && noctis::snapshot_action('m', false, true, false) == SA::none
                      && noctis::snapshot_action('b', false, false, false) == SA::raw
                      && noctis::snapshot_action(noctis::delete_snapshot_key, false, false, false) == SA::raw
                      && noctis::snapshot_action(noctis::delete_snapshot_key, false, true, false) == SA::none
                      && noctis::snapshot_action('n', true, false, false) == SA::panorama
                      && noctis::snapshot_action('.', true, false, false) == SA::raw_panorama
                      && noctis::snapshot_action('n', false, false, false) == SA::none
                      && noctis::snapshot_action('m', true, false, true) == SA::none,
                  "snapshot key routing changed");

    ok &= require(noctis::cycle_mouse_look(0) == 1 && noctis::cycle_mouse_look(1) == 2
                      && noctis::cycle_mouse_look(2) == 0,
                  "three-mode mouselook cycle changed");
    const auto space_move = noctis::space_mouse_control(0, false, 6, 4, 12);
    const auto space_look = noctis::space_mouse_control(2, false, 6, 4, 12);
    ok &= require(space_move.step == -12 && space_move.yaw == -2 && space_move.pitch == 0,
                  "space movement-mode mouse mapping changed");
    ok &= require(space_look.step == 0 && space_look.yaw == -2 && space_look.pitch == -0.5F,
                  "space inverted-look mouse mapping changed");
    const auto surface_move = noctis::surface_mouse_control(0, false, true, 6, 4, true, 5);
    const auto surface_look = noctis::surface_mouse_control(1, false, true, 6, 4, true, 5);
    ok &= require(surface_move.step == 55 && surface_move.yaw == -1 && surface_move.pitch == 0,
                  "surface movement-mode mouse mapping changed");
    ok &= require(surface_look.step == 75 && surface_look.yaw == -1 && surface_look.pitch == 0.5F,
                  "surface look-mode mouse mapping changed");

    ok &= require(noctis::should_wait_for_frame(false, true)
                      && noctis::should_wait_for_frame(true, false)
                      && !noctis::should_wait_for_frame(true, true),
                  "roof-speed pacing gate changed");
    ok &= require(noctis::is_roof_speed_key('t')
                      && noctis::is_roof_speed_key('T')
                      && noctis::is_roof_speed_key('S')
                      && !noctis::is_roof_speed_key('s')
                      && !noctis::is_roof_speed_key('w')
                      && !noctis::is_roof_speed_key('a')
                      && !noctis::is_roof_speed_key('d'),
                  "roof speed key toggle mapping changed");

    std::int16_t power = 15000;
    std::int8_t charge = 2;
    ok &= require(noctis::recharge_drive(power, charge) == noctis::DriveRecharge::lithium
                      && power == 20000 && charge == 1,
                  "normal lithium recharge changed");
    power = 14999;
    charge = -1;
    ok &= require(noctis::recharge_drive(power, charge) == noctis::DriveRecharge::omega
                      && power == 20000 && charge == -1,
                  "Omega Drive consumed its enablement flag");
    power = 15000;
    charge = 0;
    noctis::restore_standard_drive(power, charge);
    ok &= require(power == 20000 && charge == 120,
                  "standard-drive recovery did not refill depleted power and lithium");

    noctis::SurfaceVerticalState vertical{25, false, false};
    noctis::apply_surface_vertical_key('j', true, 100, 100, vertical);
    ok &= require(vertical.gravity == -500 && vertical.jumping && !vertical.jetpack,
                  "surface jump impulse changed");
    vertical = {-100, true, false};
    noctis::apply_surface_vertical_key(' ', true, -200, 100, vertical);
    ok &= require(vertical.gravity == -150 && vertical.jumping && vertical.jetpack,
                  "jetpack thrust changed");
    noctis::apply_surface_vertical_key('c', true, -200, 100, vertical);
    ok &= require(!vertical.jetpack, "jetpack directional cutoff changed");
    vertical = {12, false, false};
    noctis::apply_surface_vertical_key(' ', false, 100, 100, vertical);
    ok &= require(vertical.gravity == 12 && !vertical.jetpack,
                  "jetpack activated inside the capsule");

    std::filesystem::remove_all(root, ignored);
    return ok ? 0 : 1;
}
