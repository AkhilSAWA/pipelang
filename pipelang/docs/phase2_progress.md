# PipeLang Phase 2 Progress Report

## Scope

Phase 2 extends the Phase 1 lexer, parser, AST, and compiler driver with semantic validation and a structured compilation pipeline.

## Implemented modules

| Module | Purpose | Status |
|---|---|---|
| Symbol table | Records datasets introduced by `input`, with declaration lines | Complete |
| Semantic analyzer | Rejects use-before-input and duplicate inputs, with source-line diagnostics | Complete |
| Intermediate representation | Converts AST statements into uniform instructions | Complete |
| Dependency analyzer | Links consecutive operations on each dataset | Complete |
| Pipeline DAG | Builds dependency nodes, edges, and execution levels | Complete |
| CLI integration | Adds symbol, IR, and DAG dump commands | Complete |
| Automated tests | 15 CTest cases: exact-output golden tests and invalid programs | Complete |
| Source locations | Flex `yylineno` → Bison `%locations` → AST → all diagnostics | Complete |
| CI | GitHub Actions builds the project and runs the test suite | Complete |

## Commands

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure

./build/pipelangc --dump-symbols examples/sales.pipe
./build/pipelangc --dump-ir examples/sales.pipe
./build/pipelangc --dump-dag examples/sales.pipe
```

## Functional flow

1. Flex tokenizes the source.
2. Bison parses tokens into the custom AST.
3. The semantic analyzer constructs the symbol table and validates dataset usage.
4. The IR generator lowers AST statements into uniform instructions.
5. Dependency analysis connects operations that act on the same dataset.
6. The DAG calculates topological execution levels for future scheduling.

## Intermediate result for `sales.pipe`

```text
Intermediate Representation
[0] INPUT dataset=sales
[1] CLEAN dataset=sales
[2] FILTER dataset=sales condition=(revenue >= 1000.50)
[3] SORT dataset=sales field=revenue order=descending
[4] GROUP dataset=sales field=region
[5] AGGREGATE dataset=sales field=revenue function=sum
[6] OUTPUT dataset=sales
```

The resulting graph has seven nodes and six edges, forming a valid linear dependency chain for the `sales` dataset.

## Remaining Phase 3 work

Execution, optimization passes, Graphviz export, and parallel scheduling are intentionally outside the Phase 2 scope.

## Diagnostics

All compiler diagnostics now include exact source lines:

```text
semantic error at line 2: dataset 'students' is used by an operation before it is declared with input
semantic error at line 3: duplicate input declaration for dataset 'sales'; first declared at line 2
error: line 3: unexpected character '@'
error: line 3: syntax error, unexpected IDENTIFIER, expecting WHERE
```

Line numbers flow through the full pipeline: flex reports `yylineno`,
Bison locations (`%locations` + `%api.pure full`) carry them through
the grammar, each AST statement records its line, and the semantic
analyzer reports statement lines instead of statement ordinals.

## Test approach

Valid programs are checked with exact-output golden tests
(`tests/golden/` + `tests/compare_output.cmake`): IR listings, DAG
nodes/edges/levels, symbol tables, and ASTs are compared byte-for-byte
against reviewed expected files. Invalid programs must fail with a
specific line-accurate message. The suite covers parallel DAG branches
from independent datasets, arithmetic precedence, comment handling,
undeclared outputs, invalid characters, and syntax errors.

## Continuous integration

`.github/workflows/ci.yml` builds the project and runs the full test
suite on every push and pull request, so the test results are
reproducible on GitHub instead of only on one machine.
