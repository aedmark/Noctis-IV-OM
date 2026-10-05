#include "navigation_hud.h"

#include <cassert>
#include <cmath>
#include <iostream>

void test_heading_calculations() {
    // Noctis convention: ccom = 360 - user_beta
    // user_beta = 0 -> 0 deg -> N
    const auto h0 = noctis::compute_heading(0.0f);
    assert(std::abs(h0.degrees - 0.0) < 0.01);
    assert(h0.cardinal == "N");

    // user_beta = 315 -> 45 deg -> NE
    const auto h45 = noctis::compute_heading(315.0f);
    assert(std::abs(h45.degrees - 45.0) < 0.01);
    assert(h45.cardinal == "NE");

    // user_beta = 270 -> 90 deg -> E
    const auto h90 = noctis::compute_heading(270.0f);
    assert(std::abs(h90.degrees - 90.0) < 0.01);
    assert(h90.cardinal == "E");

    // user_beta = 225 -> 135 deg -> SE
    const auto h135 = noctis::compute_heading(225.0f);
    assert(std::abs(h135.degrees - 135.0) < 0.01);
    assert(h135.cardinal == "SE");

    // user_beta = 180 -> 180 deg -> S
    const auto h180 = noctis::compute_heading(180.0f);
    assert(std::abs(h180.degrees - 180.0) < 0.01);
    assert(h180.cardinal == "S");

    // user_beta = 135 -> 225 deg -> SW
    const auto h225 = noctis::compute_heading(135.0f);
    assert(std::abs(h225.degrees - 225.0) < 0.01);
    assert(h225.cardinal == "SW");

    // user_beta = 90 -> 270 deg -> W
    const auto h270 = noctis::compute_heading(90.0f);
    assert(std::abs(h270.degrees - 270.0) < 0.01);
    assert(h270.cardinal == "W");

    // user_beta = 45 -> 315 deg -> NW
    const auto h315 = noctis::compute_heading(45.0f);
    assert(std::abs(h315.degrees - 315.0) < 0.01);
    assert(h315.cardinal == "NW");

    // user_beta = 360 -> 0 deg -> N
    const auto h360 = noctis::compute_heading(360.0f);
    assert(std::abs(h360.degrees - 0.0) < 0.01);
    assert(h360.cardinal == "N");

    std::cout << "test_heading_calculations: PASSED\n";
}

void test_surface_coordinates() {
    // Equator (landing_pt_lat = 60), Prime meridian (landing_pt_lon = 0)
    const auto eq = noctis::compute_surface_coordinates(0, 60, 1000.0, 50.0, 2000.0, 0.0, 1000.0, 2000.0);
    assert(std::abs(eq.latitude_deg - 0.0) < 0.01);
    assert(std::abs(eq.longitude_deg - 0.0) < 0.01);
    assert(std::abs(eq.altitude_agl_m - 2.5) < 0.01); // 50 / 20 = 2.5m
    assert(std::abs(eq.elevation_msl_m - 2.5) < 0.01);

    // North pole (landing_pt_lat = 120 -> +90 deg)
    const auto np = noctis::compute_surface_coordinates(180, 120, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    assert(std::abs(np.latitude_deg - 90.0) < 0.01);
    assert(std::abs(np.longitude_deg - 180.0) < 0.01);

    // South pole (landing_pt_lat = 0 -> -90 deg)
    const auto sp = noctis::compute_surface_coordinates(270, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    assert(std::abs(sp.latitude_deg - (-90.0)) < 0.01);
    assert(std::abs(sp.longitude_deg - 270.0) < 0.01);

    std::cout << "test_surface_coordinates: PASSED\n";
}

void test_lander_beacon() {
    // 1. Docked (within 1600 units)
    const auto docked = noctis::compute_lander_beacon(100.0, 100.0, 0.0f, 150.0, 150.0);
    assert(docked.is_docked);

    // 2. Straight ahead (facing North, lander to the North)
    // user_beta = 0 -> ccom = 0 (facing North, +Z)
    // player at (0, 0), lander at (0, 5000)
    const auto ahead = noctis::compute_lander_beacon(0.0, 0.0, 0.0f, 0.0, 5000.0);
    assert(!ahead.is_docked);
    assert(std::abs(ahead.distance_m - 250.0) < 0.1); // 5000 / 20 = 250m
    assert(std::abs(ahead.relative_bearing_deg - 0.0) < 0.1);
    assert(ahead.direction_arrow == "[^]");

    // 3. Directly behind
    const auto behind = noctis::compute_lander_beacon(0.0, 0.0, 0.0f, 0.0, -5000.0);
    assert(!behind.is_docked);
    assert(std::abs(std::abs(behind.relative_bearing_deg) - 180.0) < 0.1);
    assert(behind.direction_arrow == "[v]");

    // 4. Directly to the right (East, +X)
    const auto right = noctis::compute_lander_beacon(0.0, 0.0, 0.0f, 5000.0, 0.0);
    assert(!right.is_docked);
    assert(std::abs(right.relative_bearing_deg - 90.0) < 0.1);
    assert(right.direction_arrow == "[>]");

    // 5. Directly to the left (West, -X)
    const auto left = noctis::compute_lander_beacon(0.0, 0.0, 0.0f, -5000.0, 0.0);
    assert(!left.is_docked);
    assert(std::abs(left.relative_bearing_deg - (-90.0)) < 0.1);
    assert(left.direction_arrow == "[<]");

    // 6. Turning player 90 deg right (user_beta = 270 -> ccom = 90 East)
    // lander at (5000, 0) is now straight ahead!
    const auto ahead_turned = noctis::compute_lander_beacon(0.0, 0.0, 270.0f, 5000.0, 0.0);
    assert(std::abs(ahead_turned.relative_bearing_deg - 0.0) < 0.1);
    assert(ahead_turned.direction_arrow == "[^]");

    std::cout << "test_lander_beacon: PASSED\n";
}

void test_visor_mode_cycling() {
    auto mode = noctis::VisorHudMode::standard;
    mode = noctis::cycle_visor_hud_mode(mode);
    assert(mode == noctis::VisorHudMode::explorer_telemetry);
    mode = noctis::cycle_visor_hud_mode(mode);
    assert(mode == noctis::VisorHudMode::minimal);
    mode = noctis::cycle_visor_hud_mode(mode);
    assert(mode == noctis::VisorHudMode::standard);

    assert(noctis::visor_hud_mode_name(noctis::VisorHudMode::explorer_telemetry) != nullptr);
    assert(noctis::visor_hud_mode_name(noctis::VisorHudMode::standard) != nullptr);
    assert(noctis::visor_hud_mode_name(noctis::VisorHudMode::minimal) != nullptr);

    noctis::set_visor_hud_mode(noctis::VisorHudMode::explorer_telemetry);
    assert(noctis::get_visor_hud_mode() == noctis::VisorHudMode::explorer_telemetry);

    std::cout << "test_visor_mode_cycling: PASSED\n";
}

int main() {
    test_heading_calculations();
    test_surface_coordinates();
    test_lander_beacon();
    test_visor_mode_cycling();
    std::cout << "All navigation HUD tests PASSED!\n";
    return 0;
}
