
#!/usr/bin/env python3
import argparse
import hashlib
import os
from pathlib import Path

CHUNK_SIZE = 1024 * 1024  # 1 MiB


def main():
    parser = argparse.ArgumentParser(
        description="Generate synthetic test data for OS Project 11"
    )

    size_group = parser.add_mutually_exclusive_group(required=True)
    size_group.add_argument("--size-mb", type=int)
    size_group.add_argument("--size-gb", type=int)

    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.size_mb is not None:
        if args.size_mb <= 0:
            parser.error("--size-mb must be positive")
        total_bytes = args.size_mb * 1024 * 1024
        size_label = f"{args.size_mb} MiB"
    else:
        if args.size_gb <= 0:
            parser.error("--size-gb must be positive")
        total_bytes = args.size_gb * 1024 * 1024 * 1024
        size_label = f"{args.size_gb} GiB"

    checksum_path = args.output.with_name(
        args.output.name + ".sha256"
    )

    if args.output.exists():
        parser.error("Output file already exists")

    if checksum_path.exists():
        parser.error("Checksum file already exists")

    args.output.parent.mkdir(parents=True, exist_ok=True)

    sha256 = hashlib.sha256()

    print(f"Generating {size_label}...")

    try:
        with args.output.open("xb") as f:
            remaining = total_bytes

            while remaining > 0:
                chunk = os.urandom(min(CHUNK_SIZE, remaining))
                f.write(chunk)
                sha256.update(chunk)
                remaining -= len(chunk)

        digest = sha256.hexdigest()

        # Standard sha256sum-compatible sidecar
        with checksum_path.open("x", encoding="utf-8") as f:
            f.write(f"{digest} *{args.output.name}\n")

    except Exception:
        args.output.unlink(missing_ok=True)
        checksum_path.unlink(missing_ok=True)
        raise

    print(f"Output: {args.output}")
    print(f"SHA-256: {digest}")
    print(f"Checksum file: {checksum_path}")


if __name__ == "__main__":
    main()
