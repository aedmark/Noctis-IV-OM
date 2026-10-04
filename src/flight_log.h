#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

enum class FlightLogEventType : std::uint8_t {
    system_arrival,
    orbit_arrival,
    surface_landing,
    label_assigned,
};

[[nodiscard]] const char *flight_log_event_name(FlightLogEventType type);
[[nodiscard]] const char *star_class_name(std::int16_t star_class);
[[nodiscard]] const char *planet_type_name(std::int8_t planet_type);

struct FlightLogEntry {
    FlightLogEventType event_type = FlightLogEventType::system_arrival;
    std::string timestamp; // e.g. "Epoc 6012 Triad 14" or ISO date
    double star_x = 0.0;
    double star_y = 0.0;
    double star_z = 0.0;
    double star_id = 0.0;
    std::string star_name;
    std::int16_t star_class = 0;
    double jump_distance_ly = 0.0;

    std::int16_t planet_index = -1; // -1 if star or outside system
    std::string planet_name;
    std::int8_t planet_type = 0;

    double landing_lat = 0.0;
    double landing_lon = 0.0;
    std::string notes;
};

struct FlightLogStats {
    std::size_t total_jumps = 0;
    double total_distance_ly = 0.0;
    std::size_t unique_systems_visited = 0;
    std::size_t total_landings = 0;
    std::size_t total_labeled = 0;
};

class FlightLog {
public:
    FlightLog() = default;

    void record_system_arrival(double x, double y, double z, double star_id,
                               std::string_view star_name, std::int16_t star_class,
                               double jump_distance_ly, std::string_view timestamp = {});

    void record_orbit_arrival(double star_x, double star_y, double star_z,
                              std::string_view star_name, std::int16_t planet_index,
                              std::string_view planet_name, std::int8_t planet_type,
                              std::string_view timestamp = {});

    void record_surface_landing(double star_x, double star_y, double star_z,
                                std::string_view star_name, std::int16_t planet_index,
                                std::string_view planet_name, double lat, double lon,
                                std::string_view notes = {}, std::string_view timestamp = {});

    void record_label_assigned(double id, std::string_view name, bool is_planet,
                               std::int16_t ordinal, std::string_view timestamp = {});

    [[nodiscard]] const std::vector<FlightLogEntry> &entries() const { return entries_; }
    [[nodiscard]] FlightLogStats compute_stats() const;

    [[nodiscard]] std::vector<std::string> format_goes_summary(std::size_t max_entries = 5) const;

    bool load_from_file(const std::filesystem::path &path);
    bool save_to_file(const std::filesystem::path &path) const;
    bool export_markdown(const std::filesystem::path &path) const;
    bool export_json(const std::filesystem::path &path) const;

    void clear();

private:
    std::vector<FlightLogEntry> entries_;
    double last_star_x_ = 0.0;
    double last_star_y_ = 0.0;
    double last_star_z_ = 0.0;
    bool has_last_star_ = false;
};

FlightLog &active_flight_log();

} // namespace noctis
