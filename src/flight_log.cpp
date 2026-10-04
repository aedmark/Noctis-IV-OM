#include "flight_log.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace noctis {

namespace {

constexpr std::string_view divider = "&&&&&&&&&&&&&&&&&&&&&";

std::string_view trim_view(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.remove_suffix(1);
    return s;
}

std::string trim(std::string_view s) {
    return std::string(trim_view(s));
}

std::string unquote(std::string_view s) {
    s = trim_view(s);
    if (!s.empty() && s.back() == ',') s.remove_suffix(1);
    s = trim_view(s);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s.remove_prefix(1);
        s.remove_suffix(1);
    }
    return std::string(s);
}

std::string escape_json(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string format_coord(double val) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.0f", val);
    return std::string(buf);
}

std::string format_dist(double ly) {
    char buf[32];
    if (ly >= 10000.0) {
        std::snprintf(buf, sizeof(buf), "%.0f LY", ly);
    } else if (ly >= 10.0) {
        std::snprintf(buf, sizeof(buf), "%.1f LY", ly);
    } else {
        std::snprintf(buf, sizeof(buf), "%.2f LY", ly);
    }
    return std::string(buf);
}

} // namespace

const char *flight_log_event_name(FlightLogEventType type) {
    switch (type) {
    case FlightLogEventType::system_arrival: return "System Arrival";
    case FlightLogEventType::orbit_arrival: return "Orbital Insertion";
    case FlightLogEventType::surface_landing: return "Planetfall";
    case FlightLogEventType::label_assigned: return "Object Labeled";
    }
    return "Unknown Event";
}

const char *star_class_name(std::int16_t star_class) {
    switch (star_class) {
    case 0: return "Yellow Star (Sol-type)";
    case 1: return "Blue Giant";
    case 2: return "White Dwarf";
    case 3: return "Red Giant";
    case 4: return "Orange Giant";
    case 5: return "Brown Dwarf";
    case 6: return "Gray Giant (Dead)";
    case 7: return "Blue Dwarf";
    case 8: return "Multiple System";
    case 9: return "Young Star (Nebular)";
    case 10: return "Runaway Star";
    case 11: return "Pulsar / Neutron";
    default: return "Unclassified Star";
    }
}

const char *planet_type_name(std::int8_t planet_type) {
    switch (planet_type) {
    case 0: return "Volcanic";
    case 1: return "Airless Cratered";
    case 2: return "Dense / Clouded";
    case 3: return "Felisian (Habitable)";
    case 4: return "Mountainous Rocky";
    case 5: return "Thin Atmosphere";
    case 6: return "Jovian Gas Giant";
    case 7: return "Icy Frozen";
    case 8: return "Quartz Silicate";
    case 9: return "Substellar Object";
    case 10: return "Companion Star";
    default: return "Unclassified Body";
    }
}

void FlightLog::record_system_arrival(double x, double y, double z, double star_id,
                                      std::string_view star_name, std::int16_t star_class,
                                      double jump_distance_ly, std::string_view timestamp) {
    if (!entries_.empty() && entries_.back().event_type == FlightLogEventType::system_arrival
        && std::abs(entries_.back().star_x - x) < 0.5
        && std::abs(entries_.back().star_y - y) < 0.5
        && std::abs(entries_.back().star_z - z) < 0.5) {
        // Skip identical consecutive arrival
        return;
    }

    FlightLogEntry entry;
    entry.event_type = FlightLogEventType::system_arrival;
    entry.timestamp = timestamp.empty() ? "Flight Epoc" : std::string(timestamp);
    entry.star_x = x;
    entry.star_y = y;
    entry.star_z = z;
    entry.star_id = star_id;
    entry.star_name = star_name.empty() ? "(UNNAMED)" : std::string(star_name);
    entry.star_class = star_class;
    entry.jump_distance_ly = jump_distance_ly;

    entries_.push_back(std::move(entry));
    last_star_x_ = x;
    last_star_y_ = y;
    last_star_z_ = z;
    has_last_star_ = true;
}

