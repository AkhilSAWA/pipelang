# PipeLang

PipeLang is a C++20 domain-specific language and compiler prototype for describing data-processing pipelines.

```text
PipeLang source -> Flex lexer -> Bison parser -> AST
                -> semantic analysis -> IR -> dependency DAG
                -> optimization -> execution
```

This Phase 2 baseline implements a working Flex lexer, Bison LALR parser, custom C++ AST, command-line driver, examples, and CTest smoke tests.

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

## Run

```bash
./build/pipelangc examples/student_analysis.pipe
./build/pipelangc --dump-ast examples/student_analysis.pipe
./build/pipelangc --dump-tokens --dump-ast examples/sales.pipe
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

Expressions support identifiers, integer/float/string literals, parentheses, comparison operators, and `+`, `-`, `*`, `/`. A semicolon is optional after a statement.

## Next milestone

The next Phase 2 milestone is semantic analysis using a symbol table, followed by a custom IR and dependency DAG. That DAG will support later optimization, Graphviz visualization, redundancy detection, and possible parallel execution.
