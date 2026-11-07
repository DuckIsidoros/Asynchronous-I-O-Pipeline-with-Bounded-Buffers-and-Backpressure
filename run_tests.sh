
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "=== OS Project #11: Utility Tests ==="

TEST_DIR=$(mktemp -d)
trap 'rm -rf -- "$TEST_DIR"' EXIT

INPUT="$TEST_DIR/input.bin"
OUTPUT="$TEST_DIR/output.bin"
SIDECAR="$INPUT.sha256"

echo "[1/7] Generating 10 MiB test data..."
python3 scripts/generate_data.py \
    --size-mb 10 --output "$INPUT"

echo "[2/7] Checking generated file and sidecar..."
test -f "$INPUT"
test -f "$SIDECAR"
test "$(stat -c %s "$INPUT")" -eq $((10 * 1024 * 1024))

echo "[3/7] Verifying sidecar..."
python3 scripts/verify_checksum.py \
    "$INPUT" --sidecar "$SIDECAR"

echo "[4/7] Verifying expected SHA-256 digest..."
HASH=$(sha256sum "$INPUT" | awk '{print $1}')

python3 scripts/verify_checksum.py \
    "$INPUT" --expected-sha256 "$HASH"

echo "[5/7] Testing identical files..."
cp "$INPUT" "$OUTPUT"

python3 scripts/verify_checksum.py "$INPUT" "$OUTPUT"

echo "[6/7] Testing corrupted output..."
printf 'X' | dd of="$OUTPUT" bs=1 seek=0 conv=notrunc status=none

if python3 scripts/verify_checksum.py "$INPUT" "$OUTPUT"; then
    echo "ERROR: Corrupted file was not detected!"
    exit 1
else
    echo "PASS: Corruption detected correctly."
fi

echo "[7/7] Testing invalid digest..."
INVALID_HASH=$(printf '%064d' 0)

if python3 scripts/verify_checksum.py \
    "$INPUT" --expected-sha256 "$INVALID_HASH"; then
    echo "ERROR: Invalid digest was accepted!"
    exit 1
else
    echo "PASS: Invalid digest rejected."
fi

echo "=== All utility tests passed! ==="
