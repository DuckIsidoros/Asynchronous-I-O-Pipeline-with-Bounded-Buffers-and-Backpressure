
# Environment Setup & Reproduction Guide

This document describes the development environment, dependencies, build process, test-data generation, SHA-256 integrity verification, and utility tests for **OS Project #11: Asynchronous I/O Pipeline with Bounded Buffers and Backpressure**.

> **Scope:** The steps below reproduce the build and utility checks. They do not by themselves establish correct concurrency, backpressure, or failure recovery for the complete pipeline.

## 1. Development Environment

The project was developed and tested on the following virtual-machine environment:

| Component | Version / Configuration |
|---|---|
| Operating system | Ubuntu 26.04.1 LTS (64-bit) |
| Kernel | `7.0.0-38-generic` |
| C compiler | GCC 15.2.0 |
| Build tool | GNU Make 4.4.1 |
| CMake requirement | 3.16 or newer |
| Programming language | C11 |
| Threading | POSIX threads (`pthread`) |
| Virtualization | VMware Workstation |
| Python | Python 3 |

The listed Ubuntu, kernel, compiler, and Make versions describe the development VM, not the only supported versions.

The CMake requirement is the minimum version declared in `CMakeLists.txt`.

To inspect the local environment:

```bash
lsb_release -a
uname -r
gcc --version
make --version
cmake --version
python3 --version
```

## 2. Install Dependencies

Run these commands on Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 openssl time
```

The required dependencies include:

- `build-essential`: GCC, GNU Make, and essential compilation tools.
- `cmake`: Alternative build configuration system.
- `git`: Repository management.
- `python3`: Data generation and verification scripts.
- `openssl`: Additional cryptographic utilities.
- `time`: Memory and performance measurements.

The Python checksum utilities use the standard-library `hashlib` module and do not require an external Python package.

## 3. Clone the Repository

```bash
git clone https://github.com/DuckIsidoros/Asynchronous-I-O-Pipeline-with-Bounded-Buffers-and-Backpressure.git

cd Asynchronous-I-O-Pipeline-with-Bounded-Buffers-and-Backpressure
```

All subsequent commands should be executed from the repository root unless otherwise specified.

For an existing clone, ensure that the intended branch and commits are checked out before reproducing tests.

## 4. Build the C Pipeline

The project supports two build methods: GNU Make and CMake.

Both build methods are intended to compile the project using C11, strict compiler warnings, and POSIX thread support.

### 4.1 Option A: GNU Make

Run:

```bash
make
```

Expected executable:

```text
build/pipeline
```

To force a complete rebuild:

```bash
make -B
```

Alternatively, use the existing build script:

```bash
chmod +x build.sh
./build.sh
```

The Makefile uses the following compiler flags:

```text
-std=c11 -Wall -Wextra -Werror -pedantic -pthread -Iinclude
```

These flags enforce C11 compliance, enable strict warnings, and support POSIX threads.

### 4.2 Option B: CMake

Configure the project:

```bash
cmake -S . -B build-cmake -DCMAKE_C_COMPILER=gcc
```

Build the executable:

```bash
cmake --build build-cmake
```

Expected executable:

```text
build-cmake/pipeline
```

The CMake configuration:

- Compiles C source files under `src/`.
- Includes project headers from `include/`.
- Uses the C11 language standard.
- Enables strict compiler warnings.
- Links POSIX thread support.

The separate `build-cmake/` directory prevents CMake-generated files from interfering with the existing Makefile build directory.

Both `build/` and `build-cmake/` contain generated artifacts and should be excluded from Git.

**Note:** Successful compilation verifies that the source code builds. It does not prove that the pipeline handles concurrency and backpressure correctly.

## 5. Generate Synthetic Test Data

The data generator creates random binary files for pipeline testing.

It processes data in 1 MiB chunks, avoiding the need to load the entire dataset into memory.

The generator supports:

- `--size-mb`: Specify file size in MiB.
- `--size-gb`: Specify file size in GiB.
- `--output`: Specify the output file path.

Only one size option can be supplied per invocation.

### 5.1 Generate 10 MiB

```bash
mkdir -p data

python3 scripts/generate_data.py \
  --size-mb 10 \
  --output data/test_10mb.bin
```

### 5.2 Generate 1 GiB

```bash
python3 scripts/generate_data.py \
  --size-gb 1 \
  --output data/test_1gb.bin
