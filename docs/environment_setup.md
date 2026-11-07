
# Environment Setup & Reproduction Guide

## 1. Environment

Development environment:
- OS: Ubuntu 26.04.1 LTS (64-bit)
- Kernel: 7.0.0-38-generic
- Compiler: GCC 15.2.0
- Build tool: GNU Make 4.4.1
- Language: C11
- VM: VMware Workstation

## 2. Install Dependencies

Run on Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential git python3 openssl time
```

## 3. Clone Repository

```bash
git clone https://github.com/DuckIsidoros/Asynchronous-I-O-Pipeline-with-Bounded-Buffers-and-Backpressure.git

cd Asynchronous-I-O-Pipeline-with-Bounded-Buffers-and-Backpressure
```

## 4. Build

```bash
chmod +x build.sh
./build.sh
```

Expected executable:

`build/pipeline`

## 5. Generate Test Data

Generate 10 MiB:

```bash
mkdir -p data
python3 scripts/generate_data.py --size-mb 10 --output data/test_10mb.bin
```

Generate 1 GiB:

```bash
python3 scripts/generate_data.py --size-mb 1024 --output data/test_1gb.bin
```

Generate 2 GiB:

```bash
python3 scripts/generate_data.py --size-mb 2048 --output data/test_2gb.bin
```

Generated data is excluded from Git.

## 6. Check Data Integrity

```bash
mkdir -p output
cp data/test_10mb.bin output/test_10mb.bin

python3 scripts/verify_checksum.py \
  data/test_10mb.bin output/test_10mb.bin
```

Matching checksums indicate that the copied file is intact.

## 7. Run Automated Utility Tests

```bash
chmod +x run_tests.sh
./run_tests.sh
```

Expected result:

`=== All utility tests passed! ===`

## 8. Memory Measurement

```bash
sudo apt install -y time
/usr/bin/time -v python3 scripts/generate_data.py \
  --size-mb 256 --output data/ram_test_256mb.bin
```

Observed development results:

| Dataset | Maximum RSS |
|---|---|
| 256 MiB | 22,604 KB |
| 1 GiB | 22,652 KB |

These measurements describe the Python data generator, not the entire I/O pipeline.

## 9. Files Added or Modified

- `Makefile`: C11 build automation with strict compiler flags.
- `build.sh`: automated build entry point.
- `run_tests.sh`: automated utility tests.
- `scripts/generate_data.py`: synthetic binary data generator.
- `scripts/verify_checksum.py`: SHA-256 integrity verification.
- `.gitignore`: excludes generated files and build artifacts.

## 10. Known Limitations

- Clean VM reproduction has not yet been verified.
- The generator uses nondeterministic random data.
- Utility tests do not validate concurrency, backpressure, or worker failure recovery.
- GitHub write access is required to push changes to the upstream repository.
