#include "goesnet_commands.h"

#include "bookmarks.h"
#include "flight_log.h"
#include "galaxy_sector.h"
#include "gallery.h"
#include "goesnet_data.h"
#include "video_export.h"
#include "runtime_paths.h"
#include "starmap_exchange.h"
#include "star_properties.h"
#include "system_properties.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace noctis {
namespace {
constexpr std::string_view divider = "&&&&&&&&&&&&&&&&&&&&&";

std::string_view trim_spaces(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
    return s;
}

GoesResult result(GoesResultStatus status, std::vector<std::string> rows,
                  GoesResultAction action = GoesResultAction::none) {
    std::vector<std::string_view> views;
    views.reserve(rows.size());
    for (const auto &row : rows) views.push_back(row);
    return {status, action, format_goes_rows(views), std::nullopt};
}

GoesResult data_failure(const GoesDataResult &failure) {
    switch (failure.status) {
    case GoesDataStatus::not_found: return result(GoesResultStatus::unavailable, {"DATA NOT AVAILABLE"});
    case GoesDataStatus::corrupt: return result(GoesResultStatus::corrupt_data, {"DATA FILE CORRUPT"});
    case GoesDataStatus::rejected: return result(GoesResultStatus::rejected, {"REQUEST REJECTED"});
    case GoesDataStatus::io_error: return result(GoesResultStatus::write_failed, {"DATA ACCESS ERROR"});
    case GoesDataStatus::ok: break;
    }
    return result(GoesResultStatus::corrupt_data, {"DATA ERROR"});
}

std::string display_name(const StarmapRecord &record) {
    return record.name.empty() ? "(UNNAMED)" : record.name;
}

struct ObjectArgument {
    std::string key;
    int range = 100;
    std::string suffix;
    bool valid{};
};

bool parse_positive(std::string_view text, std::size_t &value) {
    if (text.empty()) return false;
    value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9' || value > (std::numeric_limits<std::size_t>::max() - (c - '0')) / 10) return false;
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    return value != 0;
}

ObjectArgument parse_object(std::string_view argument, bool range_suffix) {
    ObjectArgument parsed;
    const auto colon = argument.find(':');
    const auto name = argument.substr(0, colon);
    if (name.empty() || name.size() > 20) return parsed;
    parsed.key.assign(name);
    std::replace(parsed.key.begin(), parsed.key.end(), '_', ' ');
    if (colon != std::string_view::npos) parsed.suffix.assign(argument.substr(colon + 1));
    if (range_suffix && !parsed.suffix.empty()) {
        std::size_t range = 0;
        if (!parse_positive(parsed.suffix, range) || range < 3 || range > 100) return parsed;
        parsed.range = static_cast<int>(range);
    }
    parsed.valid = true;
    return parsed;
}

std::optional<GalaxyStar> resolve_star(double id, double x, double y, double z, int range) {
    // 1. Fast lookup from flight log entries: visited or labeled stars have exact coordinates recorded
    for (const auto &entry : active_flight_log().entries()) {
        if (std::abs(entry.star_id - id) < goes_id_tolerance && (entry.star_x != 0.0 || entry.star_y != 0.0 || entry.star_z != 0.0)) {
            const auto star = galaxy_star_at(
                static_cast<std::int32_t>(entry.star_x / 100000.0) * 100000,
                static_cast<std::int32_t>(entry.star_y / 100000.0) * 100000,
                static_cast<std::int32_t>(entry.star_z / 100000.0) * 100000, 0);
            if (star) {
                const double candidate = static_cast<double>(star->x) / 100000.0 * star->y / 100000.0 * star->z / 100000.0;
                if (std::abs(candidate - id) < goes_id_tolerance) return star;
            }
        }
    }

    const auto start = [range](double coordinate) {
        return static_cast<std::int32_t>((coordinate - range * 50000.0) / 100000.0) * 100000;
    };
    const auto sx = start(x), sy = start(y), sz = start(z);
    for (int ix = 0; ix < range; ++ix) {
        for (int iy = 0; iy < range; ++iy) {
            for (int iz = 0; iz < range; ++iz) {
                const auto star = galaxy_star_at(sx + ix * 100000, sy + iy * 100000, sz + iz * 100000, 0);
                if (!star) continue;
                const double candidate = static_cast<double>(star->x) / 100000 * star->y / 100000 * star->z / 100000;
                if (std::abs(candidate - id) < goes_id_tolerance) return star;
            }
        }
    }
    return std::nullopt;
}

struct ResolvedObject {
    const StarmapRecord *record{};
    GalaxyStar star{};
};

GoesResult lookup_failure(const StarmapData &map, const std::vector<std::size_t> &matches) {
    if (matches.empty()) return result(GoesResultStatus::not_found, {"OBJECT NOT FOUND."});
    std::vector<std::string> rows{"AMBIGUOUS SEARCH KEY:", "PLEASE EXPAND NAME...", std::string(divider), "POSSIBLE RESULTS ARE:", std::string(divider)};
    for (const auto index : matches) rows.push_back(display_name(map.records[index]));
    rows.push_back(std::string(divider));
    return result(GoesResultStatus::ambiguous, std::move(rows));
}

std::optional<ResolvedObject> resolve_object(const StarmapData &map, const ObjectArgument &argument,
                                             const GoesCommandContext &context, GoesResult &failure) {
    const auto matches = find_starmap_objects(map, argument.key);
    if (matches.size() != 1) {
        failure = lookup_failure(map, matches);
        return std::nullopt;
    }
    const auto &record = map.records[matches.front()];
    const double star_id = record.kind == GoesObjectKind::planet ? record.id - record.ordinal : record.id;
    const auto star = resolve_star(star_id, context.observer_x, context.observer_y, context.observer_z, argument.range);
    if (!star) {
        failure = result(GoesResultStatus::not_found, {display_name(record), "IS OUT OF RANGE"});
        return std::nullopt;
    }
    return ResolvedObject{&record, *star};
}

std::string coordinate_row(char axis, double value) {
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%c=%.0f", axis, value);
    return buffer;
}

std::string count_row(std::size_t count) {
    return "    (" + std::to_string(count) + " NOTES)";
}

std::vector<std::string> wrap_note(std::string_view message) {
    std::vector<std::string> rows;
    std::string row;
    std::istringstream words{std::string(message)};
    std::string word;
    while (words >> word) {
        while (word.size() > goes_result_columns) {
            if (!row.empty()) { rows.push_back(row); row.clear(); }
            rows.push_back(word.substr(0, goes_result_columns));
            word.erase(0, goes_result_columns);
        }
        if (row.empty()) row = word;
        else if (row.size() + 1 + word.size() <= goes_result_columns) row += " " + word;
        else { rows.push_back(row); row = word; }
    }
    if (!row.empty()) rows.push_back(row);
    return rows;
}

GoesResult parse_failure(const GoesRequest &request) {
    if (request.status == GoesParseStatus::empty) return result(GoesResultStatus::usage_error, {"TYPE HELP FOR COMMANDS"});
    if (request.status == GoesParseStatus::unknown_command) return result(GoesResultStatus::unsupported, {"(UNKNOWN MODULE)"});
    return result(GoesResultStatus::usage_error, {"INVALID COMMAND SYNTAX", "TYPE HELP FOR COMMANDS"});
}

GoesResult help(std::string_view topic) {
    if (!topic.empty()) {
        if (topic == "LOG" || topic == "JOURNAL") {
            return result(GoesResultStatus::ok, {"LOG / JOURNAL", std::string(divider),
                "VIEW FLIGHT JOURNAL", "AND EXPLORATION LOG", "LOG EXPORT: SAVE MD"});
        }
        if (topic == "NAME" || topic == "LABEL") {
            return result(GoesResultStatus::ok, {"NAME / LABEL", std::string(divider),
                "NAME UNNAMED OBJECT", "NAME <NAME> FOR STAR", "P<N>:<NAME> FOR BODY"});
        }
        if (topic == "BM" || topic == "BOOKMARK" || topic == "BOOKMARKS" || topic == "WAYPOINT" || topic == "WAYPOINTS") {
            return result(GoesResultStatus::ok, {"BM / BOOKMARKS", std::string(divider),
                "BM: LIST BOOKMARKS", "BM ADD [NOTE]: MARK", "BM GOTO <ID>: TARGET", "BM DEL <ID>: REMOVE"});
        }
        if (topic == "INBOX" || topic == "IMPORT") {
            return result(GoesResultStatus::ok, {"INBOX [FILE/CHECK]", std::string(divider),
                "IMPORT STARMAP PACKET", "MERGES DISCOVERIES", "WITH INTEGRITY CHECKS", "INBOX CK: DRY RUN"});
        }
        if (topic == "OUTBOX" || topic == "EXPORT" || topic == "SHARE") {
            return result(GoesResultStatus::ok, {"OUTBOX [BIN/JSON]", std::string(divider),
                "EXPORT STARMAP PACKET", "PACKET OF DISCOVERIES", "SAVED TO DATA/ DIR", "READY FOR SHARING"});
        }
        if (topic == "CLEAN") {
            return result(GoesResultStatus::ok, {"CLEAN", std::string(divider),
                "COMPACT STARMAP FILE", "REMOVES TOMBSTONES", "RECLAIMS DISK SPACE"});
        }
        if (topic == "MOVIE" || topic == "MOVIES" || topic == "MVI") {
            return result(GoesResultStatus::ok, {"MOVIE [DECK/CMD]", std::string(divider),
                "LIST RECORDED DECKS", "MOVIE PLAY <DECK>", "PREVIEWS DECK IN HUD",
                "MOVIE EXPORT <DECK>", "EXPORTS MP4 TO FILE"});
        }
        const auto *entry = find_goes_command(topic);
        if (!entry) return result(GoesResultStatus::not_found, {"UNKNOWN HELP TOPIC"});
        return result(GoesResultStatus::ok, {std::string(entry->name), "SEE COMMAND REFERENCE"});
    }
    return result(GoesResultStatus::ok, {" GOES COMMAND HELP ", std::string(divider),
        "PAR WHERE ST DL SL", "CAT CAST REP DELE", "PRI CLR HELP", "GALLERY VIEW LOG",
        "NAME BM INBOX OUTBOX", "CLEAN", std::string(divider),
        "USE HELP COMMAND"});
}

GoesResult catalog(const StarmapData &map, const GuideData &guide, const ObjectArgument &argument) {
    const auto matches = find_starmap_objects(map, argument.key);
    if (matches.size() != 1) return lookup_failure(map, matches);
    const auto &object = map.records[matches.front()];
    std::size_t first = 1, last = std::numeric_limits<std::size_t>::max();
    if (!argument.suffix.empty()) {
        const auto dots = argument.suffix.find("..");
        if (!parse_positive(argument.suffix.substr(0, dots), first)) return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
        if (dots != std::string::npos && (!parse_positive(argument.suffix.substr(dots + 2), last) || last < first))
            return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
        if (dots == std::string::npos) last = std::numeric_limits<std::size_t>::max();
    }
    std::vector<std::string> rows{" GOES GALACTIC GUIDE ", std::string(divider),
        object.kind == GoesObjectKind::star ? "SUBJECT: STAR;" : "SUBJECT: PLANET;", display_name(object), std::string(divider)};
    const auto records = guide_records_for(guide, object.id);
    for (std::size_t ordinal = first; ordinal <= records.size() && ordinal <= last; ++ordinal) {
        rows.push_back("(" + std::to_string(ordinal) + ")");
        auto wrapped = wrap_note(guide.records[records[ordinal - 1]].message);
        rows.insert(rows.end(), wrapped.begin(), wrapped.end());
    }
    if (records.empty() || first > records.size()) {
        rows.push_back("THERE WERE NO RECORDS");
        rows.push_back("IN THE GUIDE RELATING");
        rows.push_back("SPECIFIED SUBJECT.");
    }
    return result(GoesResultStatus::ok, std::move(rows));
}
GoesResult gallery_listing(const std::filesystem::path &directory) {
    const auto entries = scan_gallery(directory);
    std::vector<std::string> rows{" GOES IMAGE ARCHIVE ", std::string(divider)};
    if (entries.empty()) {
        rows.insert(rows.end(), {"NO IMAGES ON FILE.", "PRESS M TO CAPTURE", "A SNAPSHOT."});
        return result(GoesResultStatus::ok, std::move(rows));
    }
    for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry)
        rows.push_back(entry->id + " " + gallery_kind_name(entry->kind));
    rows.push_back(std::string(divider));
    rows.push_back(std::to_string(entries.size()) + (entries.size() == 1 ? " IMAGE ON FILE." : " IMAGES ON FILE."));
    rows.push_back("VIEW N TO DISPLAY,");
    rows.push_back("VIEW EXPORT N TO SAVE.");
    return result(GoesResultStatus::ok, std::move(rows));
}

