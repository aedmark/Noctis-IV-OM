#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

struct Bookmark {
    std::size_t id{0};
    std::string label;
    double star_id{0.0};
    std::string star_name;
    std::int16_t star_class{0};
    double star_x{0.0};
    double star_y{0.0};
    double star_z{0.0};
    std::int16_t planet_index{-1}; // -1 if deep space / star itself
    std::string planet_name;
    bool is_surface{false};
    double surface_lat{0.0};
    double surface_lon{0.0};
    std::string timestamp;
};

class BookmarksManager {
public:
    BookmarksManager() = default;

    [[nodiscard]] const std::vector<Bookmark> &all() const { return bookmarks_; }
    [[nodiscard]] std::size_t count() const { return bookmarks_.size(); }
    [[nodiscard]] bool empty() const { return bookmarks_.empty(); }

    [[nodiscard]] std::optional<Bookmark> get(std::size_t id) const;
    [[nodiscard]] const Bookmark *find(std::size_t id) const;

    Bookmark add(Bookmark bm);
    bool remove(std::size_t id);
    void clear();

    [[nodiscard]] std::optional<std::size_t> active_waypoint_id() const { return active_waypoint_id_; }
    void set_active_waypoint_id(std::optional<std::size_t> id) { active_waypoint_id_ = id; }

    bool load_from_file(const std::filesystem::path &path);
    bool save_to_file(const std::filesystem::path &path) const;

    [[nodiscard]] std::vector<std::string> format_goes_list(
        std::size_t page,
        double cur_x, double cur_y, double cur_z,
        std::size_t items_per_page = 3) const;

private:
    std::vector<Bookmark> bookmarks_;
    std::optional<std::size_t> active_waypoint_id_;
    std::size_t next_id_{1};
};

BookmarksManager &active_bookmarks();

} // namespace noctis
