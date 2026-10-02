# M4-W05 Surface Life, Ruins, and Objects

M4-W05 restores the populated layer of surface exploration on top of the
environment paths protected by M4-W04. The headless `surface_content_fixtures`
test renders three production exploration frames, fingerprints the final
320×200 indexed frame, and records calls through each selected content path.

Two native scenes provide the representative split:

| World | Exact paths observed across three frames |
| --- | --- |
| FELYSIA `LQ 001:060` | 5,838 trees, 12 animals, 81 ruin-textured terrain fragments, and 3 capsule draws |
| rocky P02 `LQ 001:060` | 5,940 loose-rock details and 3 capsule draws |

The counts are draw-path invocations, not a claim that thousands of whole
objects are simultaneously visible. Surface traversal visits many terrain
fragments and detail levels per frame. Their purpose is to prove that the
complete-frame hashes really include each named production category.

The animal and capsule paths exposed two assumptions inherited from the packed
DOS model format. Polygon coordinate arrays could begin at byte addresses that
are legal on x86 but not aligned for a C++ `float`, and base-color adjustment
iterated four times beyond the one-color-per-polygon table. The loader now
reads packed archive sections into aligned native regions and bounds color
adjustment to the actual polygon count. The capsule's 32 KiB texture alias also
uses an explicit 15-bit address mask.

Clang, GCC, and Clang ASan/UBSan agree on both counts and both complete frame
hashes. These remain exact native regression fixtures. The F02C DOS evidence
confirms FELYSIA's vegetated appearance and large historical ruins, but no DOS
framebuffer or object-call trace was extracted for comparison.