void FlightLog::record_orbit_arrival(double star_x, double star_y, double star_z,
                                     std::string_view star_name, std::int16_t planet_index,
                                     std::string_view planet_name, std::int8_t planet_type,
                                     std::string_view timestamp) {
    if (!entries_.empty() && entries_.back().event_type == FlightLogEventType::orbit_arrival
        && entries_.back().planet_index == planet_index
        && std::abs(entries_.back().star_x - star_x) < 0.5
        && std::abs(entries_.back().star_z - star_z) < 0.5) {
        return;
    }

    FlightLogEntry entry;
    entry.event_type = FlightLogEventType::orbit_arrival;
    entry.timestamp = timestamp.empty() ? "Flight Epoc" : std::string(timestamp);
    entry.star_x = star_x;
    entry.star_y = star_y;
    entry.star_z = star_z;
    entry.star_name = star_name.empty() ? "(UNNAMED)" : std::string(star_name);
    entry.planet_index = planet_index;
    entry.planet_name = planet_name.empty() ? ("Planet #" + std::to_string(planet_index + 1)) : std::string(planet_name);
    entry.planet_type = planet_type;

    entries_.push_back(std::move(entry));
}

void FlightLog::record_surface_landing(double star_x, double star_y, double star_z,
                                       std::string_view star_name, std::int16_t planet_index,
                                       std::string_view planet_name, double lat, double lon,
                                       std::string_view notes, std::string_view timestamp) {
    FlightLogEntry entry;
    entry.event_type = FlightLogEventType::surface_landing;
    entry.timestamp = timestamp.empty() ? "Flight Epoc" : std::string(timestamp);
    entry.star_x = star_x;
    entry.star_y = star_y;
    entry.star_z = star_z;
    entry.star_name = star_name.empty() ? "(UNNAMED)" : std::string(star_name);
    entry.planet_index = planet_index;
    entry.planet_name = planet_name.empty() ? ("Planet #" + std::to_string(planet_index + 1)) : std::string(planet_name);
    entry.landing_lat = lat;
    entry.landing_lon = lon;
    entry.notes = std::string(notes);

    entries_.push_back(std::move(entry));
}

void FlightLog::record_label_assigned(double id, std::string_view name, bool is_planet,
                                      std::int16_t ordinal, std::string_view timestamp) {
    FlightLogEntry entry;
    entry.event_type = FlightLogEventType::label_assigned;
    entry.timestamp = timestamp.empty() ? "Flight Epoc" : std::string(timestamp);
    entry.star_id = id;
    if (is_planet) {
        entry.planet_name = std::string(name);
        entry.planet_index = static_cast<std::int16_t>(ordinal - 1);
    } else {
        entry.star_name = std::string(name);
    }

    // Retroactively update earlier entries that had (UNNAMED)
    for (auto &e : entries_) {
        if (!is_planet && (e.star_name.empty() || e.star_name == "(UNNAMED)")
            && std::abs(e.star_id - id) < 0.0001) {
            e.star_name = std::string(name);
        } else if (is_planet && e.planet_index == ordinal - 1
                   && (e.planet_name.empty() || e.planet_name.find("Planet #") != std::string::npos)) {
            e.planet_name = std::string(name);
        }
    }

    entries_.push_back(std::move(entry));
}

FlightLogStats FlightLog::compute_stats() const {
    FlightLogStats stats;
    std::set<std::tuple<long long, long long, long long>> visited_coords;

    for (const auto &e : entries_) {
        if (e.event_type == FlightLogEventType::system_arrival) {
            stats.total_jumps++;
            stats.total_distance_ly += e.jump_distance_ly;
            visited_coords.insert({static_cast<long long>(std::round(e.star_x)),
                                   static_cast<long long>(std::round(e.star_y)),
                                   static_cast<long long>(std::round(e.star_z))});
        } else if (e.event_type == FlightLogEventType::surface_landing) {
            stats.total_landings++;
        } else if (e.event_type == FlightLogEventType::label_assigned) {
            stats.total_labeled++;
        }
    }
    stats.unique_systems_visited = visited_coords.size();
    return stats;
}