GoesResult view_image(const std::filesystem::path &directory, std::string_view key,
                      const std::optional<std::filesystem::path> &export_directory) {
    const auto entries = scan_gallery(directory);
    if (entries.empty()) return result(GoesResultStatus::not_found, {"NO IMAGES ON FILE."});

    std::string_view target_key = key;
    bool do_export = false;
    GalleryExportFormat export_fmt = GalleryExportFormat::png;

    if (key.starts_with("EXPORT ") || key.starts_with("EXP ")) {
        do_export = true;
        const auto space = key.find(' ');
        target_key = (space != std::string_view::npos) ? key.substr(space + 1) : "";
    } else if (key.ends_with(" EXPORT") || key.ends_with(" EXP")) {
        do_export = true;
        const auto space = key.rfind(' ');
        target_key = (space != std::string_view::npos) ? key.substr(0, space) : "";
    }

    if (do_export) {
        if (target_key.ends_with(" BMP") || target_key.ends_with(" bmp")) {
            export_fmt = GalleryExportFormat::bmp;
            target_key = target_key.substr(0, target_key.rfind(' '));
        } else if (target_key.ends_with(" PNG") || target_key.ends_with(" png")) {
            export_fmt = GalleryExportFormat::png;
            target_key = target_key.substr(0, target_key.rfind(' '));
        }
        const auto index = find_gallery_entry(entries, target_key);
        if (!index) return result(GoesResultStatus::not_found, {"IMAGE NOT ON FILE.", "TYPE GALLERY FOR A", "LISTING."});
        const auto &entry = entries[*index];
        const bool ok = export_gallery_image(entry, export_directory, export_fmt);
        const std::string ext = (export_fmt == GalleryExportFormat::png) ? ".PNG" : ".BMP";
        if (ok) {
            return result(GoesResultStatus::ok, {
                "IMAGE EXPORT OK",
                std::string(divider),
                "EXPORTED " + entry.id + ext,
                "SAVED TO DOWNLOADS"
            });
        } else {
            return result(GoesResultStatus::write_failed, {
                "IMAGE EXPORT FAILED",
                std::string(divider),
                "COULD NOT WRITE FILE"
            });
        }
    }

    const auto index = find_gallery_entry(entries, key);
    if (!index) return result(GoesResultStatus::not_found, {"IMAGE NOT ON FILE.", "TYPE GALLERY FOR A", "LISTING."});
    const auto &entry = entries[*index];
    auto answer = result(GoesResultStatus::ok, {"DISPLAYING IMAGE", entry.id + " " + gallery_kind_name(entry.kind),
        std::to_string(entry.width) + "X" + std::to_string(entry.height), "ESC TO CLOSE VIEWER."},
        GoesResultAction::open_image);
    answer.image_id = entry.id;
    return answer;
}