```

### 5.3 Generate 2 GiB

```bash
python3 scripts/generate_data.py \
  --size-gb 2 \
  --output data/test_2gb.bin
```

### 5.4 Automatic SHA-256 Sidecar Generation

Each successful execution generates two files:

```text
data/
├── test_10mb.bin
└── test_10mb.bin.sha256
```

The `.sha256` file contains the SHA-256 digest associated with the generated binary file.

Example format:

```text
<64-character SHA-256 digest> *test_10mb.bin
```

For ordinary filenames, this format is compatible with GNU `sha256sum`.

SHA-256 is calculated incrementally while the data is generated.

This avoids rereading the entire output file solely to calculate its checksum.

### 5.5 File Protection and Storage Requirements

The generator does not overwrite existing output files.

If the binary file or its `.sha256` sidecar already exists, use a different filename or deliberately remove obsolete test data before rerunning the generator.

Check available disk space before generating large datasets:

```bash
df -h .
```

Generated data should be excluded from Git.

Because the generator uses `os.urandom()`, successive runs produce different binary contents and SHA-256 digests.

## 6. Check Data Integrity

SHA-256 verification helps determine whether a file matches its reference data.

The verification utility supports three modes.

### 6.1 Method A: Compare Two Files

Create a copy of the generated file:

```bash
mkdir -p output

cp data/test_10mb.bin output/test_10mb.bin
```

Verify both files:

```bash
python3 scripts/verify_checksum.py \
  data/test_10mb.bin \
  output/test_10mb.bin
```

The program calculates SHA-256 for both files and compares their digests.

Matching digests provide strong evidence that both files contain identical data.

This mode is retained for backward compatibility.

### 6.2 Method B: Verify Using a SHA-256 Sidecar

Verify a binary file using the generated `.sha256` file:

```bash
python3 scripts/verify_checksum.py \
  data/test_10mb.bin \
  --sidecar data/test_10mb.bin.sha256
```

Expected result:

```text
PASS: SHA-256 verification successful
```

The same sidecar can be independently verified using GNU `sha256sum`:

```bash
(cd data && sha256sum -c test_10mb.bin.sha256)
```

Expected result:

```text
test_10mb.bin: OK
```

### 6.3 Method C: Verify Using an Expected SHA-256 Digest

The verifier can accept a previously recorded SHA-256 digest directly.

Example:

```bash
python3 scripts/verify_checksum.py \
  data/test_10mb.bin \
  --expected-sha256 YOUR_TRUSTED_SHA256_DIGEST
```

The expected digest must contain exactly 64 hexadecimal characters.

For a basic command-line functionality test:

```bash
HASH=$(sha256sum data/test_10mb.bin | awk '{print $1}')

python3 scripts/verify_checksum.py \
  data/test_10mb.bin \
  --expected-sha256 "$HASH"
```

**Important:** Calculating the expected hash from the same file immediately before verification only tests the command interface. To detect previous data modification, use a previously recorded, trusted digest.

### 6.4 Verification Results

| Condition | Expected result |
|---|---|
| Matching SHA-256 digests | PASS |
| Mismatched SHA-256 digests | FAIL |
| Invalid checksum format | Error |
| Missing input file | Error |
| Missing required arguments | Error |

A successful verification returns exit status `0`.

A checksum mismatch returns a nonzero exit status.

## 7. Run Automated Utility Tests

The repository contains `run_tests.sh` for automated regression testing.

Enable execution permission:

```bash
chmod +x run_tests.sh
```

Run:

```bash
./run_tests.sh
```

Alternatively:

```bash
bash run_tests.sh
```

The updated test script covers:

1. Generating a 10 MiB binary file.
2. Checking the generated file and `.sha256` sidecar.
3. Verifying data integrity using the sidecar.
4. Verifying data integrity using an expected digest.
5. Comparing two identical binary files.
6. Detecting corrupted output data.
7. Rejecting a mismatched expected digest.

Expected final result:

```text
=== All utility tests passed! ===
```

A `FAIL: SHA-256 mismatch` message during a negative test is expected.

The test intentionally supplies corrupted data or an incorrect digest to verify that the utility detects the mismatch.

### 7.1 Python Syntax Validation

```bash
python3 -m py_compile \
  scripts/generate_data.py \
  scripts/verify_checksum.py
