#!/usr/bin/env sh
# Builds PipeLang and produces the Phase 3 visual outputs for the store demo:
#   demo/store_report.html   self-contained HTML report (open in a browser)
#   demo/store_dag.dot       Graphviz source of the optimized DAG
#   demo/store_dag.svg/.png  rendered DAG (only if Graphviz `dot` is installed)
#   demo/results/*.csv       the pipeline's output datasets
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$root_dir/build"
demo_dir="$root_dir/demo"

cmake -S "$root_dir" -B "$build_dir" > /dev/null
cmake --build "$build_dir" > /dev/null
mkdir -p "$demo_dir"

cd "$root_dir"
"$build_dir/pipelangc" -O --run --jobs 4 --timings \
    --out "$demo_dir/results" \
    --emit-dot "$demo_dir/store_dag.dot" \
    --report "$demo_dir/store_report.html" \
    examples/store_analytics.pipe

if command -v dot > /dev/null 2>&1; then
    dot -Tsvg "$demo_dir/store_dag.dot" -o "$demo_dir/store_dag.svg"
    dot -Tpng -Gdpi=150 "$demo_dir/store_dag.dot" -o "$demo_dir/store_dag.png"
    echo "Rendered DAG: $demo_dir/store_dag.svg and store_dag.png"
else
    echo "Graphviz 'dot' not found; install it to render $demo_dir/store_dag.dot"
fi
echo "HTML report: $demo_dir/store_report.html"
