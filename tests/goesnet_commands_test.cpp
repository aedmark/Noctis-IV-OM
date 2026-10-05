#include "goesnet_commands.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::fprintf(stderr, "GOESnet commands: %s\n", message);
    return condition;
}

bool contains(const noctis::GoesResult &result, std::string_view text) {
    return result.cells.find(text) != std::string::npos;
}

std::vector<char> read_bytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
}

int main(int argc, char **argv) {
    using namespace noctis;
    if (argc != 5) {
        std::fputs("usage: goesnet_commands_test STARMAP GUIDE TEMP EXPORT\n", stderr);
        return 2;
    }
    const std::filesystem::path guide_copy(argv[3]);
    std::filesystem::copy_file(argv[2], guide_copy, std::filesystem::copy_options::overwrite_existing);
    GoesCommandContext context{argv[1], guide_copy, argv[4], 3797120, -4352112, -925018,
                               -18928, -29680, -67336};
    bool ok = true;

    auto answer = execute_goes_command("HELP_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "PAR WHERE ST DL SL"), "HELP mismatch");
    ok &= require(contains(answer, "GALLERY VIEW"), "HELP omits gallery commands");

    const auto gallery = std::filesystem::path(argv[4]).parent_path() / "goesnet-gallery";
    std::filesystem::remove_all(gallery);
    std::filesystem::create_directories(gallery);
    context.gallery_path = gallery;
    answer = execute_goes_command("GALLERY_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "NO IMAGES ON FILE."), "empty GALLERY mismatch");
    answer = execute_goes_command("VIEW_", context);
    ok &= require(answer.status == GoesResultStatus::not_found && answer.action == GoesResultAction::none, "empty VIEW opened");
    for (const char *name : {"00000004.BMP", "00000009.BMP"}) {
        std::vector<char> bmp(1078 + 320 * 200, 0);
        bmp[0] = 'B'; bmp[1] = 'M'; bmp[10] = 0x36; bmp[11] = 0x04; bmp[14] = 40;
        bmp[18] = 0x40; bmp[19] = 0x01; bmp[22] = static_cast<char>(200); bmp[26] = 1; bmp[28] = 8;
        std::ofstream(gallery / name, std::ios::binary).write(bmp.data(), static_cast<std::streamsize>(bmp.size()));
    }
    answer = execute_goes_command("GALLERY_", context);
    ok &= require(answer.cells.find("00000009 SNAPSHOT") < answer.cells.find("00000004 SNAPSHOT")
                      && contains(answer, "2 IMAGES ON FILE."), "GALLERY listing mismatch");
    answer = execute_goes_command("GALLERY 4_", context);
    ok &= require(answer.status == GoesResultStatus::usage_error, "GALLERY accepted an argument");
    answer = execute_goes_command("VIEW_", context);
    ok &= require(answer.action == GoesResultAction::open_image && answer.image_id == "00000009", "VIEW newest mismatch");
    answer = execute_goes_command("view 4_", context);
    ok &= require(answer.action == GoesResultAction::open_image && answer.image_id == "00000004"
                      && contains(answer, "320X200"), "VIEW number mismatch");
    answer = execute_goes_command("VIEW 5_", context);
    ok &= require(answer.status == GoesResultStatus::not_found && contains(answer, "IMAGE NOT ON FILE."), "VIEW absent mismatch");
    std::filesystem::remove_all(gallery);
    answer = execute_goes_command("PAR MIRACLE_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "SUBJECT: STAR;")
                      && contains(answer, "NAME: MIRACLE") && contains(answer, "X=3979984")
                      && contains(answer, "Y=5143407") && contains(answer, "Z=-98451"),
                  "PAR MIRACLE differs from DOS evidence");
    answer = execute_goes_command("PAR FELYSIA_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "SUBJECT: PLANET;")
                      && contains(answer, "X=-18928") && contains(answer, "Y=29680")
                      && contains(answer, "Z=-67336"), "PAR FELYSIA parent mismatch");
    answer = execute_goes_command("WHERE FELYSIA_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BALASTRACKONASTREYA"),
                  "WHERE FELYSIA mismatch");
    auto local_context = context;
    local_context.observer_x = -18928;
    local_context.observer_y = -29680;
    local_context.observer_z = -67336;
    answer = execute_goes_command("SL 3_", local_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BALASTRACKONASTREYA")
                      && contains(answer, "$X=-18928"), "ranged SL mismatch");
    answer = execute_goes_command("ST MIRACLE_", context);
    ok &= require(answer.action == GoesResultAction::set_remote_target && answer.target
                      && answer.target->x == 3979984 && answer.target->y == -5143407,
                  "remote ST action mismatch");
    answer = execute_goes_command("ST FELYSIA_", context);
    ok &= require(answer.action == GoesResultAction::set_local_target && answer.target
                      && answer.target->planet_index == 3, "local ST action mismatch");

    answer = execute_goes_command("CAST FELYSIA:NATIVE COMMAND NOTE_", context);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::catalog_changed,
                  "CAST failed");
    answer = execute_goes_command("CAT FELYSIA_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "NATIVE COMMAND NOTE"),
                  "CAT did not show appended note");
    answer = execute_goes_command("DL FELYSIA_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "(239 NOTES)"),
                  "DL did not count the selected planet note");
    answer = execute_goes_command("REP FELYSIA:239:REPLACED COMMAND NOTE_", context);
    ok &= require(answer.status == GoesResultStatus::ok, "REP failed on mutable record");
    answer = execute_goes_command("CAT FELYSIA:239..239_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "REPLACED COMMAND NOTE"),
                  "REP did not survive reopen");
    answer = execute_goes_command("DELE FELYSIA:239_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "REMOVED: 1"), "DELE failed");
    answer = execute_goes_command("CAT FELYSIA:239..239_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "THERE WERE NO RECORDS"),
                  "deleted note remained visible");

    const auto protected_before = read_bytes(guide_copy);
    answer = execute_goes_command("REP FELYSIA:1:PROTECTED CHANGE_", context);
    ok &= require(answer.status == GoesResultStatus::rejected
                      && read_bytes(guide_copy) == protected_before,
                  "protected guide replacement changed the catalog");
    answer = execute_goes_command("DELE FELYSIA:1_", context);
    ok &= require(answer.status == GoesResultStatus::rejected
                      && read_bytes(guide_copy) == protected_before,
                  "protected guide deletion changed the catalog");

    auto missing_context = context;
    missing_context.starmap_path = guide_copy.string() + ".missing";
    answer = execute_goes_command("PAR MIRACLE_", missing_context);
    ok &= require(answer.status == GoesResultStatus::unavailable, "missing starmap outcome mismatch");
    const auto corrupt_path = guide_copy.string() + ".corrupt";
    { std::ofstream output(corrupt_path, std::ios::binary); output.write("BAD", 3); }
    auto corrupt_context = context;
    corrupt_context.starmap_path = corrupt_path;
    answer = execute_goes_command("PAR MIRACLE_", corrupt_context);
    ok &= require(answer.status == GoesResultStatus::corrupt_data, "truncated starmap outcome mismatch");

    const auto blocked_copy = guide_copy.string() + ".blocked";
    std::filesystem::copy_file(guide_copy, blocked_copy, std::filesystem::copy_options::overwrite_existing);
    const auto blocked_before = read_bytes(blocked_copy);
    std::filesystem::create_directory(blocked_copy + ".tmp");
    auto blocked_context = context;
    blocked_context.guide_path = blocked_copy;
    answer = execute_goes_command("CAST FELYSIA:SHOULD NOT COMMIT_", blocked_context);
    ok &= require(answer.status == GoesResultStatus::write_failed
                      && read_bytes(blocked_copy) == blocked_before,
                  "failed guide replacement partially changed the catalog");

    const auto readonly_copy = guide_copy.string() + ".readonly";
    std::filesystem::copy_file(guide_copy, readonly_copy, std::filesystem::copy_options::overwrite_existing);
    const auto readonly_before = read_bytes(readonly_copy);
    std::filesystem::permissions(readonly_copy, std::filesystem::perms::owner_read,
                                 std::filesystem::perm_options::replace);
    auto readonly_context = context;
    readonly_context.guide_path = readonly_copy;
    answer = execute_goes_command("CAST FELYSIA:SHOULD NOT COMMIT_", readonly_context);
    ok &= require(answer.status == GoesResultStatus::write_failed
                      && read_bytes(readonly_copy) == readonly_before,
                  "read-only guide was changed");
    std::filesystem::permissions(readonly_copy, std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::replace);

    answer = execute_goes_command("CLEAN_", context);
    ok &= require(answer.status == GoesResultStatus::unsupported && contains(answer, "LEGACY TOOL RETIRED"),
                  "obsolete command disposition mismatch");
    answer = execute_goes_command("WARP MIRACLE_", context);
    ok &= require(answer.status == GoesResultStatus::unsupported && contains(answer, "UNKNOWN MODULE"),
                  "unknown command mismatch");

    // Flight log commands
    answer = execute_goes_command("LOG_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "CAPTAIN'S FLIGHT LOG"),
                  "LOG summary mismatch");
    answer = execute_goes_command("JOURNAL_", context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "CAPTAIN'S FLIGHT LOG"),
                  "JOURNAL alias mismatch");
    answer = execute_goes_command("LOG EXPORT_", context);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::export_created
                      && contains(answer, "FLIGHT LOG EXPORTED"),
                  "LOG EXPORT mismatch");
    const auto export_dir = std::filesystem::path(argv[1]).parent_path();
    ok &= require(std::filesystem::exists(export_dir / "flight_log.md"), "flight_log.md was exported");
    ok &= require(std::filesystem::exists(export_dir / "flight_log.json"), "flight_log.json was exported");

    // Naming commands on a disposable starmap copy
    const auto starmap_copy = guide_copy.parent_path() / "disposable-starmap.bin";
    std::filesystem::copy_file(argv[1], starmap_copy, std::filesystem::copy_options::overwrite_existing);
    auto mutable_context = context;
    mutable_context.starmap_path = starmap_copy;
    mutable_context.local_star_x = 100000;
    mutable_context.local_star_x = 318928;
    mutable_context.local_star_y = 100216574;
    mutable_context.local_star_z = -33444;

    // Name unnamed star
    answer = execute_goes_command("NAME CELESTIA_", mutable_context);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::catalog_changed
                      && contains(answer, "OBJECT LABELED") && contains(answer, "CELESTIA"),
                  "NAME star failed");
    // Duplicate star name attempt rejected
    answer = execute_goes_command("NAME CELESTIA2_", mutable_context);
    ok &= require(answer.status == GoesResultStatus::rejected && contains(answer, "STAR ALREADY LABELED"),
                  "duplicate star label was not rejected");
    // Name planet body on system with bodies (318928, 100216574, -33444)
    answer = execute_goes_command("NAME P1:AERIA_", mutable_context);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::catalog_changed
                      && contains(answer, "BODY #1:") && contains(answer, "AERIA"),
                  "NAME planet body failed");
    // Name planet body with space-separated syntax
    answer = execute_goes_command("NAME P2 BOREAS_", mutable_context);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::catalog_changed
                      && contains(answer, "BODY #2:") && contains(answer, "BOREAS"),
                  "NAME planet body with space separator failed");
    // Navigate back to newly named star from another location
    auto remote_observer = mutable_context;
    remote_observer.observer_x = 318928 + 500000;
    remote_observer.observer_y = 100216574 - 200000;
    remote_observer.observer_z = -33444 + 800000;
    answer = execute_goes_command("ST CELESTIA_", remote_observer);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::set_remote_target
                      && answer.target.has_value() && contains(answer, "STARTING VIMANA DRIVE"),
                  "ST to newly named star failed");
    // Duplicate body name attempt with LABEL alias rejected
    answer = execute_goes_command("LABEL P1:AERIA2_", mutable_context);
    ok &= require(answer.status == GoesResultStatus::rejected && contains(answer, "BODY ALREADY LABELED"),
                  "duplicate body label via LABEL alias was not rejected");
    // Renaming an extant object rejected
    answer = execute_goes_command("NAME FELYSIA:RENAMED_", mutable_context);
    ok &= require(answer.status == GoesResultStatus::rejected && contains(answer, "OBJECT IS LABELED"),
                  "renaming extant object was not rejected");

    // Bookmarks and Waypoint navigation (BM)
    auto bm_context = mutable_context;
    bm_context.bookmarks_path = export_dir / "bookmarks_test.ini";
    bm_context.current_star_id = 9999.0;
    bm_context.current_star_name = "OUTPOST STAR";
    bm_context.current_star_class = 2;
    bm_context.local_star_x = 100000.0;
    bm_context.local_star_y = 200000.0;
    bm_context.local_star_z = 300000.0;

    // 1. BM with empty list
    answer = execute_goes_command("BM_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "WAYPOINT BOOKMARKS"),
                  "BM empty list failed");

    // 2. BM ADD with label
    answer = execute_goes_command("BM ADD BASE ALPHA_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BOOKMARK RECORDED")
                      && contains(answer, "BASE ALPHA"),
                  "BM ADD failed");

    // 3. BM listing shows added bookmark
    answer = execute_goes_command("BM_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BASE ALPHA"),
                  "BM listing did not show bookmark");

    // 4. BM GOTO targets the waypoint
    auto distant_context = bm_context;
    distant_context.observer_x = 900000.0;
    distant_context.observer_y = 900000.0;
    distant_context.observer_z = 900000.0;
    answer = execute_goes_command("BM GOTO 1_", distant_context);
    ok &= require(answer.status == GoesResultStatus::ok && answer.action == GoesResultAction::set_remote_target
                      && answer.target.has_value() && contains(answer, "WAYPOINT LOCK ON"),
                  "BM GOTO failed");

    // 5. BOOKMARK and WAYPOINT aliases
    answer = execute_goes_command("BOOKMARK_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BOOKMARKS"),
                  "BOOKMARK alias failed");
    answer = execute_goes_command("WAYPOINT_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BOOKMARKS"),
                  "WAYPOINT alias failed");

    // 6. BM DEL removes bookmark
    answer = execute_goes_command("BM DEL 1_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BOOKMARK DELETED"),
                  "BM DEL failed");

    // 7. BM CLEAR
    execute_goes_command("BM ADD TEST BM_", bm_context);
    answer = execute_goes_command("BM CLEAR_", bm_context);
    ok &= require(answer.status == GoesResultStatus::ok && contains(answer, "BOOKMARKS CLEARED"),
                  "BM CLEAR failed");

    std::error_code ignored;
    std::filesystem::remove(starmap_copy, ignored);
    std::filesystem::remove(export_dir / "flight_log.md", ignored);
    std::filesystem::remove(export_dir / "flight_log.json", ignored);
    std::filesystem::remove(export_dir / "bookmarks_test.ini", ignored);
    std::filesystem::remove(guide_copy, ignored);
    std::filesystem::remove(argv[4], ignored);
    std::filesystem::remove(corrupt_path, ignored);
    std::filesystem::remove_all(blocked_copy + ".tmp", ignored);
    std::filesystem::remove(blocked_copy, ignored);
    std::filesystem::remove(readonly_copy, ignored);
    return ok ? 0 : 1;
}
