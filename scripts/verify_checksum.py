
#!/usr/bin/env python3
import argparse
import hashlib
from pathlib import Path

CHUNK_SIZE = 1024 * 1024

def calculate_sha256(path):
    sha256 = hashlib.sha256()

    with path.open("rb") as file:
        while True:
            chunk = file.read(CHUNK_SIZE)
            if not chunk:
                break
            sha256.update(chunk)

    return sha256.hexdigest()

def main():
    parser = argparse.ArgumentParser(
        description="Verify SHA-256 integrity of two files"
    )
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    input_hash = calculate_sha256(args.input)
    output_hash = calculate_sha256(args.output)

    print(f"Input SHA-256:  {input_hash}")
    print(f"Output SHA-256: {output_hash}")

    if input_hash == output_hash:
        print("PASS: Files are identical by SHA-256")
    else:
        print("FAIL: SHA-256 mismatch")
        raise SystemExit(1)

if __name__ == "__main__":
    main()