std::vector<std::string> FlightLog::format_goes_summary(std::size_t max_entries) const {
    const auto stats = compute_stats();
    std::vector<std::string> rows;
    rows.reserve(16);

    auto push_row = [&](std::string s) {
        if (s.size() > 21) s = s.substr(0, 21);
        rows.push_back(std::move(s));
    };

    push_row("CAPTAIN'S FLIGHT LOG");
    push_row(std::string(divider));

    char stat1[32];
    std::snprintf(stat1, sizeof(stat1), "JUMPS: %zu LANDS: %zu", stats.total_jumps, stats.total_landings);
    push_row(stat1);

    char stat2[32];
    std::snprintf(stat2, sizeof(stat2), "DIST: %s", format_dist(stats.total_distance_ly).c_str());
    push_row(stat2);

    char stat3[32];
    std::snprintf(stat3, sizeof(stat3), "STARS VISITED: %zu", stats.unique_systems_visited);
    push_row(stat3);

    push_row(std::string(divider));

    if (entries_.empty()) {
        push_row("NO LOGGED ENTRIES.");
        push_row("EXPLORE THE GALAXY TO");
        push_row("RECORD FLIGHT DATA.");
        return rows;
    }

    push_row("RECENT LOG ENTRIES:");
    const std::size_t count = std::min(max_entries, entries_.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto &e = entries_[entries_.size() - 1 - i];
        if (e.event_type == FlightLogEventType::system_arrival) {
            push_row("* STAR: " + e.star_name);
            char dist_buf[32];
            std::snprintf(dist_buf, sizeof(dist_buf), "  %s %s", star_class_name(e.star_class), format_dist(e.jump_distance_ly).c_str());
            push_row(dist_buf);
        } else if (e.event_type == FlightLogEventType::surface_landing) {
            push_row("* LAND: " + e.planet_name);
            char coord_buf[32];
            std::snprintf(coord_buf, sizeof(coord_buf), "  LAT %.0f LON %.0f", e.landing_lat, e.landing_lon);
            push_row(coord_buf);
        } else if (e.event_type == FlightLogEventType::orbit_arrival) {
            push_row("* ORBIT: " + e.planet_name);
        } else if (e.event_type == FlightLogEventType::label_assigned) {
            std::string name = e.star_name.empty() ? e.planet_name : e.star_name;
            push_row("* LABELED: " + name);
        }
    }

    return rows;
}

