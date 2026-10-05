#include "atmospheric_scattering.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

void test_modes_and_cycling() {
    using namespace noctis;

    set_atmospheric_scattering_mode(AtmosphericScatteringMode::authentic);
    assert(get_atmospheric_scattering_mode() == AtmosphericScatteringMode::authentic);

    assert(cycle_atmospheric_scattering_mode(AtmosphericScatteringMode::authentic) == AtmosphericScatteringMode::realistic);
    assert(cycle_atmospheric_scattering_mode(AtmosphericScatteringMode::realistic) == AtmosphericScatteringMode::vibrant);
    assert(cycle_atmospheric_scattering_mode(AtmosphericScatteringMode::vibrant) == AtmosphericScatteringMode::authentic);

    auto m = cycle_atmospheric_scattering_mode();
    assert(m == AtmosphericScatteringMode::realistic);
    assert(get_atmospheric_scattering_mode() == AtmosphericScatteringMode::realistic);

    m = cycle_atmospheric_scattering_mode();
    assert(m == AtmosphericScatteringMode::vibrant);
    assert(get_atmospheric_scattering_mode() == AtmosphericScatteringMode::vibrant);

    m = cycle_atmospheric_scattering_mode();
    assert(m == AtmosphericScatteringMode::authentic);
    assert(get_atmospheric_scattering_mode() == AtmosphericScatteringMode::authentic);

    assert(std::string(atmospheric_scattering_mode_name(AtmosphericScatteringMode::authentic)).find("AUTHENTIC") != std::string::npos);
    assert(std::string(atmospheric_scattering_mode_name(AtmosphericScatteringMode::realistic)).find("REALISTIC") != std::string::npos);
    assert(std::string(atmospheric_scattering_mode_name(AtmosphericScatteringMode::vibrant)).find("VIBRANT") != std::string::npos);

    assert(parse_atmospheric_scattering_mode("authentic") == AtmosphericScatteringMode::authentic);
    assert(parse_atmospheric_scattering_mode("legacy") == AtmosphericScatteringMode::authentic);
    assert(parse_atmospheric_scattering_mode("off") == AtmosphericScatteringMode::authentic);
    assert(parse_atmospheric_scattering_mode("realistic") == AtmosphericScatteringMode::realistic);
    assert(parse_atmospheric_scattering_mode("on") == AtmosphericScatteringMode::realistic);
    assert(parse_atmospheric_scattering_mode("vibrant") == AtmosphericScatteringMode::vibrant);
    assert(parse_atmospheric_scattering_mode("enhanced") == AtmosphericScatteringMode::vibrant);
    assert(!parse_atmospheric_scattering_mode("unknown").has_value());
}

void test_twilight_data_day_to_night() {
    using namespace noctis;

    // 1. Midday: Sun high in the sky (crepzone = 60, nightzone = 0)
    auto day = compute_twilight_data(60, 0, 1, -100.0f, -50.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);
    assert(day.solar_elevation_deg > 40.0f);
    assert(day.solar_zenith_deg < 50.0f);
    assert(day.twilight_factor == 0.0f);
    assert(day.day_factor == 1.0f);
    assert(day.night_factor == 0.0f);
    assert(day.sunset_warmth == 0.0f);
    assert(day.horizon_glow_peak == 0.0f);
    assert(!day.is_twilight);

    // 2. Golden hour: Sun approaching horizon (crepzone = 10, nightzone = 0)
    auto golden = compute_twilight_data(10, 0, 1, -100.0f, -10.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);
    assert(golden.solar_elevation_deg > 5.0f && golden.solar_elevation_deg < 10.0f);
    assert(golden.twilight_factor > 0.2f);
    assert(golden.sunset_warmth > 0.4f);
    assert(golden.day_factor > 0.6f);
    assert(golden.night_factor == 0.0f);
    assert(golden.is_twilight);

    // 3. Sunset: Sun at the horizon (crepzone = 0, nightzone = 0)
    auto sunset = compute_twilight_data(0, 0, -1, 100.0f, 0.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);
    assert(std::fabs(sunset.solar_elevation_deg) < 1e-4f);
    assert(sunset.twilight_factor > 0.8f);
    assert(sunset.sunset_warmth >= 0.99f);
    assert(sunset.horizon_glow_peak > 0.8f);
    assert(sunset.is_twilight);

    // 4. Civil twilight: Sun just below horizon (crepzone = 4, nightzone = 1)
    auto civil = compute_twilight_data(4, 1, -1, 100.0f, 5.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);
    assert(civil.solar_elevation_deg < 0.0f && civil.solar_elevation_deg > -5.0f);
    assert(civil.twilight_factor > 0.7f);
    assert(civil.day_factor < 0.1f);
    assert(civil.night_factor > 0.1f && civil.night_factor < 0.5f);
    assert(civil.is_twilight);

    // 5. Nautical twilight (crepzone = 9, nightzone = 1)
    auto nautical = compute_twilight_data(9, 1, -1, 100.0f, 10.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);
    assert(nautical.solar_elevation_deg <= -6.0f && nautical.solar_elevation_deg > -12.0f);
    assert(nautical.night_factor >= 0.4f);
    assert(nautical.is_twilight);

    // 6. Deep night (crepzone = 40, nightzone = 1)
    auto night = compute_twilight_data(40, 1, -1, 100.0f, 40.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);
    assert(night.solar_elevation_deg < -25.0f);
    assert(night.twilight_factor == 0.0f);
    assert(night.day_factor == 0.0f);
    assert(night.night_factor == 1.0f);
    assert(!night.is_twilight);
}

