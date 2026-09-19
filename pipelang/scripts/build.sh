#!/usr/bin/env sh
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$root_dir/build"

cmake -S "$root_dir" -B "$build_dir"
cmake --build "$build_dir"
ctest --test-dir "$build_dir" --output-on-failure