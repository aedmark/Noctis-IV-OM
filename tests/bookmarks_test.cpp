#include "bookmarks.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

void test_basic_crud() {
    noctis::BookmarksManager manager;
    assert(manager.empty());
    assert(manager.count() == 0);

    noctis::Bookmark bm1;
    bm1.label = "SOLAR OUTPOST";
    bm1.star_id = 10001.0;
    bm1.star_name = "FELYSIA";
    bm1.star_class = 3;
    bm1.star_x = 100000.0;
    bm1.star_y = -200000.0;
    bm1.star_z = 300000.0;
    bm1.planet_index = 0;
    bm1.planet_name = "FELYSIA I";
    bm1.is_surface = true;
    bm1.surface_lat = 14.5;
    bm1.surface_lon = 120.0;
    bm1.timestamp = "EPOC 6012";

    const auto added1 = manager.add(bm1);
    assert(added1.id == 1);
    assert(manager.count() == 1);
    assert(!manager.empty());

    noctis::Bookmark bm2;
    bm2.label = "MINING MOON";
    bm2.star_id = 10002.0;
    bm2.star_name = "KHAIT";
    bm2.star_class = 5;
    bm2.star_x = 400000.0;
    bm2.star_y = -100000.0;
    bm2.star_z = 500000.0;
    bm2.planet_index = 2;
    bm2.planet_name = "KHAIT III";
    bm2.is_surface = false;

    const auto added2 = manager.add(bm2);
    assert(added2.id == 2);
    assert(manager.count() == 2);

    const auto retrieved1 = manager.get(1);
    assert(retrieved1.has_value());
    assert(retrieved1->label == "SOLAR OUTPOST");
    assert(retrieved1->star_name == "FELYSIA");
    assert(retrieved1->is_surface);
    assert(retrieved1->surface_lat == 14.5);

    const auto missing = manager.get(999);
    assert(!missing.has_value());

    assert(manager.remove(1));
    assert(manager.count() == 1);
    assert(!manager.get(1).has_value());
    assert(manager.get(2).has_value());
    assert(!manager.remove(999));

    manager.clear();
    assert(manager.empty());
    assert(manager.count() == 0);
    std::cout << "test_basic_crud: PASSED\n";
}

void test_ini_persistence() {
    const auto temp_dir = fs::temp_directory_path() / "noctis_bm_test";
    fs::create_directories(temp_dir);
    const auto ini_path = temp_dir / "bookmarks_test.ini";

    noctis::BookmarksManager manager;
    noctis::Bookmark bm1;
    bm1.label = "ALPHA CAMP";
    bm1.star_id = 5555.0;
    bm1.star_name = "DELPHI";
    bm1.star_class = 2;
    bm1.star_x = 12345.0;
    bm1.star_y = -67890.0;
    bm1.star_z = 24680.0;
    bm1.planet_index = 1;
    bm1.planet_name = "DELPHI II";
    bm1.is_surface = true;
    bm1.surface_lat = -22.5;
    bm1.surface_lon = 88.0;
    bm1.timestamp = "EPOC 7000";

    noctis::Bookmark bm2;
    bm2.label = "DEEP VOID";
    bm2.star_id = 7777.0;
    bm2.star_name = "VEGA PRIME";
    bm2.star_class = 4;
    bm2.star_x = -99999.0;
    bm2.star_y = 11111.0;
    bm2.star_z = 33333.0;
    bm2.planet_index = -1;
    bm2.is_surface = false;

    manager.add(bm1);
    manager.add(bm2);
    manager.set_active_waypoint_id(1);

    assert(manager.save_to_file(ini_path));
    assert(fs::exists(ini_path));

    noctis::BookmarksManager reloaded;
    assert(reloaded.load_from_file(ini_path));
    assert(reloaded.count() == 2);
    assert(reloaded.active_waypoint_id().has_value());
    assert(*reloaded.active_waypoint_id() == 1);

    const auto r1 = reloaded.get(1);
    assert(r1.has_value());
    assert(r1->label == "ALPHA CAMP");
    assert(r1->star_name == "DELPHI");
    assert(r1->planet_name == "DELPHI II");
    assert(r1->is_surface);
    assert(r1->surface_lat == -22.5);
    assert(r1->surface_lon == 88.0);

    const auto r2 = reloaded.get(2);
    assert(r2.has_value());
    assert(r2->label == "DEEP VOID");
    assert(r2->star_name == "VEGA PRIME");
    assert(r2->planet_index == -1);
    assert(!r2->is_surface);

    fs::remove_all(temp_dir);
    std::cout << "test_ini_persistence: PASSED\n";
}

void test_goes_formatting_and_constraints() {
    noctis::BookmarksManager manager;

    // 1. Empty list
    const auto empty_rows = manager.format_goes_list(1, 0.0, 0.0, 0.0);
    assert(!empty_rows.empty());
    for (const auto &r : empty_rows) {
        assert(r.size() <= 21);
    }

    // 2. Populated list with 5 bookmarks
    for (int i = 1; i <= 5; ++i) {
        noctis::Bookmark bm;
        bm.label = "BASE STATION " + std::to_string(i);
        bm.star_name = "STAR " + std::to_string(i);
        bm.star_x = i * 200000.0;
        bm.star_y = 0.0;
        bm.star_z = 0.0;
        if (i % 2 == 0) {
            bm.is_surface = true;
            bm.surface_lat = i * 10.0;
            bm.surface_lon = i * 40.0;
            bm.planet_name = "PLANET " + std::to_string(i);
            bm.planet_index = 0;
        }
        manager.add(bm);
    }

    manager.set_active_waypoint_id(2);

    // Page 1
    const auto page1 = manager.format_goes_list(1, 0.0, 0.0, 0.0, 3);
    for (const auto &r : page1) {
        assert(r.size() <= 21);
    }
    // Verify active waypoint has marker
    bool found_marker = false;
    for (const auto &r : page1) {
        if (r.find("*#2") != std::string::npos) {
            found_marker = true;
            break;
        }
    }
    assert(found_marker);

    // Page 2
    const auto page2 = manager.format_goes_list(2, 0.0, 0.0, 0.0, 3);
    for (const auto &r : page2) {
        assert(r.size() <= 21);
    }

    std::cout << "test_goes_formatting_and_constraints: PASSED\n";
}

int main() {
    test_basic_crud();
    test_ini_persistence();
    test_goes_formatting_and_constraints();
    std::cout << "All bookmarks tests PASSED!\n";
    return 0;
}
