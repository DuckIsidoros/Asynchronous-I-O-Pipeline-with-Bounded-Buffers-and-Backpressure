
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
    parser.add_argument("--size-mb", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.size_mb <= 0:
        parser.error("--size-mb must be positive")

    if args.output.exists():
        parser.error("Output file already exists")

    args.output.parent.mkdir(parents=True, exist_ok=True)

    total_bytes = args.size_mb * CHUNK_SIZE
    sha256 = hashlib.sha256()

    print(f"Generating {args.size_mb} MiB...")

    try:
        with args.output.open("xb") as f:
            remaining = total_bytes
            while remaining > 0:
                chunk = os.urandom(min(CHUNK_SIZE, remaining))
                f.write(chunk)
                sha256.update(chunk)
                remaining -= len(chunk)
    except Exception:
        args.output.unlink(missing_ok=True)
        raise

    print(f"Output: {args.output}")
    print(f"SHA-256: {sha256.hexdigest()}")

if __name__ == "__main__":
    main()