bool FlightLog::export_markdown(const std::filesystem::path &path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;

    const auto stats = compute_stats();

    out << "# Noctis IV OM — Captain's Flight Log\n\n";
    out << "## Exploration Summary\n\n";
    out << "- **Total Interstellar Jumps:** " << stats.total_jumps << "\n";
    out << "- **Total Distance Traveled:** " << format_dist(stats.total_distance_ly) << "\n";
    out << "- **Unique Star Systems Visited:** " << stats.unique_systems_visited << "\n";
    out << "- **Planetary Landings (Planetfalls):** " << stats.total_landings << "\n";
    out << "- **Celestial Objects Labeled:** " << stats.total_labeled << "\n\n";

    out << "## Journal Entries\n\n";
    if (entries_.empty()) {
        out << "_No flight log entries recorded yet._\n";
        return true;
    }

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const auto &e = entries_[i];
        out << "### " << (i + 1) << ". " << flight_log_event_name(e.event_type) << "\n\n";
        out << "- **Timestamp:** " << e.timestamp << "\n";

        if (e.event_type == FlightLogEventType::system_arrival) {
            out << "- **Star System:** " << e.star_name << "\n";
            out << "- **Coordinates:** (" << format_coord(e.star_x) << ", "
                << format_coord(e.star_y) << ", " << format_coord(e.star_z) << ")\n";
            out << "- **Spectral Class:** " << star_class_name(e.star_class) << "\n";
            out << "- **Jump Distance:** " << format_dist(e.jump_distance_ly) << "\n";
        } else if (e.event_type == FlightLogEventType::orbit_arrival) {
            out << "- **System:** " << e.star_name << "\n";
            out << "- **Target Body:** " << e.planet_name << " (Index #" << (e.planet_index + 1) << ")\n";
            out << "- **Body Classification:** " << planet_type_name(e.planet_type) << "\n";
        } else if (e.event_type == FlightLogEventType::surface_landing) {
            out << "- **Landing Location:** " << e.planet_name << " in system " << e.star_name << "\n";
            out << "- **Coordinates:** Lat: " << std::fixed << std::setprecision(1) << e.landing_lat
                << "°, Lon: " << e.landing_lon << "°\n";
            if (!e.notes.empty()) {
                out << "- **Notes:** " << e.notes << "\n";
            }
        } else if (e.event_type == FlightLogEventType::label_assigned) {
            std::string name = e.star_name.empty() ? e.planet_name : e.star_name;
            out << "- **Assigned Name:** " << name << "\n";
        }
        out << "\n";
    }

    return true;
}