GoesResult handle_bookmarks_command(std::string_view argument, const GoesCommandContext &context) {
    if (!context.bookmarks_path.empty()) {
        active_bookmarks().load_from_file(context.bookmarks_path);
    }

    std::string arg = std::string(trim_spaces(argument));

    // BM / BM LIST / BM <page>
    if (arg.empty() || arg == "LIST" || (arg.size() <= 4 && std::all_of(arg.begin(), arg.end(), [](unsigned char c) { return std::isdigit(c); }))) {
        std::size_t page = 1;
        if (!arg.empty() && arg != "LIST") {
            try {
                page = static_cast<std::size_t>(std::stoul(arg));
            } catch (...) {
                page = 1;
            }
        }
        return result(GoesResultStatus::ok,
                      active_bookmarks().format_goes_list(page, context.observer_x, context.observer_y, context.observer_z));
    }

    // BM ADD / BM ADD <note>
    if (arg == "ADD" || arg.rfind("ADD ", 0) == 0) {
        std::string note;
        if (arg.size() > 4) {
            note = std::string(trim_spaces(std::string_view(arg).substr(4)));
        }

        Bookmark bm;
        bm.timestamp = "FLIGHT EPOC";
        bm.star_id = context.current_star_id;
        bm.star_name = context.current_star_name.empty() ? "(STAR)" : context.current_star_name;
        bm.star_class = context.current_star_class;
        bm.star_x = context.local_star_x;
        bm.star_y = context.local_star_y;
        bm.star_z = context.local_star_z;

        if (context.is_on_surface) {
            bm.is_surface = true;
            bm.surface_lat = context.surface_lat;
            bm.surface_lon = context.surface_lon;
            bm.planet_index = context.current_planet_index;
            bm.planet_name = context.current_planet_name;
        } else if (context.current_planet_index >= 0) {
            bm.is_surface = false;
            bm.planet_index = context.current_planet_index;
            bm.planet_name = context.current_planet_name;
        } else {
            bm.is_surface = false;
            bm.planet_index = -1;
        }

        if (!note.empty()) {
            bm.label = std::move(note);
        } else {
            bm.label = bm.planet_index >= 0 && !bm.planet_name.empty()
                           ? bm.planet_name
                           : bm.star_name;
        }

        const auto added = active_bookmarks().add(std::move(bm));
        if (!context.bookmarks_path.empty()) {
            active_bookmarks().save_to_file(context.bookmarks_path);
        }

        std::string loc_desc;
        if (added.is_surface) {
            loc_desc = "SURFACE OF " + (added.planet_name.empty() ? "BODY" : added.planet_name);
        } else if (added.planet_index >= 0) {
            loc_desc = "ORBIT: " + (added.planet_name.empty() ? ("BODY #" + std::to_string(added.planet_index + 1)) : added.planet_name);
        } else {
            loc_desc = "STAR " + added.star_name;
        }

        std::vector<std::string> rows = {
            " BOOKMARK RECORDED  ",
            std::string(divider),
            "WAYPOINT #" + std::to_string(added.id),
            "LABEL: " + added.label,
            std::move(loc_desc),
            "SAVED TO BOOKMARKS.",
        };
        return result(GoesResultStatus::ok, std::move(rows));
    }

    // BM GOTO <id> / BM GO <id>
    if (arg.rfind("GOTO ", 0) == 0 || arg.rfind("GO ", 0) == 0) {
        const auto space_pos = arg.find(' ');
        const auto id_str = trim_spaces(std::string_view(arg).substr(space_pos + 1));
        std::size_t id = 0;
        try {
            std::string clean_id(id_str);
            if (!clean_id.empty() && clean_id.front() == '#') clean_id.erase(0, 1);
            id = static_cast<std::size_t>(std::stoul(clean_id));
        } catch (...) {
            return result(GoesResultStatus::usage_error, {"INVALID WAYPOINT ID", "USE BM GOTO <NUM>"});
        }

        const auto bm = active_bookmarks().get(id);
        if (!bm) {
            return result(GoesResultStatus::not_found, {"WAYPOINT NOT FOUND", "USE BM TO LIST"});
        }

        active_bookmarks().set_active_waypoint_id(id);
        if (!context.bookmarks_path.empty()) {
            active_bookmarks().save_to_file(context.bookmarks_path);
        }

        const bool is_same_system =
            std::abs(bm->star_x - context.local_star_x) < 0.5 &&
            std::abs(bm->star_y - context.local_star_y) < 0.5 &&
            std::abs(bm->star_z - context.local_star_z) < 0.5;

        if (is_same_system && bm->planet_index >= 0) {
            GoesResult res;
            res.status = GoesResultStatus::ok;
            res.action = GoesResultAction::set_local_target;
            res.target = {bm->star_x, bm->star_y, bm->star_z, bm->planet_index};
            std::string target_name = bm->planet_name.empty()
                                          ? ("BODY #" + std::to_string(bm->planet_index + 1))
                                          : bm->planet_name;
            res.cells = format_goes_rows({
                " WAYPOINT LOCK ON   ",
                divider,
                "LOCAL TARGET:",
                target_name,
                "AUTOPILOT ENGAGED",
            });
            return res;
        }

        const double dx = bm->star_x - context.observer_x;
        const double dy = bm->star_y - context.observer_y;
        const double dz = bm->star_z - context.observer_z;
        const double dist_ly = std::sqrt(dx * dx + dy * dy + dz * dz) * 5E-5;
        char dist_line[24];
        std::snprintf(dist_line, sizeof(dist_line), "DIST: %.1f LY", dist_ly);

        GoesResult res;
        res.status = GoesResultStatus::ok;
        res.action = GoesResultAction::set_remote_target;
        res.target = {bm->star_x, bm->star_y, bm->star_z, bm->planet_index};
        std::string target_name = bm->star_name.empty() ? "(STAR)" : bm->star_name;
        res.cells = format_goes_rows({
            " WAYPOINT LOCK ON   ",
            divider,
            "REMOTE TARGET:",
            target_name,
            dist_line,
            "AUTOPILOT ENGAGED",
        });
        return res;
    }

    // BM DEL <id> / BM DELETE <id> / BM REMOVE <id>
    if (arg.rfind("DEL ", 0) == 0 || arg.rfind("DELETE ", 0) == 0 || arg.rfind("REMOVE ", 0) == 0) {
        const auto space_pos = arg.find(' ');
        const auto id_str = trim_spaces(std::string_view(arg).substr(space_pos + 1));
        std::size_t id = 0;
        try {
            std::string clean_id(id_str);
            if (!clean_id.empty() && clean_id.front() == '#') clean_id.erase(0, 1);
            id = static_cast<std::size_t>(std::stoul(clean_id));
        } catch (...) {
            return result(GoesResultStatus::usage_error, {"INVALID WAYPOINT ID", "USE BM DEL <NUM>"});
        }

        const bool ok = active_bookmarks().remove(id);
        if (!ok) {
            return result(GoesResultStatus::not_found, {"WAYPOINT NOT FOUND", "USE BM TO LIST"});
        }
        if (!context.bookmarks_path.empty()) {
            active_bookmarks().save_to_file(context.bookmarks_path);
        }

        return result(GoesResultStatus::ok, {
            " BOOKMARK DELETED   ",
            std::string(divider),
            "WAYPOINT #" + std::to_string(id),
            "REMOVED FROM FILE.",
        });
    }

    // BM CLEAR
    if (arg == "CLEAR") {
        active_bookmarks().clear();
        if (!context.bookmarks_path.empty()) {
            active_bookmarks().save_to_file(context.bookmarks_path);
        }
        return result(GoesResultStatus::ok, {
            " BOOKMARKS CLEARED  ",
            std::string(divider),
            "ALL WAYPOINTS",
            "REMOVED FROM FILE.",
        });
    }

    return result(GoesResultStatus::usage_error, {
        "INVALID BM ARGUMENT",
        std::string(divider),
        "USE BM, BM ADD,",
        "BM GOTO <ID>,",
        "OR BM DEL <ID>",
    });
}

