# PipeLang

A domain-specific compiler for data-processing pipeline analysis and optimization (C++20, Flex, Bison, CMake).

PipeLang parses a pipeline, checks it, builds a dependency DAG, optimizes it, and runs it on CSV data,
in parallel where datasets are independent. It can draw the result as a Graphviz graph and as a
self-contained HTML report.

The project lives in [`pipelang/`](pipelang/):
- [`pipelang/README.md`](pipelang/README.md): the language, build steps and usage.
- [`docs/phase2_progress.md`](pipelang/docs/phase2_progress.md) and
  [`docs/phase3_progress.md`](pipelang/docs/phase3_progress.md): what each phase delivered.

```bash
cd pipelang
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
sh scripts/demo.sh        # writes demo/store_report.html and the rendered DAG
```
