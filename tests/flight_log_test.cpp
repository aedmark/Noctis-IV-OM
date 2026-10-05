#include "flight_log.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "Flight log test failure: %s\n", message);
    }
    return condition;
}

} // namespace

int main() {
    using namespace noctis;
    bool ok = true;

    FlightLog log;
    ok &= require(log.entries().empty(), "initial flight log should be empty");

    // Initial summary on empty log
    const auto empty_rows = log.format_goes_summary();
    ok &= require(!empty_rows.empty(), "empty summary produced rows");
    for (const auto &row : empty_rows) {
        ok &= require(row.size() <= 21, "empty summary row exceeds 21 columns");
    }

    // 1. System arrivals
    log.record_system_arrival(100000.0, 200000.0, 300000.0, 6.0, "MIRACLE", 0, 42.5, "Epoc 6012");
    ok &= require(log.entries().size() == 1, "first arrival recorded");
    ok &= require(log.entries().front().star_name == "MIRACLE", "star name recorded");
    ok &= require(log.entries().front().jump_distance_ly == 42.5, "jump distance recorded");

    // Deduplication check: consecutive arrival at same coords should be ignored
    log.record_system_arrival(100000.0, 200000.0, 300000.0, 6.0, "MIRACLE", 0, 42.5, "Epoc 6012");
    ok &= require(log.entries().size() == 1, "consecutive identical arrival was deduplicated");

    // 2. Orbital insertion
    log.record_orbit_arrival(100000.0, 200000.0, 300000.0, "MIRACLE", 0, "FELYSIA", 3, "Epoc 6012");
    ok &= require(log.entries().size() == 2, "orbit arrival recorded");
    ok &= require(log.entries().back().event_type == FlightLogEventType::orbit_arrival, "event type is orbit");
    ok &= require(log.entries().back().planet_name == "FELYSIA", "planet name recorded");

    // 3. Surface landing (Planetfall)
    log.record_surface_landing(100000.0, 200000.0, 300000.0, "MIRACLE", 0, "FELYSIA", 12.5, -45.0, "Lush plains", "Epoc 6012");
    ok &= require(log.entries().size() == 3, "landing recorded");
    ok &= require(log.entries().back().event_type == FlightLogEventType::surface_landing, "event type is landing");
    ok &= require(log.entries().back().landing_lat == 12.5 && log.entries().back().landing_lon == -45.0, "landing coordinates recorded");

    // 4. Second jump to unnamed system
    log.record_system_arrival(500000.0, -100000.0, 200000.0, -10.0, "", 3, 120.0, "Epoc 6013");
    ok &= require(log.entries().size() == 4, "unnamed arrival recorded");
    ok &= require(log.entries().back().star_name == "(UNNAMED)", "unnamed star displayed as (UNNAMED)");

    // 5. Label assigned to unnamed system
    log.record_label_assigned(-10.0, "AURORA", false, 3, "Epoc 6013");
    ok &= require(log.entries().size() == 5, "label assigned event recorded");
    // Verify retroactive update of earlier arrival
    ok &= require(log.entries()[3].star_name == "AURORA", "retroactive star name updated");
    ok &= require(log.entries().back().star_x == 500000.0, "star coordinates captured in label event");

    // 5b. Jump with UNKNOWN STAR / CLASS label followed by naming
    log.record_system_arrival(600000.0, 700000.0, 800000.0, 99.0, "UNKNOWN STAR / CLASS", 4, 30.0, "Epoc 6014");
    ok &= require(log.entries().back().star_name == "UNKNOWN STAR / CLASS", "unknown star label preserved");
    log.record_label_assigned(99.0, "SOLIS", false, 4, "Epoc 6014");
    ok &= require(log.entries()[5].star_name == "SOLIS", "retroactive update from UNKNOWN STAR / CLASS succeeded");

    // 6. Statistics
    const auto stats = log.compute_stats();
    ok &= require(stats.total_jumps == 3, "jump count is 3");
    ok &= require(std::abs(stats.total_distance_ly - 192.5) < 0.001, "total distance is 192.5");
    ok &= require(stats.unique_systems_visited == 3, "unique systems visited is 3");
    ok &= require(stats.total_landings == 1, "total landings is 1");
    ok &= require(stats.total_labeled == 2, "total labeled is 2");

    // 7. GOESnet screen row formatting
    const auto goes_rows = log.format_goes_summary();
    ok &= require(!goes_rows.empty(), "goes summary produced rows");
    for (std::size_t i = 0; i < goes_rows.size(); ++i) {
        ok &= require(goes_rows[i].size() <= 21, "goes summary row exceeds 21 columns");
    }

    // 8. Markdown export
    const auto test_dir = std::filesystem::temp_directory_path() / "flight_log_test_dir";
    std::filesystem::create_directories(test_dir);
    const auto md_path = test_dir / "flight_log.md";
    ok &= require(log.export_markdown(md_path), "export markdown succeeded");
    ok &= require(std::filesystem::exists(md_path), "markdown file created");

    std::ifstream md_file(md_path);
    std::string md_content((std::istreambuf_iterator<char>(md_file)), std::istreambuf_iterator<char>());
    ok &= require(md_content.find("Captain's Flight Log") != std::string::npos, "markdown contains title");
    ok &= require(md_content.find("Total Interstellar Jumps") != std::string::npos, "markdown contains summary");
    ok &= require(md_content.find("AURORA") != std::string::npos, "markdown contains renamed star");
    ok &= require(md_content.find("FELYSIA") != std::string::npos, "markdown contains planet");

    // 9. JSON export and round-trip load
    const auto json_path = test_dir / "flight_log.json";
    ok &= require(log.save_to_file(json_path), "save to json succeeded");
    ok &= require(std::filesystem::exists(json_path), "json file created");

    FlightLog loaded_log;
    ok &= require(loaded_log.load_from_file(json_path), "load from json succeeded");
    ok &= require(loaded_log.entries().size() == log.entries().size(), "loaded entry count matches");

    const auto loaded_stats = loaded_log.compute_stats();
    ok &= require(loaded_stats.total_jumps == stats.total_jumps, "loaded jumps match");
    ok &= require(std::abs(loaded_stats.total_distance_ly - stats.total_distance_ly) < 0.001, "loaded distance matches");
    ok &= require(loaded_stats.unique_systems_visited == stats.unique_systems_visited, "loaded unique systems match");
    ok &= require(loaded_stats.total_landings == stats.total_landings, "loaded landings match");
    ok &= require(loaded_stats.total_labeled == stats.total_labeled, "loaded labeled count matches");

    // Cleanup
    std::filesystem::remove_all(test_dir);

    return ok ? 0 : 1;
}
