#include "engine_state.h"
#include "legacy_save.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

bool terminal_adapter_preserves(const noctis::NativeSaveState &state) {
    noctis::GoesTerminalState terminal;
    noctis::restore_goes_terminal_state(state, terminal);
    auto adapted = state;
    adapted.gnc_pos = 0;
    adapted.goesfile_pos = 0;
    adapted.goesnet_command = {};
    noctis::capture_goes_terminal_state(terminal, adapted);
    return noctis::encode_native_save(adapted) == noctis::encode_native_save(state);
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: legacy_save_tests SEED_CURRENT_BIN\n";
        return 2;
    }
    const auto tracked = read_file(argv[1]);
    if (!require(tracked.size() == 378, "tracked NIV+ save is not 378 bytes")) return 1;

    struct SituationCase {
        std::size_t size;
        noctis::LegacySituationLayout layout;
    };
    const SituationCase cases[] = {
        {245, noctis::LegacySituationLayout::vanilla_245},
        {370, noctis::LegacySituationLayout::lr_370},
        {377, noctis::LegacySituationLayout::nivplus_377},
        {378, noctis::LegacySituationLayout::nivplus_378},
    };
    const auto upgrade_path = std::filesystem::current_path() / "legacy-upgrade-test.niv";
    std::error_code cleanup_error;
    for (const auto &test : cases) {
        noctis::LegacySituationImport imported;
        const std::span bytes(tracked.data(), test.size);
        if (!require(noctis::import_legacy_situation(bytes, imported).status == noctis::NativeSaveStatus::ok,
                     "source-backed legacy situation was rejected")
            || !require(imported.layout == test.layout, "legacy situation layout was misidentified")
            || !require(imported.state.nearstar_x == 0 && imported.state.goesnet_command[0] == '_',
                        "legacy situation fields were decoded incorrectly")) {
            return 1;
        }
        if (!require(terminal_adapter_preserves(imported.state),
                     "terminal state adapter changed normalized legacy bytes")) {
            return 1;
        }
        noctis::NativeSaveState upgraded;
        if (!require(noctis::save_native_save(upgrade_path, imported.state).status == noctis::NativeSaveStatus::ok,
                     "legacy situation did not migrate to native v1")
            || !require(noctis::load_native_save(upgrade_path, upgraded).status == noctis::NativeSaveStatus::ok,
                        "migrated native v1 situation did not reopen")
            || !require(noctis::encode_native_save(upgraded) == noctis::encode_native_save(imported.state),
                        "legacy-to-v1 upgrade changed state")) {
            return 1;
        }
        std::filesystem::remove(upgrade_path, cleanup_error);
    }

    auto extended = tracked;
    extended.insert(extended.end(), {1, 0, 1});
    const SituationCase extension_cases[] = {
        {379, noctis::LegacySituationLayout::nivplus_379},
        {380, noctis::LegacySituationLayout::nivplus_380},
        {381, noctis::LegacySituationLayout::nivplus_381},
    };
    for (const auto &test : extension_cases) {
        noctis::LegacySituationImport imported;
        if (!require(noctis::import_legacy_situation(std::span(extended.data(), test.size), imported).status
                         == noctis::NativeSaveStatus::ok,
                     "source-backed extended NIV+ layout was rejected")
            || !require(imported.layout == test.layout, "extended NIV+ layout was misidentified")
            || !require(terminal_adapter_preserves(imported.state),
                        "terminal state adapter changed extended legacy bytes")) {
            return 1;
        }
        noctis::NativeSaveState upgraded;
        if (!require(noctis::save_native_save(upgrade_path, imported.state).status == noctis::NativeSaveStatus::ok,
                     "extended NIV+ situation did not migrate to native v1")
            || !require(noctis::load_native_save(upgrade_path, upgraded).status == noctis::NativeSaveStatus::ok,
                        "upgraded extended NIV+ situation did not reopen")) {
            return 1;
        }
        std::filesystem::remove(upgrade_path, cleanup_error);
    }
    noctis::LegacySituationImport pinned_import;
    if (!require(noctis::import_legacy_situation(extended, pinned_import).status == noctis::NativeSaveStatus::ok,
                 "pinned 381-byte NIV+ layout was rejected")
        || !require(pinned_import.state.draw_hud == 1 && pinned_import.state.lens_flare_mode == 0
                        && pinned_import.state.seamless_border == 1,
                    "pinned NIV+ preferences were not preserved")) {
        return 1;
    }
    auto transitional = std::vector<std::uint8_t>(tracked.begin(), tracked.begin() + 377);
    transitional.insert(transitional.end(), {180, 0, 0, 0, 0});
    noctis::LegacySituationImport transitional_import;
    if (!require(noctis::import_legacy_situation(transitional, transitional_import).status
                     == noctis::NativeSaveStatus::ok,
                 "transitional 382-byte NIV+ layout was rejected")
        || !require(transitional_import.layout == noctis::LegacySituationLayout::nivplus_transitional_382,
                    "transitional NIV+ layout was misidentified")
        || !require(transitional_import.state.hud_closed == 0,
                    "transitional NIV+ HUD state was not normalized")
        || !require(terminal_adapter_preserves(transitional_import.state),
                    "terminal state adapter changed transitional legacy bytes")) {
        return 1;
    }
    noctis::NativeSaveState transitional_upgraded;
    if (!require(noctis::save_native_save(upgrade_path, transitional_import.state).status
                     == noctis::NativeSaveStatus::ok,
                 "transitional NIV+ situation did not migrate to native v1")
        || !require(noctis::load_native_save(upgrade_path, transitional_upgraded).status
                        == noctis::NativeSaveStatus::ok,
                    "upgraded transitional NIV+ situation did not reopen")
        || !require(noctis::encode_native_save(transitional_upgraded)
                        == noctis::encode_native_save(transitional_import.state),
                    "transitional upgrade changed normalized state")) {
        return 1;
    }
    std::filesystem::remove(upgrade_path, cleanup_error);

    for (const auto bad_size : {std::size_t{0}, std::size_t{244}, std::size_t{246}, std::size_t{369},
                                std::size_t{371}, std::size_t{375}, std::size_t{376}, std::size_t{383}}) {
        auto malformed = tracked;
        malformed.resize(bad_size);
        noctis::LegacySituationImport ignored;
        if (!require(noctis::import_legacy_situation(malformed, ignored).status
                         == noctis::NativeSaveStatus::invalid,
                     "unsupported legacy situation size was accepted")) {
            return 1;
        }
    }

    auto invalid_selector = tracked;
    invalid_selector[25] = 99;
    invalid_selector[26] = 0;
    noctis::LegacySituationImport ignored_situation;
    if (!require(noctis::import_legacy_situation(invalid_selector, ignored_situation).status
                     == noctis::NativeSaveStatus::invalid,
                 "out-of-range legacy selector was accepted")) {
        return 1;
    }

    noctis::SurfaceSaveState surface;
    surface.landing_longitude = 1;
    surface.landing_latitude = 60;
    surface.atl_x = 100;
    surface.atl_z = 101;
    surface.atl_x2 = 8192;
    surface.atl_z2 = 4096;
    surface.pos_x = 1640000;
    surface.pos_y = -200;
    surface.pos_z = 1630000;
    surface.user_alfa = 10;
    surface.user_beta = -20;
    surface.openhuddelta = -5;
    surface.openhudcount = 90;
    surface.hud_rtl_closed = 0;
    const auto native_surface = noctis::encode_surface_save(surface);
    for (const auto size : {std::size_t{40}, std::size_t{45}}) {
        noctis::LegacySurfaceImport imported;
        const std::span legacy(native_surface.data() + 20, size);
        if (!require(noctis::import_legacy_surface(legacy, imported).status == noctis::NativeSaveStatus::ok,
                     "source-backed legacy surface was rejected")
            || !require(imported.state.landing_longitude == 1 && imported.state.pos_x == 1640000,
                        "legacy surface fields were decoded incorrectly")) {
            return 1;
        }
        if (size == 40 && !require(imported.state.openhudcount == 180,
                                   "DOS surface import did not supply native HUD defaults")) {
            return 1;
        }
        const auto surface_upgrade_path = std::filesystem::current_path() / "legacy-surface-upgrade-test.niv";
        noctis::SurfaceSaveState upgraded;
        if (!require(noctis::save_surface_save(surface_upgrade_path, imported.state).status
                         == noctis::NativeSaveStatus::ok,
                     "legacy surface did not migrate to native v1")
            || !require(noctis::load_surface_save(surface_upgrade_path, upgraded).status
                            == noctis::NativeSaveStatus::ok,
                        "migrated native surface did not reopen")
            || !require(noctis::encode_surface_save(upgraded) == noctis::encode_surface_save(imported.state),
                        "legacy-to-v1 surface upgrade changed state")) {
            return 1;
        }
        std::filesystem::remove(surface_upgrade_path, cleanup_error);
    }
    std::vector<std::uint8_t> bad_surface(41);
    noctis::LegacySurfaceImport ignored_surface;
    if (!require(noctis::import_legacy_surface(bad_surface, ignored_surface).status
                     == noctis::NativeSaveStatus::invalid,
                 "unsupported legacy surface size was accepted")) {
        return 1;
    }

    const auto oversized_path = std::filesystem::current_path() / "oversized-legacy-save.bin";
    {
        std::ofstream output(oversized_path, std::ios::binary | std::ios::trunc);
        const std::vector<std::uint8_t> oversized(383);
        output.write(reinterpret_cast<const char *>(oversized.data()),
                     static_cast<std::streamsize>(oversized.size()));
    }
    noctis::LegacySituationImport oversized_import;
    const auto oversized_result = noctis::load_legacy_situation(oversized_path, oversized_import);
    std::error_code ignored;
    std::filesystem::remove(oversized_path, ignored);
    if (!require(oversized_result.status == noctis::NativeSaveStatus::invalid,
                 "oversized legacy situation file was not rejected before parsing")) {
        return 1;
    }

    const auto legacy_surface_path = std::filesystem::current_path() / "interrupted-surface.bin";
    const auto native_surface_path = std::filesystem::current_path() / "interrupted-surface.niv";
    {
        std::ofstream output(legacy_surface_path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char *>(native_surface.data() + 20), 40);
    }
    const auto legacy_before = read_file(legacy_surface_path);
    std::filesystem::create_directory(native_surface_path.string() + ".tmp");
    noctis::SurfaceRestore restore_guard;
    restore_guard.state.landing_longitude = 777;
    const auto interrupted = noctis::load_or_migrate_surface(native_surface_path, legacy_surface_path, restore_guard);
    const bool interrupted_ok = require(interrupted.status == noctis::NativeSaveStatus::io_error,
                                        "interrupted surface migration was not reported")
        && require(!std::filesystem::exists(native_surface_path),
                   "interrupted surface migration published a native save")
        && require(read_file(legacy_surface_path) == legacy_before,
                   "interrupted surface migration changed its legacy source")
        && require(restore_guard.state.landing_longitude == 777,
                   "interrupted surface migration partially changed its result")
        && require(std::filesystem::is_directory(native_surface_path.string() + ".tmp"),
                   "surface writer removed the blocking directory");
    std::filesystem::remove_all(native_surface_path.string() + ".tmp", cleanup_error);
    std::filesystem::remove(legacy_surface_path, cleanup_error);
    if (!interrupted_ok) return 1;
    return 0;
}