GoesResult handle_movie_command(const std::filesystem::path &movies_path, std::string_view argument) {
    const auto effective_path = !movies_path.empty() ? movies_path : (runtime_paths().movies_dir);
    const auto decks = scan_movie_decks(effective_path);

    if (argument.empty() || argument == "LIST") {
        if (decks.empty()) {
            return result(GoesResultStatus::ok, {
                " NO RECORDED DECKS. ",
                std::string(divider),
                "USE F3 MOVIEMAKER",
                "TO RECORD GAMEPLAY",
                "IMAGE SEQUENCES."
            });
        }
        std::vector<std::string> rows;
        rows.push_back(" MOVIEDECK ARCHIVE  ");
        rows.push_back(std::string(divider));
        for (const auto &d : decks) {
            char line[40];
            std::snprintf(line, sizeof(line), "DECK %s: %zu FRAMES", d.deck_str.c_str(), d.frame_count);
            rows.emplace_back(line);
        }
        rows.push_back(std::string(divider));
        rows.push_back(std::to_string(decks.size()) + (decks.size() == 1 ? " DECK ON FILE." : " DECKS ON FILE."));
        rows.push_back("MOVIE PLAY [N] TO VIEW");
#if defined(__EMSCRIPTEN__)
        rows.push_back("MOVIE EXPORT [N] WEBM");
#else
        rows.push_back("MOVIE EXPORT [N] TO MP4");
#endif
        return result(GoesResultStatus::ok, std::move(rows));
    }

    if (argument.starts_with("EXPORT")) {
        auto sub = trim_spaces(argument.substr(6));
        std::size_t num = 0;
        if (sub.empty() && !decks.empty()) {
            num = decks.back().deck;
        } else if (!parse_positive(sub, num)) {
            return result(GoesResultStatus::usage_error, {"INVALID DECK NUMBER", "USE MOVIE EXPORT <N>"});
        }

        const MovieDeckEntry *target_deck = nullptr;
        for (const auto &d : decks) {
            if (d.deck == num) { target_deck = &d; break; }
        }
        if (!target_deck) {
            return result(GoesResultStatus::not_found, {"DECK NOT FOUND.", "TYPE MOVIE FOR LIST."});
        }

#if defined(__EMSCRIPTEN__)
        const auto deck_path = target_deck->path.string();
        const bool started = start_browser_deck_export(
            deck_path.c_str(), target_deck->deck_str.c_str(), target_deck->fps);
        if (!started) {
            return result(GoesResultStatus::unavailable, {
                " WEBM EXPORT BUSY   ",
                std::string(divider),
                "WAIT FOR THE CURRENT",
                "EXPORT TO FINISH."
            });
        }
#else
        VideoExportOptions opts;
        opts.deck_dir = target_deck->path;
        opts.fps = target_deck->fps;
        opts.format = VideoFormat::mp4;
        start_video_export_async(opts);
#endif

        return result(GoesResultStatus::ok, {
            " EXPORTING MOVIEDECK ",
            std::string(divider),
            "DECK " + target_deck->deck_str + ": " + std::to_string(target_deck->frame_count) + " FRAMES",
            "ENCODING IN PROGRESS",
#if defined(__EMSCRIPTEN__)
            "DOWNLOADING WEBM",
#else
            "SAVING TO DOWNLOADS",
#endif
        }, GoesResultAction::export_created);
    }

    auto play_arg = argument;
    if (play_arg.starts_with("PLAY")) {
        play_arg = trim_spaces(play_arg.substr(4));
    }

    std::size_t num = 0;
    if (play_arg.empty() && !decks.empty()) {
        num = decks.back().deck;
    } else if (!parse_positive(play_arg, num)) {
        return result(GoesResultStatus::usage_error, {"INVALID DECK NUMBER", "USE MOVIE PLAY <N>"});
    }

    const MovieDeckEntry *target_deck = nullptr;
    for (const auto &d : decks) {
        if (d.deck == num) { target_deck = &d; break; }
    }
    if (!target_deck) {
        return result(GoesResultStatus::not_found, {"DECK NOT FOUND.", "TYPE MOVIE FOR LIST."});
    }

    auto res = result(GoesResultStatus::ok, {
        " OPENING MOVIEDECK   ",
        std::string(divider),
        "DECK " + target_deck->deck_str + " (" + std::to_string(target_deck->frame_count) + " FRAMES)",
        "STARDRIFTER PROJECTOR",
        "ESC TO CLOSE VIEWER.",
    }, GoesResultAction::open_movie);
    res.movie_deck = target_deck->deck;
    return res;
}
} // namespace

