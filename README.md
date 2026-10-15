# PipeLang

[![CI](https://github.com/AkhilSAWA/pipelang/actions/workflows/ci.yml/badge.svg)](https://github.com/AkhilSAWA/pipelang/actions/workflows/ci.yml)

PipeLang is a C++20 domain-specific language and compiler for describing
data-processing pipelines. It goes beyond a lexer/parser exercise: the
pipeline source is compiled through semantic validation into a custom IR
and a dependency DAG, whose execution levels expose parallelism.

```text
PipeLang source -> Flex lexer -> Bison parser -> AST
                -> symbol table + semantic analysis
                -> custom IR -> dependency analysis -> DAG
                -> (future) optimization -> (future) execution
```

## Status

Phase 2 is complete. Implemented stages:

| Stage | What it does |
| --- | --- |
| Flex lexer | All keywords, operators, literals, identifiers, `//`, `#`, and `/* */` comments; line tracking |
| Bison LALR parser | Full grammar for all eight operations, with operator precedence and detailed error messages |
| AST | Typed statement and expression nodes, each statement carrying its source line |
| Symbol table | Datasets introduced by `input`, with declaration lines |
| Semantic analysis | Rejects use-before-input and duplicate inputs, with source-line diagnostics |
| IR | Uniform instruction list lowered from the validated AST |
| Dependency analysis | Edges between consecutive operations on the same dataset |
| DAG | Topologically ordered execution levels; independent datasets form parallel branches |

Diagnostics are line-accurate end to end:

```text
semantic error at line 3: dataset 'inventory' is used by an operation before it is declared with input
error: line 3: unexpected character '@'
error: line 3: syntax error, unexpected IDENTIFIER, expecting WHERE
```

## Requirements

- CMake 3.20+
- C++20 compiler
- Flex 2.6+
- Bison 3.0+

Ubuntu/Debian:

```bash
sudo apt install build-essential cmake flex bison
```

## Build and test

Run from this `pipelang/` directory:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or:

```bash
sh scripts/build.sh
```

The suite has 15 tests, and every push and pull request is built and
tested automatically by [GitHub Actions](.github/workflows/ci.yml).

## Run

```bash
./build/pipelangc examples/student_analysis.pipe
./build/pipelangc --dump-tokens --dump-ast examples/student_analysis.pipe
./build/pipelangc --dump-symbols examples/sales.pipe
./build/pipelangc --dump-ir examples/sales.pipe
./build/pipelangc --dump-dag examples/sales.pipe
```

Example `--dump-dag` output for a two-dataset pipeline
(`tests/valid/multi_dataset.pipe`) — note the parallel execution levels:

```text
Level 0: [0] [1]
Level 1: [2] [3]
Level 2: [4] [5]
Level 3: [6] [7]
```

## Supported syntax

| Operation | Syntax |
| --- | --- |
| Input | `input dataset` |
| Clean | `clean dataset` |
| Filter | `filter dataset where expression` |
| Transform | `transform dataset function` |
| Sort | `sort dataset by field [ascending\|descending]` |
| Group | `group dataset by field` |
| Aggregate | `aggregate dataset by field function` |
| Output | `output dataset` |

Expressions support identifiers, integer/float/string literals,
parentheses, comparison operators, and `+`, `-`, `*`, `/` with standard
precedence. A semicolon is optional after a statement.

## Testing

Valid programs are checked with exact-output golden tests
(`tests/golden/`): token, AST, symbol-table, IR, and DAG output is
compared byte-for-byte against reviewed expected files. Invalid programs
must fail with a specific line-accurate diagnostic. Coverage includes
parallel DAG branches from independent datasets, arithmetic precedence,
comment handling, undeclared outputs, invalid characters, and syntax
errors. See `tests/README.md` for the full inventory.

## Next milestone

Phase 3: optimization passes (dead/redundant operation detection),
Graphviz rendering of the DAG, an execution engine, and optional
parallel execution of independent DAG levels using `std::thread` /
`std::async`.
