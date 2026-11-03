#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "=== OS Project #11: Automated Build ==="

make all

echo "Build completed successfully!"