GoesResult execute_goes_command(std::string_view console_line, const GoesCommandContext &context) {
    const auto request = parse_goes_command(console_line);
    if (request.status != GoesParseStatus::ok) return parse_failure(request);
    if (request.command == GoesCommand::clear) return {GoesResultStatus::ok, GoesResultAction::clear_output, {}, std::nullopt};
    if (request.command == GoesCommand::help) return help(request.argument);
    if (request.command == GoesCommand::gallery) return gallery_listing(context.gallery_path);
    if (request.command == GoesCommand::view_image) {
        return view_image(context.gallery_path, request.argument, context.image_export_directory);
    }
    if (request.command == GoesCommand::movie) return handle_movie_command(context.movies_path, request.argument);
    if (request.command == GoesCommand::flight_log) {
        if (request.argument.empty()) {
            return result(GoesResultStatus::ok, active_flight_log().format_goes_summary());
        }
        if (request.argument == "EXPORT") {
            const auto dir = context.starmap_path.parent_path();
            const bool ok_md = active_flight_log().export_markdown(dir / "flight_log.md");
            const bool ok_json = active_flight_log().export_json(dir / "flight_log.json");
            if (!ok_md || !ok_json) {
                return result(GoesResultStatus::write_failed, {"EXPORT FAILED", "CHECK DISK ACCESS"});
            }
            return result(GoesResultStatus::ok,
                          {" FLIGHT LOG EXPORTED ", std::string(divider),
                           "SAVED TO FLIGHT_LOG", "MD AND JSON FILES."},
                          GoesResultAction::export_created);
        }
        return result(GoesResultStatus::usage_error, {"INVALID LOG ARGUMENT", "USE LOG OR LOG EXPORT"});
    }
    if (request.command == GoesCommand::bookmarks) {
        return handle_bookmarks_command(request.argument, context);
    }
    if (request.command == GoesCommand::clean) {
        std::size_t compacted = 0;
        const auto res = compact_starmap(context.starmap_path, compacted);
        if (res.status != GoesDataStatus::ok) return data_failure(res);
        if (compacted > 0) {
            return result(GoesResultStatus::ok,
                          {" GOES STARMAP CLEAN  ", std::string(divider),
                           "STARMAP COMPACTED.",
                           "TOMBSTONES ERASED: " + std::to_string(compacted),
                           "SPACE RECLAIMED."},
                          GoesResultAction::catalog_changed);
        }
        return result(GoesResultStatus::ok,
                      {" GOES STARMAP CLEAN  ", std::string(divider),
                       "STARMAP IS CLEAN.", "NO TOMBSTONES FOUND.", "MAP COMPACTED OK."});
    }

    if (request.command == GoesCommand::outbox) {
        const auto dir = context.starmap_path.parent_path();
        StarmapPacketFormat fmt = StarmapPacketFormat::auto_detect;
        if (request.argument == "JSON") fmt = StarmapPacketFormat::json;
        else if (request.argument == "BIN" || request.argument == "NSM") fmt = StarmapPacketFormat::binary_nsm;

        const auto rep = export_starmap_packet(context.starmap_path, context.guide_path, dir, fmt);
        if (rep.status == StarmapExchangeStatus::no_records_to_export) {
            return result(GoesResultStatus::ok,
                          {" GOES STARMAP OUTBOX ", std::string(divider),
                           "NO USER RECORDS FOUND", "STARMAP UNMODIFIED.",
                           "NAME STARS OR BODIES", "BEFORE EXPORTING."});
        }
        if (rep.status != StarmapExchangeStatus::ok) {
            return result(GoesResultStatus::write_failed,
                          {"EXPORT FAILED", "CHECK DISK ACCESS"});
        }
        return result(GoesResultStatus::ok,
                      {" GOES STARMAP OUTBOX ", std::string(divider),
                       "EXPORTED: " + std::to_string(rep.records_exported) + " BODIES",
                       "OUTBOX.NSM SAVED", "OUTBOX.JSON SAVED", "READY FOR SHARING."},
                      GoesResultAction::export_created);
    }

    if (request.command == GoesCommand::inbox) {
        const auto dir = context.starmap_path.parent_path();
        std::string arg = std::string(trim_spaces(request.argument));
        bool dry_run = false;
        std::filesystem::path packet_file;

        if (arg == "CHECK" || arg == "CK") {
            dry_run = true;
            arg.clear();
        } else if (arg.rfind("CHECK ", 0) == 0) {
            dry_run = true;
            arg = std::string(trim_spaces(std::string_view(arg).substr(6)));
        } else if (arg.rfind("CK ", 0) == 0) {
            dry_run = true;
            arg = std::string(trim_spaces(std::string_view(arg).substr(3)));
        }

        if (!arg.empty()) {
            std::filesystem::path candidate(arg);
            if (candidate.is_absolute() && std::filesystem::exists(candidate)) {
                packet_file = candidate;
            } else if (std::filesystem::exists(dir / candidate)) {
                packet_file = dir / candidate;
            } else if (std::filesystem::exists(candidate)) {
                packet_file = candidate;
            } else {
                return result(GoesResultStatus::not_found,
                              {" GOES STARMAP INBOX  ", std::string(divider),
                               "PACKET NOT FOUND:", arg.substr(0, 21), "CHECK DATA/ DIR."});
            }
        } else {
            for (const char *cand : {"inbox.nsm", "inbox.json", "inbox.bin", "outbox.nsm", "outbox.json"}) {
                if (std::filesystem::exists(dir / cand)) {
                    packet_file = dir / cand;
                    break;
                }
            }
            if (packet_file.empty()) {
                return result(GoesResultStatus::not_found,
                              {" GOES STARMAP INBOX  ", std::string(divider),
                               "NO PACKET FOUND", "PLACE INBOX.NSM OR",
                               "INBOX.JSON IN DATA/", "DIR BEFORE RUNNING."});
            }
        }

        StarmapImportOptions opts;
        opts.dry_run = dry_run;
        opts.skip_conflicts = true;

        const auto rep = import_starmap_packet(context.starmap_path, context.guide_path, packet_file, opts);

        if (rep.status == StarmapExchangeStatus::checksum_mismatch) {
            return result(GoesResultStatus::corrupt_data,
                          {" GOES STARMAP INBOX  ", std::string(divider),
                           "PACKET CORRUPTED", "CHECKSUM MISMATCH", "IMPORT ABORTED."});
        }
        if (rep.status == StarmapExchangeStatus::payload_corrupted || rep.status == StarmapExchangeStatus::format_invalid) {
            return result(GoesResultStatus::corrupt_data,
                          {" GOES STARMAP INBOX  ", std::string(divider),
                           "INVALID PACKET FORMAT", "FILE CORRUPTED", "IMPORT ABORTED."});
        }
        if (rep.status == StarmapExchangeStatus::capacity_exceeded) {
            return result(GoesResultStatus::rejected,
                          {" GOES STARMAP INBOX  ", std::string(divider),
                           "CAPACITY EXCEEDED", "PACKET TOO LARGE", "IMPORT ABORTED."});
        }
        if (rep.status != StarmapExchangeStatus::ok) {
            return result(GoesResultStatus::rejected,
                          {" GOES STARMAP INBOX  ", std::string(divider),
                           "VALIDATION FAILED", "IMPORT ABORTED."});
        }

        std::vector<std::string> rows = {
            dry_run ? " INBOX VERIFICATION  " : " GOES STARMAP INBOX  ",
            std::string(divider),
            "SCANNED: " + std::to_string(rep.records_scanned),
            "IMPORTED: " + std::to_string(rep.records_imported),
            "DUPLICATES: " + std::to_string(rep.duplicates_skipped),
        };
        const std::size_t conflicts = rep.id_conflicts + rep.name_collisions + rep.protected_conflicts + rep.invalid_records;
        if (conflicts > 0) {
            rows.push_back("CONFLICTS: " + std::to_string(conflicts));
        }
        if (rep.guide_notes_imported > 0) {
            rows.push_back("NOTES ADDED: " + std::to_string(rep.guide_notes_imported));
        }
        rows.push_back(dry_run ? "VERIFICATION OK." : "MAP UPDATED OK.");
        return result(GoesResultStatus::ok, std::move(rows),
                      dry_run ? GoesResultAction::none : GoesResultAction::catalog_changed);
    }

    StarmapData map;
    auto loaded = load_starmap(context.starmap_path, map);
    if (loaded.status != GoesDataStatus::ok) return data_failure(loaded);

    if (request.command == GoesCommand::parameters || request.command == GoesCommand::set_target
        || request.command == GoesCommand::dependencies || request.command == GoesCommand::parent) {
        const auto argument = parse_object(request.argument, request.command != GoesCommand::parent);
        if (!argument.valid) return result(GoesResultStatus::usage_error, {"INVALID OBJECT OR RANGE"});
        GoesResult failure;
        const auto object = resolve_object(map, argument, context, failure);
        if (!object) return failure;
        if (request.command == GoesCommand::parameters) {
            return result(GoesResultStatus::ok, {"GOES STARMAP ANALYSIS", std::string(divider),
                object->record->kind == GoesObjectKind::star ? "SUBJECT: STAR;" : "SUBJECT: PLANET;",
                "NAME: " + display_name(*object->record), coordinate_row('X', object->star.x),
                coordinate_row('Y', -object->star.y), coordinate_row('Z', object->star.z)});
        }
        if (request.command == GoesCommand::parent) {
            if (object->record->kind == GoesObjectKind::star)
                return result(GoesResultStatus::rejected, {"THIS OBJECT IS A STAR", "USE THE PAR COMMAND"});
            const double parent_id = object->record->id - object->record->ordinal;
            const auto parent = std::find_if(map.records.begin(), map.records.end(), [&](const auto &candidate) {
                return !candidate.removed && candidate.kind == GoesObjectKind::star
                    && std::abs(candidate.id - parent_id) < goes_id_tolerance;
            });
            if (parent == map.records.end()) return result(GoesResultStatus::not_found, {"PARENT STAR UNKNOWN"});
            return result(GoesResultStatus::ok, {display_name(*object->record), "IS PART OF THE", display_name(*parent), "SYSTEM."});
        }
        if (request.command == GoesCommand::set_target) {
            GoesResult answer;
            if (object->record->kind == GoesObjectKind::star) {
                answer = result(GoesResultStatus::ok, {"REM. TARGET DATA SENT", "STARTING VIMANA DRIVE"}, GoesResultAction::set_remote_target);
                answer.target = GoesResult::Target{static_cast<double>(object->star.x), static_cast<double>(object->star.y), static_cast<double>(object->star.z), -1};
                return answer;
            }
            if (std::abs(context.local_star_x - object->star.x) >= goes_id_tolerance
                || std::abs(context.local_star_y - object->star.y) >= goes_id_tolerance
                || std::abs(context.local_star_z - object->star.z) >= goes_id_tolerance)
                return result(GoesResultStatus::rejected, {"PLANET NOT FOUND AS", "PART OF THIS SYSTEM.", "USE PAR FOR ITS STAR"});
            const auto properties = derive_star_properties(object->star.x, object->star.y, object->star.z);
            const auto system = derive_planet_system(object->star.x, object->star.y, object->star.z,
                                                     properties.star_class, properties.radius);
            const auto index = object->record->ordinal - 1;
            if (index < 0 || index >= system.body_count) return result(GoesResultStatus::not_found, {"PLANET NOT IN SYSTEM"});
            answer = result(GoesResultStatus::ok, {"LOC. TARGET DATA SENT", "BEGIN IN-SYSTEM DRIVE"}, GoesResultAction::set_local_target);
            answer.target = GoesResult::Target{static_cast<double>(object->star.x), static_cast<double>(object->star.y), static_cast<double>(object->star.z), static_cast<std::int16_t>(index)};
            return answer;
        }

        GuideData guide;
        loaded = load_guide(context.guide_path, guide);
        if (loaded.status != GoesDataStatus::ok) return data_failure(loaded);
        const auto properties = derive_star_properties(object->star.x, object->star.y, object->star.z);
        const auto system = derive_planet_system(object->star.x, object->star.y, object->star.z,
                                                 properties.star_class, properties.radius);
        std::vector<std::string> rows{"DEPENDENCIES LISTING:", std::string(divider), "*" + display_name(*object->record)};
        const auto subject_notes = guide_records_for(guide, object->record->id).size();
        if (subject_notes) rows.push_back(count_row(subject_notes));
        for (const auto &candidate : map.records) {
            if (candidate.removed || candidate.kind != GoesObjectKind::planet || candidate.ordinal <= 0
                || std::abs((candidate.id - candidate.ordinal) - (object->record->id - (object->record->kind == GoesObjectKind::planet ? object->record->ordinal : 0))) >= goes_id_tolerance)
                continue;
            const int index = candidate.ordinal - 1;
            if (index < 0 || index >= system.body_count) continue;
            if (object->record->kind == GoesObjectKind::planet && system.bodies[index].owner != object->record->ordinal - 1) continue;
            if (object->record->kind == GoesObjectKind::star && system.bodies[index].owner != -1) continue;
            rows.push_back(display_name(candidate));
            const auto notes = guide_records_for(guide, candidate.id).size();
            if (notes) rows.push_back(count_row(notes));
        }
        rows.push_back(std::string(divider));
        rows.push_back(object->record->kind == GoesObjectKind::star ? "PLANETS LISTING END." : "MOONS LISTING END.");
        return result(GoesResultStatus::ok, std::move(rows));
    }

    const auto argument = parse_object(request.argument, false);
    if ((request.command != GoesCommand::list_stars) && !argument.valid)
        return result(GoesResultStatus::usage_error, {"INVALID COMMAND ARGUMENT"});

    if (request.command == GoesCommand::catalog || request.command == GoesCommand::print_guide) {
        GuideData guide;
        loaded = load_guide(context.guide_path, guide);
        if (loaded.status != GoesDataStatus::ok) return data_failure(loaded);
        auto answer = catalog(map, guide, argument);
        if (request.command == GoesCommand::print_guide && answer.status == GoesResultStatus::ok) {
            std::ofstream output(context.export_path, std::ios::binary | std::ios::trunc);
            if (!output.write(answer.cells.data(), static_cast<std::streamsize>(answer.cells.size())) || !output.flush())
                return result(GoesResultStatus::write_failed, {"EXPORT FAILED"});
            answer.action = GoesResultAction::export_created;
        }
        return answer;
    }

    if (request.command == GoesCommand::add_note || request.command == GoesCommand::replace_note
        || request.command == GoesCommand::delete_note) {
        const auto matches = find_starmap_objects(map, argument.key);
        if (matches.size() != 1) return lookup_failure(map, matches);
        const auto id = map.records[matches.front()].id;
        GoesDataResult mutation;
        if (request.command == GoesCommand::add_note) {
            if (argument.suffix.empty()) return result(GoesResultStatus::usage_error, {"NOTE TEXT REQUIRED"});
            mutation = append_guide_record(context.guide_path, id, argument.suffix);
        } else if (request.command == GoesCommand::replace_note) {
            const auto colon = argument.suffix.find(':');
            std::size_t ordinal = 0;
            if (colon == std::string::npos || !parse_positive(argument.suffix.substr(0, colon), ordinal)
                || colon + 1 == argument.suffix.size()) return result(GoesResultStatus::usage_error, {"RECORD AND NOTE NEEDED"});
            mutation = replace_guide_record(context.guide_path, id, ordinal, argument.suffix.substr(colon + 1));
        } else {
            std::size_t first = 1, last = std::numeric_limits<std::size_t>::max();
            if (!argument.suffix.empty()) {
                const auto dots = argument.suffix.find("..");
                if (!parse_positive(argument.suffix.substr(0, dots), first)) return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
                last = first;
                if (dots != std::string::npos && (!parse_positive(argument.suffix.substr(dots + 2), last) || last < first))
                    return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
            }
            std::size_t removed = 0, protected_records = 0;
            mutation = delete_guide_records(context.guide_path, id, first, last, removed, protected_records);
            if (mutation.status == GoesDataStatus::ok)
                return result(GoesResultStatus::ok, {"TOTAL SELECTED: " + std::to_string(removed + protected_records),
                    "REMOVED: " + std::to_string(removed), "PROTECTED: " + std::to_string(protected_records)}, GoesResultAction::catalog_changed);
        }
        if (mutation.status != GoesDataStatus::ok) return data_failure(mutation);
        return result(GoesResultStatus::ok, {"MESSAGE ACCEPTED."}, GoesResultAction::catalog_changed);
    }

    if (request.command == GoesCommand::list_stars) {
        std::vector<std::string> rows{request.argument.empty() ? "GLOBAL STARS LISTING:" : "RANGED STARS LISTING:", std::string(divider)};
        if (request.argument.empty()) {
            for (const auto &record : map.records)
                if (!record.removed && record.kind == GoesObjectKind::star && !record.name.empty()) rows.push_back("*" + record.name);
        } else {
            std::size_t requested_range = 0;
            if (!parse_positive(request.argument, requested_range) || requested_range < 3 || requested_range > 100)
                return result(GoesResultStatus::usage_error, {"RANGE MUST BE 3..100"});
            std::unordered_multimap<std::int64_t, std::size_t> ids;
            for (std::size_t index = 0; index < map.records.size(); ++index) {
                const auto &record = map.records[index];
                if (!record.removed && record.kind == GoesObjectKind::star && !record.name.empty())
                    ids.emplace(std::llround(record.id / goes_id_tolerance), index);
            }
            struct Located { std::size_t index; GalaxyStar star; };
            std::vector<Located> located;
            const int range = static_cast<int>(requested_range);
            const auto start = [range](double coordinate) {
                return static_cast<std::int32_t>((coordinate - range * 50000.0) / 100000.0) * 100000;
            };
            const auto sx = start(context.observer_x), sy = start(context.observer_y), sz = start(context.observer_z);
            for (int ix = 0; ix < range; ++ix) for (int iy = 0; iy < range; ++iy) for (int iz = 0; iz < range; ++iz) {
                const auto star = galaxy_star_at(sx + ix * 100000, sy + iy * 100000, sz + iz * 100000, 0);
                if (!star) continue;
                const double id = static_cast<double>(star->x) / 100000 * star->y / 100000 * star->z / 100000;
                const auto key = std::llround(id / goes_id_tolerance);
                for (std::int64_t candidate_key = key - 1; candidate_key <= key + 1; ++candidate_key) {
                    const auto [begin, end] = ids.equal_range(candidate_key);
                    for (auto candidate = begin; candidate != end; ++candidate) {
                        if (std::abs(map.records[candidate->second].id - id) < goes_id_tolerance)
                            located.push_back({candidate->second, *star});
                    }
                }
            }
            std::sort(located.begin(), located.end(), [](const auto &left, const auto &right) { return left.index < right.index; });
            located.erase(std::unique(located.begin(), located.end(), [](const auto &left, const auto &right) { return left.index == right.index; }), located.end());
            for (const auto &entry : located) {
                rows.push_back("*" + map.records[entry.index].name);
                rows.push_back("$" + coordinate_row('X', entry.star.x));
                rows.push_back("$" + coordinate_row('Y', -entry.star.y));
                rows.push_back("$" + coordinate_row('Z', entry.star.z));
                const double dx = context.observer_x - entry.star.x;
                const double dy = context.observer_y - entry.star.y;
                const double dz = context.observer_z - entry.star.z;
                char distance[40];
                std::snprintf(distance, sizeof(distance), "$D=%.2f L.Y.", std::sqrt(dx * dx + dy * dy + dz * dz) * 5E-5);
                rows.emplace_back(distance);
                rows.push_back("[&&&&&&&&&&&&&&&&&&&&");
            }
        }
        rows.push_back(std::string(divider));
        rows.push_back("STARS LISTING END.");
        return result(GoesResultStatus::ok, std::move(rows));
    }

    if (request.command == GoesCommand::name_object) {
        std::string target_key;
        std::string new_name;
        std::string_view arg = trim_spaces(request.argument);
        const auto colon = arg.find(':');
        if (colon != std::string_view::npos) {
            target_key = std::string(trim_spaces(arg.substr(0, colon)));
            new_name = std::string(trim_spaces(arg.substr(colon + 1)));
        } else {
            if (arg.rfind("STAR ", 0) == 0) {
                target_key = "STAR";
                new_name = std::string(trim_spaces(arg.substr(5)));
            } else if (arg.rfind("HERE ", 0) == 0) {
                target_key = "STAR";
                new_name = std::string(trim_spaces(arg.substr(5)));
            } else if (arg.rfind("LOCAL ", 0) == 0) {
                target_key = "STAR";
                new_name = std::string(trim_spaces(arg.substr(6)));
            } else if (arg.rfind("CURRENT ", 0) == 0) {
                target_key = "STAR";
                new_name = std::string(trim_spaces(arg.substr(8)));
            } else if (arg.size() > 2 && (arg[0] == 'P' || arg[0] == 'p') && std::isdigit(static_cast<unsigned char>(arg[1])) && arg.find(' ') != std::string_view::npos) {
                const auto space = arg.find(' ');
                target_key = std::string(arg.substr(0, space));
                new_name = std::string(trim_spaces(arg.substr(space + 1)));
            } else if (arg.rfind("PLANET ", 0) == 0) {
                std::string_view rest = trim_spaces(arg.substr(7));
                const auto space = rest.find(' ');
                if (space != std::string_view::npos) {
                    target_key = "P" + std::string(trim_spaces(rest.substr(0, space)));
                    new_name = std::string(trim_spaces(rest.substr(space + 1)));
                } else {
                    target_key = "P" + std::string(rest);
                }
            } else {
                target_key = "STAR";
                new_name = std::string(arg);
            }
        }

        if (new_name.empty() || new_name.size() > 20) {
            return result(GoesResultStatus::usage_error, {"NAME REQUIRED", "1-20 CHARACTERS"});
        }

        const double star_id = context.local_star_x / 100000.0 * context.local_star_y / 100000.0 * context.local_star_z / 100000.0;

        if (target_key == "STAR" || target_key == "HERE" || target_key == "LOCAL" || target_key == "CURRENT") {
            const auto existing = find_starmap_name_by_id(map, star_id);
            if (existing) {
                return result(GoesResultStatus::rejected, {"STAR ALREADY LABELED", *existing});
            }
            const auto properties = derive_star_properties(context.local_star_x, context.local_star_y, context.local_star_z);
            const std::int16_t star_class = properties.star_class;
            std::int32_t offset = -1;
            const auto mutation = append_starmap_label(context.starmap_path, star_id, new_name,
                                                       GoesObjectKind::star, star_class, offset);
            if (mutation.status != GoesDataStatus::ok) return data_failure(mutation);
            active_flight_log().record_label_assigned(star_id, new_name, false, star_class);
            active_flight_log().save_to_file(context.starmap_path.parent_path() / "flight_log.json");
            return result(GoesResultStatus::ok,
                          {"OBJECT LABELED", std::string(divider), "NAME ASSIGNED:", new_name, "RECORD SAVED TO MAP."},
                          GoesResultAction::catalog_changed);
        }

        bool is_planet = false;
        std::size_t planet_num = 0;
        if (target_key.rfind("PLANET", 0) == 0) {
            std::string_view rest = trim_spaces(std::string_view(target_key).substr(6));
            if (parse_positive(rest, planet_num)) is_planet = true;
        } else if (target_key.size() > 1 && target_key.front() == 'P') {
            std::string_view rest = trim_spaces(std::string_view(target_key).substr(1));
            if (parse_positive(rest, planet_num)) is_planet = true;
        } else if (parse_positive(target_key, planet_num)) {
            is_planet = true;
        }

        if (is_planet) {
            const auto properties = derive_star_properties(context.local_star_x, context.local_star_y, context.local_star_z);
            const auto system = derive_planet_system(context.local_star_x, context.local_star_y, context.local_star_z,
                                                     properties.star_class, properties.radius);
            if (planet_num < 1 || planet_num > static_cast<std::size_t>(system.body_count)) {
                return result(GoesResultStatus::usage_error,
                              {"INVALID BODY NUMBER", "SYSTEM HAS " + std::to_string(system.body_count) + " BODIES"});
            }
            const double planet_id = star_id + planet_num;
            const auto existing = find_starmap_name_by_id(map, planet_id);
            if (existing) {
                return result(GoesResultStatus::rejected, {"BODY ALREADY LABELED", *existing});
            }
            std::int32_t offset = -1;
            const auto mutation = append_starmap_label(context.starmap_path, planet_id, new_name,
                                                       GoesObjectKind::planet, static_cast<std::int16_t>(planet_num), offset);
            if (mutation.status != GoesDataStatus::ok) return data_failure(mutation);
            active_flight_log().record_label_assigned(planet_id, new_name, true, static_cast<std::int16_t>(planet_num));
            active_flight_log().save_to_file(context.starmap_path.parent_path() / "flight_log.json");
            return result(GoesResultStatus::ok,
                          {"OBJECT LABELED", std::string(divider),
                           "BODY #" + std::to_string(planet_num) + ":", new_name, "RECORD SAVED TO MAP."},
                          GoesResultAction::catalog_changed);
        }

        const auto matches = find_starmap_objects(map, target_key);
        if (!matches.empty()) {
            return result(GoesResultStatus::rejected, {"OBJECT IS LABELED", "RENAME NOT PERMITTED"});
        }
        return result(GoesResultStatus::usage_error, {"UNKNOWN TARGET OBJECT", "USE STAR OR P<N>"});
    }

    return result(GoesResultStatus::unsupported, {"COMMAND NOT IMPLEMENTED"});
}

} // namespace noctis