bool FlightLog::export_json(const std::filesystem::path &path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;

    const auto stats = compute_stats();

    out << "{\n";
    out << "  \"summary\": {\n";
    out << "    \"total_jumps\": " << stats.total_jumps << ",\n";
    out << "    \"total_distance_ly\": " << stats.total_distance_ly << ",\n";
    out << "    \"unique_systems_visited\": " << stats.unique_systems_visited << ",\n";
    out << "    \"total_landings\": " << stats.total_landings << ",\n";
    out << "    \"total_labeled\": " << stats.total_labeled << "\n";
    out << "  },\n";
    out << "  \"entries\": [\n";

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const auto &e = entries_[i];
        out << "    {\n";
        out << "      \"event_type\": \"" << flight_log_event_name(e.event_type) << "\",\n";
        out << "      \"timestamp\": \"" << escape_json(e.timestamp) << "\",\n";
        out << "      \"star_x\": " << e.star_x << ",\n";
        out << "      \"star_y\": " << e.star_y << ",\n";
        out << "      \"star_z\": " << e.star_z << ",\n";
        out << "      \"star_name\": \"" << escape_json(e.star_name) << "\",\n";
        out << "      \"star_class\": " << e.star_class << ",\n";
        out << "      \"jump_distance_ly\": " << e.jump_distance_ly << ",\n";
        out << "      \"planet_index\": " << e.planet_index << ",\n";
        out << "      \"planet_name\": \"" << escape_json(e.planet_name) << "\",\n";
        out << "      \"planet_type\": " << static_cast<int>(e.planet_type) << ",\n";
        out << "      \"landing_lat\": " << e.landing_lat << ",\n";
        out << "      \"landing_lon\": " << e.landing_lon << ",\n";
        out << "      \"notes\": \"" << escape_json(e.notes) << "\"\n";
        out << "    }" << (i + 1 < entries_.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";
    return true;
}

bool FlightLog::save_to_file(const std::filesystem::path &path) const {
    return export_json(path);
}

bool FlightLog::load_from_file(const std::filesystem::path &path) {
    std::ifstream in(path);
    if (!in) return false;

    std::string line;
    FlightLogEntry current;
    bool in_entry = false;
    std::vector<FlightLogEntry> loaded;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line.find("\"event_type\":") != std::string::npos) {
            in_entry = true;
            current = FlightLogEntry{};
            auto colon = line.find(':');
            std::string val = unquote(line.substr(colon + 1));
            if (val == "System Arrival") current.event_type = FlightLogEventType::system_arrival;
            else if (val == "Orbital Insertion") current.event_type = FlightLogEventType::orbit_arrival;
            else if (val == "Planetfall") current.event_type = FlightLogEventType::surface_landing;
            else if (val == "Object Labeled") current.event_type = FlightLogEventType::label_assigned;
        } else if (in_entry && line.find("\"timestamp\":") != std::string::npos) {
            auto colon = line.find(':');
            current.timestamp = unquote(line.substr(colon + 1));
        } else if (in_entry && line.find("\"star_x\":") != std::string::npos) {
            auto colon = line.find(':');
            current.star_x = std::strtod(line.substr(colon + 1).c_str(), nullptr);
        } else if (in_entry && line.find("\"star_y\":") != std::string::npos) {
            auto colon = line.find(':');
            current.star_y = std::strtod(line.substr(colon + 1).c_str(), nullptr);
        } else if (in_entry && line.find("\"star_z\":") != std::string::npos) {
            auto colon = line.find(':');
            current.star_z = std::strtod(line.substr(colon + 1).c_str(), nullptr);
        } else if (in_entry && line.find("\"star_name\":") != std::string::npos) {
            auto colon = line.find(':');
            current.star_name = unquote(line.substr(colon + 1));
        } else if (in_entry && line.find("\"star_class\":") != std::string::npos) {
            auto colon = line.find(':');
            current.star_class = static_cast<std::int16_t>(std::strtol(line.substr(colon + 1).c_str(), nullptr, 10));
        } else if (in_entry && line.find("\"jump_distance_ly\":") != std::string::npos) {
            auto colon = line.find(':');
            current.jump_distance_ly = std::strtod(line.substr(colon + 1).c_str(), nullptr);
        } else if (in_entry && line.find("\"planet_index\":") != std::string::npos) {
            auto colon = line.find(':');
            current.planet_index = static_cast<std::int16_t>(std::strtol(line.substr(colon + 1).c_str(), nullptr, 10));
        } else if (in_entry && line.find("\"planet_name\":") != std::string::npos) {
            auto colon = line.find(':');
            current.planet_name = unquote(line.substr(colon + 1));
        } else if (in_entry && line.find("\"planet_type\":") != std::string::npos) {
            auto colon = line.find(':');
            current.planet_type = static_cast<std::int8_t>(std::strtol(line.substr(colon + 1).c_str(), nullptr, 10));
        } else if (in_entry && line.find("\"landing_lat\":") != std::string::npos) {
            auto colon = line.find(':');
            current.landing_lat = std::strtod(line.substr(colon + 1).c_str(), nullptr);
        } else if (in_entry && line.find("\"landing_lon\":") != std::string::npos) {
            auto colon = line.find(':');
            current.landing_lon = std::strtod(line.substr(colon + 1).c_str(), nullptr);
        } else if (in_entry && line.find("\"notes\":") != std::string::npos) {
            auto colon = line.find(':');
            current.notes = unquote(line.substr(colon + 1));
        } else if (in_entry && (line == "}" || line == "}," || line == "]" || line == "],")) {
            loaded.push_back(std::move(current));
            in_entry = false;
        }
    }

    entries_ = std::move(loaded);
    if (!entries_.empty()) {
        for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
            if (it->event_type == FlightLogEventType::system_arrival) {
                last_star_x_ = it->star_x;
                last_star_y_ = it->star_y;
                last_star_z_ = it->star_z;
                has_last_star_ = true;
                break;
            }
        }
    }
    return true;
}

void FlightLog::clear() {
    entries_.clear();
    last_star_x_ = 0.0;
    last_star_y_ = 0.0;
    last_star_z_ = 0.0;
    has_last_star_ = false;
}

FlightLog &active_flight_log() {
    static FlightLog instance;
    return instance;
}

} // namespace noctis
