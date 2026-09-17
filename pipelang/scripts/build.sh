#!/usr/bin/env bash
# One-shot build helper.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
echo
echo "Try:"
echo "  ./build/pipelangc examples/student_analysis.pipe --dump-ast"
