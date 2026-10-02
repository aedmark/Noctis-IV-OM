#!/usr/bin/env python3
"""Verify and optionally stage the pinned Noctis starmap/guide package."""

import argparse
import hashlib
import json
import shutil
import struct
import sys
from pathlib import Path


def fail(message: str) -> None:
    raise ValueError(message)


def verify_file(path: Path, expected: dict) -> None:
    try:
        data = path.read_bytes()
    except OSError as error:
        fail(f"{path}: cannot read content: {error}")

    if len(data) != expected["size"]:
        fail(f"{path}: size {len(data)} does not match {expected['size']}")
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected["sha256"]:
        fail(f"{path}: SHA-256 {digest} does not match the manifest")

    record_size = expected["record_size"]
    if len(data) < 4 or (len(data) - 4) % record_size:
        fail(f"{path}: content is not aligned to {record_size}-byte records")
    consolidated_size = struct.unpack_from("<I", data)[0]
    if consolidated_size != expected["consolidated_size"]:
        fail(f"{path}: consolidated boundary {consolidated_size} does not match the manifest")
    if not 4 <= consolidated_size <= len(data) or (consolidated_size - 4) % record_size:
        fail(f"{path}: consolidated boundary is invalid")
    records = (len(data) - 4) // record_size
    if records != expected["records"]:
        fail(f"{path}: record count {records} does not match {expected['records']}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--stage", type=Path,
                        help="copy verified files that are missing; preserve existing catalogs")
    args = parser.parse_args()

    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        files = manifest["files"]
        if set(files) != {"STARMAP.BIN", "GUIDE.BIN"}:
            fail("manifest must describe exactly STARMAP.BIN and GUIDE.BIN")
        for name, expected in files.items():
            verify_file(args.source / name, expected)

        staged = []
        preserved = []
        if args.stage:
            args.stage.mkdir(parents=True, exist_ok=True)
            for name in files:
                destination = args.stage / name
                if destination.exists():
                    preserved.append(name)
                else:
                    shutil.copyfile(args.source / name, destination)
                    verify_file(destination, files[name])
                    staged.append(name)
        print(f"content {manifest['content_version']} verified: "
              f"{', '.join(files)}")
        if args.stage:
            print(f"staged: {', '.join(staged) or 'none'}; "
                  f"preserved: {', '.join(preserved) or 'none'}")
        return 0
    except (KeyError, OSError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"content verification failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
