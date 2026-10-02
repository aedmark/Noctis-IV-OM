#!/usr/bin/env python3
"""Read the pinned NIV+ DOS target fields from one or more CURRENT.BIN saves."""

import argparse
import hashlib
import json
import struct
from pathlib import Path


def extract(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) < 119:
        raise ValueError(f"{path}: expected at least 119 bytes, got {len(data)}")

    def at(fmt: str, offset: int):
        return struct.unpack_from("<" + fmt, data, offset)[0]

    return {
        "path": str(path),
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "ap_targetting": at("b", 8),
        "ap_targetted": at("b", 9),
        "class": at("h", 31),
        "spin": at("b", 14),
        "rgb": [at("b", offset) for offset in (15, 16, 17)],
        "radius_f32": at("f", 63),
        "radius_bits_hex": f"0x{at('I', 63):08x}",
        "xyz": [at("d", offset) for offset in (95, 103, 111)],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("current_bin", type=Path, nargs="+")
    args = parser.parse_args()

    captures = [extract(path) for path in args.current_bin]
    fields = ("ap_targetting", "ap_targetted", "class", "spin", "rgb", "radius_bits_hex", "xyz")
    stable = all(
        all(capture[field] == captures[0][field] for field in fields)
        for capture in captures[1:]
    )
    print(json.dumps({"captures": captures, "target_fields_equal": stable}, indent=2))


if __name__ == "__main__":
    main()
