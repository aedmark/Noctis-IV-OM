#include "goesnet_data.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::fprintf(stderr, "GOESnet data: %s\n", message);
    return condition;
}
}

int main(int argc, char **argv) {
    using namespace noctis;
    if (argc != 5) {
        std::fputs("usage: goesnet_data_test STARMAP GUIDE TEMP_STARMAP TEMP_GUIDE\n", stderr);
        return 2;
    }
    bool ok = true;
    StarmapData starmap;
    auto result = load_starmap(argv[1], starmap);
    if (result.status != GoesDataStatus::ok) std::fprintf(stderr, "GOESnet data: %s\n", result.message.c_str());
    ok &= require(result.status == GoesDataStatus::ok && !starmap.records.empty(), "pinned starmap did not load");
    if (result.status != GoesDataStatus::ok) return 1;
    const auto miracle = find_starmap_objects(starmap, "MIRACLE");
    ok &= require(miracle.size() == 1 && starmap.records[miracle[0]].kind == GoesObjectKind::star
                      && std::abs(starmap.records[miracle[0]].id - 2015.3586769998592) < goes_id_tolerance,
                  "MIRACLE lookup mismatch");
    const auto felysia = find_starmap_objects(starmap, "FELYSIA");
    ok &= require(felysia.size() == 1 && starmap.records[felysia[0]].kind == GoesObjectKind::planet
                      && starmap.records[felysia[0]].ordinal == 4,
                  "exact FELYSIA lookup did not outrank prefixes");

    GuideData guide;
    result = load_guide(argv[2], guide);
    ok &= require(result.status == GoesDataStatus::ok && !guide.records.empty(), "pinned guide did not load");
    ok &= require(guide.consolidated_size == std::filesystem::file_size(argv[2]), "pinned guide boundary mismatch");

    const std::filesystem::path starmap_copy(argv[3]);
    std::filesystem::copy_file(argv[1], starmap_copy, std::filesystem::copy_options::overwrite_existing);
    const auto original_starmap_size = std::filesystem::file_size(starmap_copy);
    std::int32_t label_offset = -1;
    result = append_starmap_label(starmap_copy, 2015.3586769998592, "WORKFLOW STAR",
                                  GoesObjectKind::star, 0, label_offset);
    ok &= require(result.status == GoesDataStatus::ok
                      && label_offset == static_cast<std::int32_t>(original_starmap_size),
                  "starmap label append failed");
    StarmapData changed_starmap;
    result = load_starmap(starmap_copy, changed_starmap);
    auto workflow_star = find_starmap_objects(changed_starmap, "WORKFLOW STAR");
    ok &= require(result.status == GoesDataStatus::ok && workflow_star.size() == 1
                      && changed_starmap.records[workflow_star[0]].id == 2015.3586769998592
                      && !changed_starmap.records[workflow_star[0]].protected_record,
                  "starmap label did not survive reopen");
    std::int32_t duplicate_offset = -1;
    result = append_starmap_label(starmap_copy, 2015.3586769998592, "WORKFLOW STAR",
                                  GoesObjectKind::star, 0, duplicate_offset);
    ok &= require(result.status == GoesDataStatus::rejected
                      && std::filesystem::file_size(starmap_copy) == original_starmap_size + 32,
                  "duplicate starmap label changed the file");
    result = remove_starmap_label(starmap_copy, 4);
    ok &= require(result.status == GoesDataStatus::rejected,
                  "protected starmap label removal was accepted");
    result = remove_starmap_label(starmap_copy, label_offset);
    ok &= require(result.status == GoesDataStatus::ok, "starmap label removal failed");
    result = load_starmap(starmap_copy, changed_starmap);
    ok &= require(result.status == GoesDataStatus::ok
                      && find_starmap_objects(changed_starmap, "WORKFLOW STAR").empty()
                      && std::filesystem::file_size(starmap_copy) == original_starmap_size + 32,
                  "removed starmap label remained visible or truncated the file");

    const std::filesystem::path temporary(argv[4]);
    std::filesystem::copy_file(argv[2], temporary, std::filesystem::copy_options::overwrite_existing);
    const double subject = starmap.records[felysia[0]].id;
    const auto original = guide_records_for(guide, subject).size();
    result = append_guide_record(temporary, subject, "NATIVE TEST NOTE");
    ok &= require(result.status == GoesDataStatus::ok, "guide append failed");
    GuideData changed;
    result = load_guide(temporary, changed);
    auto matches = guide_records_for(changed, subject);
    ok &= require(result.status == GoesDataStatus::ok && matches.size() == original + 1
                      && changed.records[matches.back()].message == "NATIVE TEST NOTE"
                      && !changed.records[matches.back()].protected_record,
                  "appended note did not survive reopen");
    result = replace_guide_record(temporary, subject, original + 1, "REPLACED TEST NOTE");
    ok &= require(result.status == GoesDataStatus::ok, "guide replacement failed");
    std::size_t removed = 0, protected_records = 0;
    result = delete_guide_records(temporary, subject, 1, original + 1, removed, protected_records);
    ok &= require(result.status == GoesDataStatus::ok && removed == 1 && protected_records == original,
                  "guide deletion/protection accounting mismatch");
    result = load_guide(temporary, changed);
    ok &= require(result.status == GoesDataStatus::ok && guide_records_for(changed, subject).size() == original,
                  "deleted record survived reopen");
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    std::filesystem::remove(starmap_copy, ignored);

    const auto corrupt = temporary.string() + ".bad";
    { std::ofstream output(corrupt, std::ios::binary); output.write("BAD", 3); }
    result = load_guide(corrupt, changed);
    ok &= require(result.status == GoesDataStatus::corrupt, "truncated guide was accepted");
    std::filesystem::remove(corrupt, ignored);
    return ok ? 0 : 1;
}
