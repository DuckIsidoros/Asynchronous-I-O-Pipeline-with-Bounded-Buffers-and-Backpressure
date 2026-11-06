
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "=== OS Project #11: Utility Tests ==="

TEST_DIR=$(mktemp -d)
trap 'rm -rf -- "$TEST_DIR"' EXIT

INPUT="$TEST_DIR/input.bin"
OUTPUT="$TEST_DIR/output.bin"

echo "[1/3] Generating 10 MiB test data..."
python3 scripts/generate_data.py \
    --size-mb 10 --output "$INPUT"

echo "[2/3] Testing identical files..."
cp "$INPUT" "$OUTPUT"

python3 scripts/verify_checksum.py "$INPUT" "$OUTPUT"

echo "[3/3] Testing corrupted output..."
printf 'X' | dd of="$OUTPUT" bs=1 seek=0 conv=notrunc status=none

if python3 scripts/verify_checksum.py "$INPUT" "$OUTPUT"; then
    echo "ERROR: Corrupted file was not detected!"
    exit 1
else
    echo "PASS: Corruption detected correctly."
fi

echo "=== All utility tests passed! ==="

