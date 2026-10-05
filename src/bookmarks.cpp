#include "bookmarks.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace noctis {
namespace {

std::string trim_spaces(std::string_view sv) {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r')) {
        sv.remove_suffix(1);
    }
    return std::string(sv);
}

} // namespace

std::optional<Bookmark> BookmarksManager::get(std::size_t id) const {
    const auto *found = find(id);
    if (found != nullptr) {
        return *found;
    }
    return std::nullopt;
}

const Bookmark *BookmarksManager::find(std::size_t id) const {
    const auto it = std::find_if(bookmarks_.begin(), bookmarks_.end(),
                                 [id](const Bookmark &b) { return b.id == id; });
    return it != bookmarks_.end() ? &(*it) : nullptr;
}

Bookmark BookmarksManager::add(Bookmark bm) {
    if (bm.id == 0 || find(bm.id) != nullptr) {
        bm.id = next_id_++;
    } else {
        if (bm.id >= next_id_) {
            next_id_ = bm.id + 1;
        }
    }
    bookmarks_.push_back(bm);
    return bm;
}

bool BookmarksManager::remove(std::size_t id) {
    const auto it = std::find_if(bookmarks_.begin(), bookmarks_.end(),
                                 [id](const Bookmark &b) { return b.id == id; });
    if (it == bookmarks_.end()) {
        return false;
    }
    if (active_waypoint_id_ && *active_waypoint_id_ == id) {
        active_waypoint_id_ = std::nullopt;
    }
    bookmarks_.erase(it);
    return true;
}

void BookmarksManager::clear() {
    bookmarks_.clear();
    active_waypoint_id_ = std::nullopt;
    next_id_ = 1;
}

bool BookmarksManager::load_from_file(const std::filesystem::path &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    clear();
    std::string line;
    std::string current_section;
    Bookmark current_bm;
    bool in_bm_section = false;

    auto finish_current_bm = [&]() {
        if (in_bm_section && current_bm.id > 0) {
            bookmarks_.push_back(current_bm);
            if (current_bm.id >= next_id_) {
                next_id_ = current_bm.id + 1;
            }
        }
        current_bm = Bookmark{};
        in_bm_section = false;
    };

    while (std::getline(file, line)) {
        std::string trimmed = trim_spaces(line);
        if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            finish_current_bm();
            current_section = trimmed.substr(1, trimmed.size() - 2);
            if (current_section.rfind("bookmark_", 0) == 0) {
                in_bm_section = true;
            }
            continue;
        }

        const auto eq = trimmed.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = trim_spaces(trimmed.substr(0, eq));
        std::string val = trim_spaces(trimmed.substr(eq + 1));

        if (current_section == "bookmarks") {
            if (key == "active_waypoint") {
                try {
                    const auto id = static_cast<std::size_t>(std::stoul(val));
                    if (id > 0) active_waypoint_id_ = id;
                } catch (...) {}
            }
        } else if (in_bm_section) {
            try {
                if (key == "id") current_bm.id = static_cast<std::size_t>(std::stoul(val));
                else if (key == "label") current_bm.label = val;
                else if (key == "star_id") current_bm.star_id = std::stod(val);
                else if (key == "star_name") current_bm.star_name = val;
                else if (key == "star_class") current_bm.star_class = static_cast<std::int16_t>(std::stoi(val));
                else if (key == "star_x") current_bm.star_x = std::stod(val);
                else if (key == "star_y") current_bm.star_y = std::stod(val);
                else if (key == "star_z") current_bm.star_z = std::stod(val);
                else if (key == "planet_index") current_bm.planet_index = static_cast<std::int16_t>(std::stoi(val));
                else if (key == "planet_name") current_bm.planet_name = val;
                else if (key == "is_surface") current_bm.is_surface = (val == "1" || val == "true");
                else if (key == "surface_lat") current_bm.surface_lat = std::stod(val);
                else if (key == "surface_lon") current_bm.surface_lon = std::stod(val);
                else if (key == "timestamp") current_bm.timestamp = val;
            } catch (...) {}
        }
    }
    finish_current_bm();

    return true;
}

