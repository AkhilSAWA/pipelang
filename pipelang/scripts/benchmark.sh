#!/usr/bin/env sh
# Measures the optimizer and parallel execution on generated data.
#   sh scripts/benchmark.sh [rows-per-dataset] [threads]     (default 200000 4)
# Prints the median pipeline run time of 3 runs per configuration (input
# loading is excluded) and writes build/benchmark/report.html.
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$root_dir/build"
bench_dir="$build_dir/benchmark"
rows=${1:-200000}
jobs=${2:-4}

cmake -S "$root_dir" -B "$build_dir" > /dev/null
cmake --build "$build_dir" > /dev/null
mkdir -p "$bench_dir"
"$build_dir/pipelang_datagen" "$bench_dir/data" "$rows"
cp "$root_dir/examples/benchmark/benchmark.pipe" "$bench_dir/benchmark.pipe"

run_time() {
    "$build_dir/pipelangc" --run --timings "$@" "$bench_dir/benchmark.pipe" |
        sed -n 's/^Run pipeline: \(.*\) ms$/\1/p'
}

median_of_three() {
    printf '%s\n%s\n%s\n' "$(run_time "$@")" "$(run_time "$@")" "$(run_time "$@")" |
        sort -n | sed -n 2p
}

printf '%-32s %12s\n' "Configuration" "Run (ms)"
printf '%-32s %12s\n' "sequential, unoptimized" "$(median_of_three --jobs 1)"
printf '%-32s %12s\n' "sequential, optimized (-O)" "$(median_of_three --jobs 1 -O)"
printf '%-32s %12s\n' "$jobs threads, unoptimized" "$(median_of_three --jobs "$jobs")"
printf '%-32s %12s\n' "$jobs threads, optimized (-O)" "$(median_of_three --jobs "$jobs" -O)"

"$build_dir/pipelangc" --run -O --jobs "$jobs" --report "$bench_dir/report.html" \
    "$bench_dir/benchmark.pipe" > /dev/null
echo "Report: $bench_dir/report.html"
