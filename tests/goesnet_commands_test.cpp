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

    std::error_code ignored;
    std::filesystem::remove(guide_copy, ignored);
    std::filesystem::remove(argv[4], ignored);
    std::filesystem::remove(corrupt_path, ignored);
    std::filesystem::remove_all(blocked_copy + ".tmp", ignored);
    std::filesystem::remove(blocked_copy, ignored);
    std::filesystem::remove(readonly_copy, ignored);
    return ok ? 0 : 1;
}