bool BookmarksManager::save_to_file(const std::filesystem::path &path) const {
    if (const auto parent = path.parent_path(); !parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << "[bookmarks]\n";
    file << "count=" << bookmarks_.size() << "\n";
    if (active_waypoint_id_) {
        file << "active_waypoint=" << *active_waypoint_id_ << "\n";
    }
    file << "\n";

    for (const auto &bm : bookmarks_) {
        file << "[bookmark_" << bm.id << "]\n";
        file << "id=" << bm.id << "\n";
        file << "label=" << bm.label << "\n";
        file << "star_id=" << bm.star_id << "\n";
        file << "star_name=" << bm.star_name << "\n";
        file << "star_class=" << bm.star_class << "\n";
        file << "star_x=" << bm.star_x << "\n";
        file << "star_y=" << bm.star_y << "\n";
        file << "star_z=" << bm.star_z << "\n";
        file << "planet_index=" << bm.planet_index << "\n";
        file << "planet_name=" << bm.planet_name << "\n";
        file << "is_surface=" << (bm.is_surface ? "1" : "0") << "\n";
        file << "surface_lat=" << bm.surface_lat << "\n";
        file << "surface_lon=" << bm.surface_lon << "\n";
        file << "timestamp=" << bm.timestamp << "\n";
        file << "\n";
    }

    return true;
}

std::vector<std::string> BookmarksManager::format_goes_list(
    std::size_t page,
    double cur_x, double cur_y, double cur_z,
    std::size_t items_per_page) const {
    std::vector<std::string> rows;
    rows.reserve(16);

    auto push_row = [&](std::string s) {
        if (s.size() > 21) s = s.substr(0, 21);
        rows.push_back(std::move(s));
    };

    if (bookmarks_.empty()) {
        push_row(" WAYPOINT BOOKMARKS ");
        push_row("--------------------");
        push_row("NO SAVED WAYPOINTS");
        push_row("USE BM ADD [NOTE]");
        push_row("TO RECORD BOOKMARK.");
        push_row("--------------------");
        push_row("BM GOTO <ID>");
        return rows;
    }

    if (items_per_page == 0) items_per_page = 3;
    const std::size_t total_pages = (bookmarks_.size() + items_per_page - 1) / items_per_page;
    if (page == 0) page = 1;
    if (page > total_pages) page = total_pages;

    char header[24];
    std::snprintf(header, sizeof(header), "BOOKMARKS  PAGE %zu/%zu", page, total_pages);
    push_row(header);
    push_row("--------------------");

    const std::size_t start_idx = (page - 1) * items_per_page;
    const std::size_t end_idx = std::min(start_idx + items_per_page, bookmarks_.size());

    for (std::size_t i = start_idx; i < end_idx; ++i) {
        const auto &bm = bookmarks_[i];

        // Distance in LY
        const double dx = bm.star_x - cur_x;
        const double dy = bm.star_y - cur_y;
        const double dz = bm.star_z - cur_z;
        const double dist_ly = std::sqrt(dx * dx + dy * dy + dz * dz) * 5E-5;

        char dist_buf[16];
        if (dist_ly < 0.1) {
            if (bm.is_surface) {
                std::snprintf(dist_buf, sizeof(dist_buf), "(SURF)");
            } else if (bm.planet_index >= 0) {
                std::snprintf(dist_buf, sizeof(dist_buf), "(LOCAL)");
            } else {
                std::snprintf(dist_buf, sizeof(dist_buf), "(HERE)");
            }
        } else if (dist_ly < 999.0) {
            std::snprintf(dist_buf, sizeof(dist_buf), "(%.0fLY)", dist_ly);
        } else {
            std::snprintf(dist_buf, sizeof(dist_buf), "(>1kLY)");
        }

        // Line 1: #ID NAME (DIST)
        std::string target_name = bm.planet_index >= 0 && !bm.planet_name.empty()
                                      ? bm.planet_name
                                      : bm.star_name;
        if (target_name.empty()) target_name = "TARGET";

        const bool is_active = (active_waypoint_id_ && *active_waypoint_id_ == bm.id);
        char line1[32];
        std::snprintf(line1, sizeof(line1), "%s#%zu %s %s",
                      is_active ? "*" : "", bm.id, target_name.c_str(), dist_buf);
        push_row(line1);

        // Line 2: "LABEL"
        if (!bm.label.empty()) {
            std::string line2 = " \"" + bm.label + "\"";
            push_row(line2);
        }

        // Line 3: Surface or Orbital location details
        if (bm.is_surface) {
            char line3[32];
            std::snprintf(line3, sizeof(line3), " LAT %+04.0f* LON %03.0f*",
                          bm.surface_lat, bm.surface_lon);
            push_row(line3);
        } else if (bm.planet_index >= 0) {
            char line3[32];
            std::snprintf(line3, sizeof(line3), " [ORBIT: BODY #%d]", bm.planet_index + 1);
            push_row(line3);
        }
    }

    push_row("--------------------");
    if (page < total_pages) {
        char prompt[24];
        std::snprintf(prompt, sizeof(prompt), "NEXT: BM %zu", page + 1);
        push_row(prompt);
    } else {
        push_row("GOTO: BM GOTO <ID>");
    }

    return rows;
}

BookmarksManager &active_bookmarks() {
    static BookmarksManager manager;
    return manager;
}

} // namespace noctis
