#!/usr/bin/env bash
set -euo pipefail

BIN="./pipeline"
TEST_DIR="/tmp/pipeline_tests_$$"
mkdir -p "$TEST_DIR"

cleanup() {
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

echo "=== Running End-to-End Pipeline Tests ==="

run_check() {
    local test_name="$1"
    local input_file="$2"
    local workers="$3"
    local q_cap="$4"
    local chunk_size="$5"

    local output_file="$TEST_DIR/out.bin"
    rm -f "$output_file"

    $BIN -i "$input_file" -o "$output_file" -w "$workers" -q "$q_cap" -b "$chunk_size"

    local in_hash
    local out_hash
    in_hash=$(sha256sum "$input_file" | awk '{print $1}')
    out_hash=$(sha256sum "$output_file" | awk '{print $1}')

    if [ "$in_hash" = "$out_hash" ]; then
        echo "[PASS] $test_name (W=$workers, Q=$q_cap, B=$chunk_size, Size=$(stat -c%s "$input_file")B)"
    else
        echo "[FAIL] $test_name - SHA256 mismatch!"
        echo "  Input:  $in_hash"
        echo "  Output: $out_hash"
        exit 1
    fi
}

# Test 1: Empty file (0 bytes)
touch "$TEST_DIR/empty.bin"
run_check "Test 1: Empty File" "$TEST_DIR/empty.bin" 4 16 65536

# Test 2: Small payload (127 bytes < 1 chunk)
head -c 127 /dev/urandom > "$TEST_DIR/small.bin"
run_check "Test 2: Small File (< 1 chunk)" "$TEST_DIR/small.bin" 4 16 65536

# Test 3: Exact chunk boundary (65536 bytes)
head -c 65536 /dev/urandom > "$TEST_DIR/exact_chunk.bin"
run_check "Test 3: Exact 1-Chunk Boundary" "$TEST_DIR/exact_chunk.bin" 4 16 65536

# Test 4: Multiple chunks with odd byte length (350,000 bytes)
head -c 350000 /dev/urandom > "$TEST_DIR/multi_chunk.bin"
run_check "Test 4: Multi-chunk (W=1)" "$TEST_DIR/multi_chunk.bin" 1 8 4096
run_check "Test 4: Multi-chunk (W=2)" "$TEST_DIR/multi_chunk.bin" 2 4 4096
run_check "Test 4: Multi-chunk (W=4)" "$TEST_DIR/multi_chunk.bin" 4 16 8192
run_check "Test 4: Multi-chunk (W=8)" "$TEST_DIR/multi_chunk.bin" 8 32 16384

# Test 5: High backpressure test (Queue capacity = 2, large payload)
run_check "Test 5: Backpressure (Q=2)" "$TEST_DIR/multi_chunk.bin" 4 2 4096

# Test 6: Large 10MB binary file stress test
echo "Generating 10MB test file..."
head -c 10485760 /dev/urandom > "$TEST_DIR/large_10M.bin"
run_check "Test 6: 10MB Stress Test (W=4)" "$TEST_DIR/large_10M.bin" 4 16 65536
run_check "Test 6: 10MB Stress Test (W=8)" "$TEST_DIR/large_10M.bin" 8 8 32768

# Test 7: Stdin to Stdout pipeline redirection
pipe_out="$TEST_DIR/pipe_out.bin"
$BIN -w 4 -q 8 -b 16384 < "$TEST_DIR/multi_chunk.bin" > "$pipe_out"
in_hash=$(sha256sum "$TEST_DIR/multi_chunk.bin" | awk '{print $1}')
out_hash=$(sha256sum "$pipe_out" | awk '{print $1}')
if [ "$in_hash" = "$out_hash" ]; then
    echo "[PASS] Test 7: Stdin -> Stdout Redirection"
else
    echo "[FAIL] Test 7: Redirection Hash mismatch!"
    exit 1
fi

echo "=== All Pipeline Tests Passed Successfully! ==="

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

