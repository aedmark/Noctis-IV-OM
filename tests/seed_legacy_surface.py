#!/usr/bin/env python3
"""Write a deterministic 40- or 45-byte legacy surface checkpoint."""

import argparse
import struct
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    parser.add_argument("size", type=int, choices=(40, 45))
    args = parser.parse_args()

    payload = struct.pack(
        "<hhiiiifffff",
        1,
        60,
        100,
        101,
        8192,
        4096,
        1640000.0,
        -200.0,
        1630000.0,
        10.0,
        -20.0,
    )
    if args.size == 45:
        payload += struct.pack("<hhb", -5, 90, 0)
    args.destination.write_bytes(payload)


if __name__ == "__main__":
    main()