void test_sun_limb_visibility() {
    using namespace noctis;

    // In daytime, sun is always visible
    assert(is_sun_limb_visible_at_twilight(10, 0));
    assert(is_sun_limb_visible_at_twilight(0, 0));

    // In nightzone: sun limb visible if crepzone <= 3
    assert(is_sun_limb_visible_at_twilight(0, 1));
    assert(is_sun_limb_visible_at_twilight(1, 1));
    assert(is_sun_limb_visible_at_twilight(2, 1));
    assert(is_sun_limb_visible_at_twilight(3, 1));
    assert(!is_sun_limb_visible_at_twilight(4, 1));
    assert(!is_sun_limb_visible_at_twilight(20, 1));
}

void test_sky_map_generation() {
    using namespace noctis;

    constexpr uint16_t kTotalBytes = 64800; // 360 x 180
    constexpr uint16_t kLinesToHorizon = 120;
    std::vector<uint8_t> bg(kTotalBytes, 40);

    auto sunset = compute_twilight_data(0, 0, -1, 100.0f, 0.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);

    generate_twilight_sky_map(
        bg.data(), kTotalBytes, kLinesToHorizon, 40,
        sunset, AtmosphericScatteringMode::realistic, 0.0f);

    // Verify values remain in valid 6-bit palette range [0, 63]
    for (size_t i = 0; i < kLinesToHorizon * 360; ++i) {
        assert(bg[i] <= 63);
    }

    // Near the horizon (line 115) at the solar azimuth, brightness should be higher
    // than at the zenith (line 5) due to forward Mie scattering and atmospheric path
    uint16_t sun_col = static_cast<uint16_t>(std::round(sunset.sun_azimuth_deg)) % 360;
    uint8_t horizon_sun_val = bg[115 * 360 + sun_col];
    uint8_t zenith_sun_val  = bg[5 * 360 + sun_col];
    assert(horizon_sun_val > zenith_sun_val);

    // Anti-solar azimuth (sun_col + 180) % 360
    uint16_t anti_col = (sun_col + 180) % 360;
    uint8_t horizon_anti_val = bg[115 * 360 + anti_col];
    // Forward glow at the sun's azimuth should be brighter than the anti-solar horizon
    assert(horizon_sun_val > horizon_anti_val);
}

void test_palette_filters() {
    using namespace noctis;

    float fr_g = 10.0f, fg_g = 12.0f, fb_g = 8.0f;
    float fr_s = 25.0f, fg_s = 35.0f, fb_s = 50.0f;
    float fr_h = 20.0f, fg_h = 25.0f, fb_h = 35.0f;
    float fr_v = 15.0f, fg_v = 40.0f, fb_v = 10.0f;

    auto sunset = compute_twilight_data(0, 0, -1, 100.0f, 0.0f, 0.0f, 200.0f, 1.0f, 2, 0.0f);

    apply_twilight_palette_filters(
        fr_g, fg_g, fb_g,
        fr_s, fg_s, fb_s,
        fr_h, fg_h, fb_h,
        fr_v, fg_v, fb_v,
        3, // habitable planet type
        sunset,
        AtmosphericScatteringMode::realistic,
        0.0f);

    // Horizon should be warm-tinted (red > green > blue)
    assert(fr_h > fg_h);
    assert(fg_h > fb_h);
}

int main() {
    test_modes_and_cycling();
    test_twilight_data_day_to_night();
    test_sun_limb_visibility();
    test_sky_map_generation();
    test_palette_filters();
    std::cout << "All atmospheric scattering tests passed successfully." << std::endl;
    return 0;
}
