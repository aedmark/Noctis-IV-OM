# M6-W04 starmap and guide content

Status: complete on Linux. Windows package verification remains deferred by
ADR-0009.

## Shipped content

The native build uses the Noctis IV Plus October 2023 `STARMAP.BIN` and
`GUIDE.BIN` introduced by repository commit `3759857`. Their release identity,
byte sizes, record geometry, record counts, and consolidated boundaries are
recorded once in `data/CONTENT_MANIFEST.json`. This is the accepted P22 content
version; runtime catalogs may subsequently gain player labels and guide notes.

The build initializes a missing runtime catalog from these seeds and never
overwrites an existing file. This separation is deliberate: files under the
repository's `data/` directory are immutable release inputs, while files next
to a built executable are writable player state.

## Verification and staging

From `modern/`, run:

```sh
cmake --build --preset linux-clang-debug --target verify-content
python3 tools/verify_content.py \
  --manifest ../data/CONTENT_MANIFEST.json --source ../data --stage build/package-data
```

The verifier checks both SHA-256 digests before the record geometry, record
count, and consolidated boundary. `--stage` copies only missing files and
reports existing catalogs as preserved. It never provides an overwrite mode.
The normal post-build initializer follows the same missing-only rule.

`content_package` tests the release manifest, a clean staging operation, exact
staged bytes, and a second staging pass with a deliberately changed guide. The
changed guide must survive unchanged. The existing native parser/query and
application fixtures remain the semantic evidence for the content itself,
including DOS-confirmed MIRACLE and FELYSIA results and P15 note counts.

## Provenance and scope

Commit `3759857` identifies the files as the 2023-10 Department of
Astrocartography update. Distribution is governed by the project clearance and
credit conditions in ADR-0007 and `PROVENANCE.md`; the package does not include
the historical DOS maintenance executables. `CLEAN`, `INBOX`, and `OUTBOX`
remain explicitly retired because native validated readers and atomic catalog
updates replace their unsafe file-exchange role.

This pass verifies the accepted archive as a whole; it does not claim a manual
authorship audit of every community catalog entry. Player-modified runtime
catalogs are structurally validated by the native readers but are not expected
to retain the release checksums.
