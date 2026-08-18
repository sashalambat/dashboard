#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="${CXX:-g++}"
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
echo "Build OK. Binaries in $ROOT/Binaries"
