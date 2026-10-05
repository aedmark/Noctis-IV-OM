#include "starmap_exchange.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "Starmap exchange test failed: %s\n", message);
    }
    return condition;
}

} // namespace

int main(int argc, char **argv) {
    using namespace noctis;

    if (argc != 5) {
        std::fputs("usage: starmap_exchange_test STARMAP GUIDE TEMP_STARMAP TEMP_GUIDE\n", stderr);
        return 2;
    }

    const std::filesystem::path seed_starmap(argv[1]);
    const std::filesystem::path seed_guide(argv[2]);
    const std::filesystem::path test_sm(argv[3]);
    const std::filesystem::path test_gd(argv[4]);
    const auto temp_dir = test_sm.parent_path() / "exchange_temp";

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir, ec);

    // Setup working copy of base starmap and guide
    std::filesystem::copy_file(seed_starmap, test_sm, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(seed_guide, test_gd, std::filesystem::copy_options::overwrite_existing);

    bool ok = true;

    // =========================================================================
    // 1. Syntax Validation Checks (in isolation)
    // =========================================================================
    {
        StarmapExchangeRecord valid_star{12345.678, "NEW EDMARK", GoesObjectKind::star, 0};
        auto res = validate_starmap_record_syntax(valid_star);
        ok &= require(res.kind == StarmapConflictKind::none, "Valid star failed syntax check");

        StarmapExchangeRecord valid_planet{12346.678, "NEW EDMARK I", GoesObjectKind::planet, 1};
        res = validate_starmap_record_syntax(valid_planet);
        ok &= require(res.kind == StarmapConflictKind::none, "Valid planet failed syntax check");

        // NaN ID
        StarmapExchangeRecord nan_rec{std::numeric_limits<double>::quiet_NaN(), "NAN STAR", GoesObjectKind::star, 0};
        res = validate_starmap_record_syntax(nan_rec);
        ok &= require(res.kind == StarmapConflictKind::invalid_id, "NaN ID was not rejected");

        // Infinity ID
        StarmapExchangeRecord inf_rec{std::numeric_limits<double>::infinity(), "INF STAR", GoesObjectKind::star, 0};
        res = validate_starmap_record_syntax(inf_rec);
        ok &= require(res.kind == StarmapConflictKind::invalid_id, "Inf ID was not rejected");

        // Zero ID
        StarmapExchangeRecord zero_rec{0.0, "ZERO STAR", GoesObjectKind::star, 0};
        res = validate_starmap_record_syntax(zero_rec);
        ok &= require(res.kind == StarmapConflictKind::invalid_id, "Zero ID was not rejected");

        // Galactic bounds overflow
        StarmapExchangeRecord huge_rec{1.0e16, "HUGE STAR", GoesObjectKind::star, 0};
        res = validate_starmap_record_syntax(huge_rec);
        ok &= require(res.kind == StarmapConflictKind::invalid_id, "Out of galactic bounds ID was not rejected");

        // Empty / whitespace name
        StarmapExchangeRecord empty_name{12345.0, "   ", GoesObjectKind::star, 0};
        res = validate_starmap_record_syntax(empty_name);
        ok &= require(res.kind == StarmapConflictKind::invalid_name, "Empty name was not rejected");

        // Overlong name (> 20 chars)
        StarmapExchangeRecord long_name{12345.0, "THIS NAME IS DEFINITELY LONGER THAN TWENTY CHARACTERS", GoesObjectKind::star, 0};
        res = validate_starmap_record_syntax(long_name);
        ok &= require(res.kind == StarmapConflictKind::invalid_name, "Overlong name was not rejected");

        // Forbidden characters: $, &, <, >
        for (const char *bad_char_name : {"STAR$ONE", "STAR&TWO", "STAR<THREE>", "BAD\"QUOTE"}) {
            StarmapExchangeRecord bad_char_rec{12345.0, bad_char_name, GoesObjectKind::star, 0};
            res = validate_starmap_record_syntax(bad_char_rec);
            ok &= require(res.kind == StarmapConflictKind::invalid_name, "Invalid character was not rejected");
        }

        // Invalid planet ordinal (< 1 or > 99)
        StarmapExchangeRecord bad_planet_ord{12345.0, "BAD PLANET", GoesObjectKind::planet, 0};
        res = validate_starmap_record_syntax(bad_planet_ord);
        ok &= require(res.kind == StarmapConflictKind::invalid_kind_ordinal, "Planet ordinal 0 was not rejected");

        StarmapExchangeRecord bad_planet_ord2{12345.0, "BAD PLANET 100", GoesObjectKind::planet, 100};
        res = validate_starmap_record_syntax(bad_planet_ord2);
        ok &= require(res.kind == StarmapConflictKind::invalid_kind_ordinal, "Planet ordinal 100 was not rejected");
    }

    // =========================================================================
    // 2. Semantic & Conflict Validation Checks
    // =========================================================================
    {
        StarmapData base_data;
        load_starmap(test_sm, base_data);

        // Find a protected seed record (e.g. FENIA or first record)
        const auto &seed_rec = base_data.records.front();

        // Test attempt to rename protected canonical seed ID
        StarmapExchangeRecord overwrite_seed{seed_rec.id, "MY OWN STAR", seed_rec.kind, seed_rec.ordinal};
        auto res = validate_starmap_record_semantics(overwrite_seed, base_data, {});
        ok &= require(res.kind == StarmapConflictKind::protected_id_conflict,
                      "Protected ID overwrite attempt was not blocked");

        // Test attempt to reuse protected canonical seed name on another star
        StarmapExchangeRecord reuse_seed_name{seed_rec.id + 99999.0, seed_rec.name, seed_rec.kind, seed_rec.ordinal};
        res = validate_starmap_record_semantics(reuse_seed_name, base_data, {});
        ok &= require(res.kind == StarmapConflictKind::protected_name_collision,
                      "Protected seed name collision attempt was not blocked");

        // Test duplicate of existing seed record (benign duplicate)
        StarmapExchangeRecord exact_seed_dupe{seed_rec.id, seed_rec.name, seed_rec.kind, seed_rec.ordinal};
        res = validate_starmap_record_semantics(exact_seed_dupe, base_data, {});
        ok &= require(res.kind == StarmapConflictKind::duplicate_record,
                      "Exact duplicate of seed record was not classified as duplicate");

        // Add a local player record
        std::int32_t offset = -1;
        append_starmap_label(test_sm, 77777.0, "LOCAL STAR", GoesObjectKind::star, 1, offset);
        load_starmap(test_sm, base_data);

        // Test ID collision with local player record (same ID, different name)
        StarmapExchangeRecord local_id_coll{77777.0, "INCOMING CONFLICT", GoesObjectKind::star, 1};
        res = validate_starmap_record_semantics(local_id_coll, base_data, {});
        ok &= require(res.kind == StarmapConflictKind::id_conflict,
                      "Local ID conflict was not detected");

        // Test Name collision with local player record (same name, different ID)
        StarmapExchangeRecord local_name_coll{88888.0, "LOCAL STAR", GoesObjectKind::star, 1};
        res = validate_starmap_record_semantics(local_name_coll, base_data, {});
        ok &= require(res.kind == StarmapConflictKind::name_collision,
                      "Local name collision was not detected");

        // Test Intra-packet collision (batch conflicts)
        std::vector<StarmapExchangeRecord> batch{
            {90001.0, "BATCH STAR A", GoesObjectKind::star, 0}
        };
        StarmapExchangeRecord batch_coll{90001.0, "BATCH STAR B", GoesObjectKind::star, 0};
        res = validate_starmap_record_semantics(batch_coll, base_data, batch);
        ok &= require(res.kind == StarmapConflictKind::internal_batch_conflict,
                      "Intra-packet ID conflict was not detected");

        StarmapExchangeRecord batch_name_coll{90002.0, "BATCH STAR A", GoesObjectKind::star, 0};
        res = validate_starmap_record_semantics(batch_name_coll, base_data, batch);
        ok &= require(res.kind == StarmapConflictKind::internal_batch_conflict,
                      "Intra-packet name collision was not detected");
    }

    // =========================================================================
    // 3. Binary NSM Packet Export & Import Round-trip
    // =========================================================================
    {
        // Add custom stars and planets to source profile
        const auto src_sm = temp_dir / "src_starmap.bin";
        const auto src_gd = temp_dir / "src_guide.bin";
        std::filesystem::copy_file(seed_starmap, src_sm, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(seed_guide, src_gd, std::filesystem::copy_options::overwrite_existing);

        std::int32_t off1 = -1, off2 = -1;
        append_starmap_label(src_sm, 111111.0, "EXPLORER STAR", GoesObjectKind::star, 3, off1);
        append_starmap_label(src_sm, 111112.0, "EXPLORER PRIME", GoesObjectKind::planet, 1, off2);
        append_guide_record(src_gd, 111111.0, "FIRST STAR SURVEY NOTE BY EXPLORER");

        // Export packet
        const auto out_packet = temp_dir / "share.nsm";
        const auto exp_report = export_starmap_packet(src_sm, src_gd, out_packet, StarmapPacketFormat::binary_nsm, "ALICE");
        ok &= require(exp_report.status == StarmapExchangeStatus::ok, "Export packet failed");
        ok &= require(exp_report.records_exported == 2, "Export record count != 2");
        ok &= require(exp_report.guide_notes_exported == 1, "Export guide count != 1");
        ok &= require(std::filesystem::exists(out_packet), "Export packet file does not exist");

        // Test corruption / truncation detection
        {
            const auto corrupt_file = temp_dir / "corrupted.nsm";
            std::filesystem::copy_file(out_packet, corrupt_file, std::filesystem::copy_options::overwrite_existing);

            // Truncate by 5 bytes
            std::filesystem::resize_file(corrupt_file, std::filesystem::file_size(corrupt_file) - 5);
            auto imp_rep = import_starmap_packet(test_sm, test_gd, corrupt_file);
            ok &= require(imp_rep.status == StarmapExchangeStatus::payload_corrupted,
                          "Truncated packet was not detected as corrupt");

            // Tamper 1 byte (checksum mismatch)
            std::filesystem::copy_file(out_packet, corrupt_file, std::filesystem::copy_options::overwrite_existing);
            std::fstream f(corrupt_file, std::ios::in | std::ios::out | std::ios::binary);
            f.seekp(30);
            f.put('Z');
            f.close();
            imp_rep = import_starmap_packet(test_sm, test_gd, corrupt_file);
            ok &= require(imp_rep.status == StarmapExchangeStatus::checksum_mismatch,
                          "Tampered packet was not detected via CRC32 mismatch");
        }

        // Target profile import
        const auto dest_sm = temp_dir / "dest_starmap.bin";
        const auto dest_gd = temp_dir / "dest_guide.bin";
        std::filesystem::copy_file(seed_starmap, dest_sm, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(seed_guide, dest_gd, std::filesystem::copy_options::overwrite_existing);

        // Dry-run import
        StarmapImportOptions dry_opts;
        dry_opts.dry_run = true;
        auto dry_rep = import_starmap_packet(dest_sm, dest_gd, out_packet, dry_opts);
        ok &= require(dry_rep.status == StarmapExchangeStatus::ok, "Dry run import failed");
        ok &= require(dry_rep.records_scanned == 2, "Dry run scanned != 2");
        ok &= require(dry_rep.records_imported == 2, "Dry run imported != 2");

        // Verify dest_sm was untouched by dry run
        StarmapData dest_check;
        load_starmap(dest_sm, dest_check);
        ok &= require(find_starmap_objects(dest_check, "EXPLORER STAR").empty(),
                      "Dry run mutated target starmap file on disk!");

        // Live import
        StarmapImportOptions live_opts;
        live_opts.dry_run = false;
        auto live_rep = import_starmap_packet(dest_sm, dest_gd, out_packet, live_opts);
        ok &= require(live_rep.status == StarmapExchangeStatus::ok, "Live import failed");
        ok &= require(live_rep.records_imported == 2, "Live records_imported != 2");
        ok &= require(live_rep.guide_notes_imported == 1, "Live guide_notes_imported != 1");

        // Verify imported records exist in target catalog
        load_starmap(dest_sm, dest_check);
        const auto star_matches = find_starmap_objects(dest_check, "EXPLORER STAR");
        ok &= require(star_matches.size() == 1, "Imported star not found in destination catalog");
        const auto planet_matches = find_starmap_objects(dest_check, "EXPLORER PRIME");
        ok &= require(planet_matches.size() == 1, "Imported planet not found in destination catalog");

        // Verify re-importing the same packet treats them as duplicates
        auto dupe_rep = import_starmap_packet(dest_sm, dest_gd, out_packet, live_opts);
        ok &= require(dupe_rep.status == StarmapExchangeStatus::ok, "Duplicate import failed");
        ok &= require(dupe_rep.records_imported == 0, "Duplicate records were incorrectly imported again");
        ok &= require(dupe_rep.duplicates_skipped == 2, "Duplicates were not counted as skipped");
    }

    // =========================================================================
    // 4. JSON Packet Export & Import
    // =========================================================================
    {
        const auto json_src_sm = temp_dir / "json_src_sm.bin";
        const auto json_src_gd = temp_dir / "json_src_gd.bin";
        std::filesystem::copy_file(seed_starmap, json_src_sm, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(seed_guide, json_src_gd, std::filesystem::copy_options::overwrite_existing);

        std::int32_t off = -1;
        append_starmap_label(json_src_sm, 333333.0, "JSON STAR", GoesObjectKind::star, 4, off);
        append_guide_record(json_src_gd, 333333.0, "NOTE EXPORTED AS JSON");

        const auto json_packet = temp_dir / "packet.json";
        const auto exp_rep = export_starmap_packet(json_src_sm, json_src_gd, json_packet, StarmapPacketFormat::json, "BOB");
        ok &= require(exp_rep.status == StarmapExchangeStatus::ok, "JSON export failed");
        ok &= require(std::filesystem::exists(json_packet), "JSON packet file was not written");

        // Import into clean dest
        const auto json_dest_sm = temp_dir / "json_dest_sm.bin";
        const auto json_dest_gd = temp_dir / "json_dest_gd.bin";
        std::filesystem::copy_file(seed_starmap, json_dest_sm, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(seed_guide, json_dest_gd, std::filesystem::copy_options::overwrite_existing);

        const auto imp_rep = import_starmap_packet(json_dest_sm, json_dest_gd, json_packet);
        ok &= require(imp_rep.status == StarmapExchangeStatus::ok, "JSON import failed");
        ok &= require(imp_rep.records_imported == 1, "JSON imported count != 1");
        ok &= require(imp_rep.guide_notes_imported == 1, "JSON guide notes count != 1");

        StarmapData check;
        load_starmap(json_dest_sm, check);
        ok &= require(!find_starmap_objects(check, "JSON STAR").empty(), "Imported JSON star not found in catalog");
    }

    // =========================================================================
    // 5. Starmap Compaction / Clean Checks
    // =========================================================================
    {
        const auto comp_sm = temp_dir / "compact_sm.bin";
        std::filesystem::copy_file(seed_starmap, comp_sm, std::filesystem::copy_options::overwrite_existing);

        std::int32_t off1 = -1, off2 = -1;
        append_starmap_label(comp_sm, 444444.0, "TEMP STAR 1", GoesObjectKind::star, 0, off1);
        append_starmap_label(comp_sm, 555555.0, "TEMP STAR 2", GoesObjectKind::star, 0, off2);

        // Remove one label (creates a tombstone)
        const auto rem_res = remove_starmap_label(comp_sm, off1);
        ok &= require(rem_res.status == GoesDataStatus::ok, "Remove label failed");

        // Run compact_starmap
        std::size_t compacted_count = 0;
        const auto comp_res = compact_starmap(comp_sm, compacted_count);
        ok &= require(comp_res.status == GoesDataStatus::ok, "Compacting starmap failed");
        ok &= require(compacted_count == 1, "Compacted count != 1");

        // Verify remaining label is present and clean
        StarmapData comp_check;
        load_starmap(comp_sm, comp_check);
        ok &= require(find_starmap_objects(comp_check, "TEMP STAR 2").size() == 1,
                      "Preserved star missing after compaction");
        ok &= require(find_starmap_objects(comp_check, "TEMP STAR 1").empty(),
                      "Removed star reappeared after compaction");

        // Running compact again should report 0 compacted
        std::size_t comp_second = 0;
        compact_starmap(comp_sm, comp_second);
        ok &= require(comp_second == 0, "Second compaction found tombstones unexpectedly");
    }

    // Cleanup temp directory
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::remove(test_sm, ec);
    std::filesystem::remove(test_gd, ec);

    if (ok) {
        std::puts("All starmap exchange validation and sharing tests passed successfully.");
        return 0;
    }
    return 1;
}
