#!/usr/bin/env python3
"""Create a disposable DOS save positioned in FELYSIA's parent system."""

import argparse
import hashlib
import json
import struct
from pathlib import Path


PARENT_XYZ = (-18928.0, -29680.0, -67336.0)
STAR_RADIUS = 5.020999908447266


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--planetary-map-pose",
        action="store_true",
        help="place the player beside the far-right planetary-map screen",
    )
    parser.add_argument("source", type=Path, help="tracked 378-byte GOESnet seed")
    parser.add_argument("destination", type=Path, help="new disposable CURRENT.BIN")
    args = parser.parse_args()

    if args.source.resolve() == args.destination.resolve():
        parser.error("source and destination must differ")
    data = bytearray(args.source.read_bytes())
    if len(data) != 378:
        parser.error(f"expected a 378-byte NIV+ save, got {len(data)} bytes")

    before = hashlib.sha256(data).hexdigest()
    struct.pack_into("<b", data, 0, 1)       # fixed-point chase
    struct.pack_into("<b", data, 8, 0)       # not selecting remote target
    struct.pack_into("<b", data, 9, 1)       # parent star is selected
    struct.pack_into("<b", data, 10, 0)      # not selecting local target
    struct.pack_into("<b", data, 11, 3)      # P04 / FELYSIA
    struct.pack_into("<b", data, 12, 0)      # fine approach inactive
    struct.pack_into("<b", data, 13, 1)      # local target reached
    struct.pack_into("<bbbb", data, 14, 0, 63, 58, 40)
    struct.pack_into("<bbbb", data, 18, 0, 63, 58, 40)
    struct.pack_into("<h", data, 25, 2)      # onboard devices
    struct.pack_into("<h", data, 27, 20000)  # full current charge interval
    struct.pack_into("<h", data, 29, 2)      # miscellaneous/data page
    struct.pack_into("<h", data, 31, 0)      # class-0 parent star
    struct.pack_into("<h", data, 35, 0)
    struct.pack_into("<h", data, 37, 5)      # regenerated on load; documentary
    if args.planetary_map_pose:
        struct.pack_into("<fffff", data, 39, 3000.0, 0.0, -5600.0, 0.0, -90.0)
    else:
        struct.pack_into("<fffff", data, 39, 0.0, 0.0, -500.0, 0.0, 0.0)
    struct.pack_into("<f", data, 63, STAR_RADIUS)
    struct.pack_into("<f", data, 67, STAR_RADIUS)
    for offset, value in zip((71, 79, 87), (PARENT_XYZ[0] + 1000.0, PARENT_XYZ[1], PARENT_XYZ[2])):
        struct.pack_into("<d", data, offset, value)
    for offsets in ((95, 103, 111), (119, 127, 135)):
        for offset, value in zip(offsets, PARENT_XYZ):
            struct.pack_into("<d", data, offset, value)
    struct.pack_into("<b", data, 232, 1)     # inside parent system
    struct.pack_into("<b", data, 243, 0)     # data panel initially closed

    with args.destination.open("xb") as output:
        output.write(data)
    print(json.dumps({
        "source_sha256": before,
        "destination_sha256": hashlib.sha256(data).hexdigest(),
        "parent_xyz": PARENT_XYZ,
        "star_radius_f32": STAR_RADIUS,
        "local_target": 3,
        "local_target_label": "P04 FELYSIA",
        "planetary_map_pose": args.planetary_map_pose,
    }, indent=2))


if __name__ == "__main__":
    main()
