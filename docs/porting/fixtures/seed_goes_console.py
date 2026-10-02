#!/usr/bin/env python3
"""Set a copied DOS save to a GOESnet console pose and initial minimum power."""

import argparse
import hashlib
import json
import struct
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="clean, normally exited CURRENT.BIN")
    parser.add_argument("destination", type=Path, help="new seeded CURRENT.BIN")
    args = parser.parse_args()

    if args.source.resolve() == args.destination.resolve():
        parser.error("source and destination must differ")
    data = bytearray(args.source.read_bytes())
    if len(data) != 378:
        parser.error(f"expected a 378-byte DOS save, got {len(data)} bytes")
    if struct.unpack_from("<b", data, 9)[0] != 0:
        parser.error("expected a clean save without a selected remote target")

    before = hashlib.sha256(data).hexdigest()
    # Offsets are documented in source/docs/current bin format.txt. The game
    # can consume a lithium charge and raise power again after this seed.
    struct.pack_into("<h", data, 27, 15000)    # initial pwr
    struct.pack_into("<f", data, 39, 3000.0)   # pos_x, near the right wall
    struct.pack_into("<f", data, 47, -2000.0)  # pos_z, at the input console
    struct.pack_into("<f", data, 55, -90.0)    # user_beta, facing the wall
    with args.destination.open("xb") as output:
        output.write(data)
    print(json.dumps({
        "source_sha256": before,
        "destination_sha256": hashlib.sha256(data).hexdigest(),
        "changed_offsets": [27, 39, 47, 55],
        "pwr": 15000,
        "pose": {"pos_x": 3000.0, "pos_z": -2000.0, "user_beta": -90.0},
    }, indent=2))


if __name__ == "__main__":
    main()
