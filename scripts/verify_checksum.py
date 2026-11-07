
#!/usr/bin/env python3
import argparse
import hashlib
import re
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


def read_sidecar(sidecar_path, input_path):
    # Format: <64 hex characters> *<filename>
    line = sidecar_path.read_text(encoding="utf-8").rstrip("\r\n")

    match = re.fullmatch(r"([0-9a-fA-F]{64}) ([ *])(.+)", line)

    if not match:
        raise ValueError("Invalid SHA-256 sidecar format")

    digest, mode, filename = match.groups()

    if filename != input_path.name:
        raise ValueError(
            "Sidecar filename does not match input filename"
        )

    return digest.lower()


def main():
    parser = argparse.ArgumentParser(
        description="Verify SHA-256 file integrity"
    )

    parser.add_argument("input", type=Path)
    parser.add_argument(
        "output", type=Path, nargs="?",
        help="Second file for legacy comparison"
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--sidecar", type=Path,
        help="Verify using a .sha256 sidecar"
    )
    group.add_argument(
        "--expected-sha256", type=str,
        help="Verify using an expected SHA-256 digest"
    )

    args = parser.parse_args()

    if args.output is not None:
        if args.sidecar or args.expected_sha256:
            parser.error(
                "Cannot combine output with checksum options"
            )

        expected_hash = calculate_sha256(args.input)
        actual_hash = calculate_sha256(args.output)

    elif args.sidecar is not None:
        try:
            expected_hash = read_sidecar(args.sidecar, args.input)
        except (ValueError, OSError, UnicodeError) as exc:
            parser.error(str(exc))

        actual_hash = calculate_sha256(args.input)

    elif args.expected_sha256 is not None:
        expected_hash = args.expected_sha256.lower()

        if not re.fullmatch(r"[0-9a-f]{64}", expected_hash):
            parser.error(
                "Expected SHA-256 must contain 64 hex characters"
            )

        actual_hash = calculate_sha256(args.input)

    else:
        parser.error(
            "Provide output, --sidecar or --expected-sha256"
        )

    print(f"Expected SHA-256: {expected_hash}")
    print(f"Actual SHA-256:   {actual_hash}")

    if actual_hash == expected_hash:
        print("PASS: SHA-256 verification successful")
    else:
        print("FAIL: SHA-256 mismatch")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