```

If no syntax errors are reported, both scripts pass Python syntax validation.

**Note:** These utility tests do not replace dedicated tests for producer-consumer synchronization, bounded queues, or backpressure behavior.

## 8. Memory Measurement

The data generator processes binary data in fixed-size chunks instead of allocating the entire output file in memory.

This helps maintain bounded memory consumption during large-file generation.

### 8.1 Measure Peak Memory Usage

Generate a new 256 MiB dataset:

```bash
mkdir -p data

/usr/bin/time -v python3 scripts/generate_data.py \
  --size-mb 256 \
  --output data/ram_test_256mb_new.bin
```

The `time -v` command reports resource usage, including:

```text
Maximum resident set size (kbytes)
```

This value represents the maximum resident memory observed during execution.

The output filename must not already exist.

### 8.2 Observed Development Results

| Dataset | Maximum RSS |
|---|---:|
| 256 MiB | 22,604 KB |
| 1 GiB | 22,652 KB |

These results were observed on the development VM.

The measurements refer to the Python data generator, not the complete asynchronous I/O pipeline.

Results may differ depending on system configuration, Python version, and runtime conditions.

## 9. Files Added or Modified

| File | Purpose |
|---|---|
| `Makefile` | GNU Make build automation with strict C11 compiler flags |
| `CMakeLists.txt` | CMake build configuration |
| `build.sh` | Automated build entry point |
| `run_tests.sh` | Automated utility regression tests |
| `scripts/generate_data.py` | Synthetic binary data generator with MiB/GiB support and SHA-256 sidecar generation |
| `scripts/verify_checksum.py` | SHA-256 verification through two-file comparison, sidecar, or expected digest |
| `docs/environment_setup.md` | Environment setup and reproduction instructions |
| `.gitignore` | Excludes generated datasets and build artifacts |

Generated build artifacts should not be committed to the repository.

## 10. Known Limitations

### 10.1 Environment Reproduction

Clean VM reproduction has not yet been independently verified from a fresh installation and repository clone.

The documented environment describes the development VM and does not establish compatibility with every Ubuntu or compiler version.

### 10.2 Nondeterministic Test Data

The generator uses `os.urandom()`.

As a result, generated files are not reproducible byte for byte across different runs.

### 10.3 Limited Utility Test Coverage

The utility tests validate data generation and SHA-256 integrity checking.

They do not independently prove:

- Correct synchronization between producer and consumer threads.
- Proper backpressure behavior when the bounded queue is full.
- Absence of race conditions or deadlocks.
- Correct worker failure recovery.
- Overall I/O pipeline performance.

These properties require separate runtime and concurrency tests.

### 10.4 SHA-256 Security Limitations

SHA-256 detects mismatches relative to a trusted reference digest.

However, checksum verification alone does not authenticate the data source if both the data and its checksum can be modified.

### 10.5 Repository Access

GitHub write permission is required to push changes to the upstream repository.

Read-only access is sufficient for cloning, building, and running local tests.

## 11. Final Validation Before Commit

Run the following commands from the repository root:

### 11.1 Check Git Formatting

```bash
git diff --check
```

### 11.2 Validate Bash Syntax

```bash
bash -n run_tests.sh
```

### 11.3 Validate Python Syntax

```bash
python3 -m py_compile \
  scripts/generate_data.py \
  scripts/verify_checksum.py
```

### 11.4 Run Utility Tests

```bash
bash run_tests.sh
```

Expected result:

```text
=== All utility tests passed! ===
```

### 11.5 Validate CMake Build

```bash
cmake -S . -B build-cmake -DCMAKE_C_COMPILER=gcc

cmake --build build-cmake
```

Expected result:

```text
[100%] Built target pipeline
```

### 11.6 Review Repository Changes

```bash
git status --short
```

Verify that generated files under the following directories are not staged:

```text
build/
build-cmake/
data/
output/
```

Only intended source code, scripts, build configurations, and documentation should be included in the commit.

---

## Conclusion

The documented environment provides two supported build workflows using GNU Make and CMake.

The upgraded Python utilities support binary data generation in MiB/GiB units and SHA-256 verification through file comparison, sidecar files, and expected digests.

Automated regression tests validate the main utility functions and detect mismatched data.

Further testing is required to establish correctness and performance of the full asynchronous I/O pipeline, including bounded-buffer synchronization, backpressure, and worker failure handling.
