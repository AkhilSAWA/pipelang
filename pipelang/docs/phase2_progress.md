# PipeLang Phase 2 Progress Report

## Scope

Phase 2 extends the Phase 1 lexer, parser, AST and compiler driver with semantic validation and a
structured compilation pipeline that ends in a dependency DAG.

## Implemented modules

| Module | Purpose | Status |
|---|---|---|
| Lexer (Flex) | Keywords, identifiers, literals, operators, three comment styles; reports bad characters, unterminated strings and comments | Complete |
| Parser (Bison LALR) | All eight operations; expressions with `and`/`or`, comparisons, arithmetic, unary minus; readable syntax errors | Complete |
| AST | Statements with typed members (field, function, order, condition) and source lines | Complete |
| Diagnostics | One format for every stage: `<kind> at line N: message`; warnings never stop compilation | Complete |
| Symbol table | Datasets in declaration order, with declaration and output lines | Complete |
| Semantic analyzer | Dataset usage rules, pipeline-level checks, aggregate functions, condition type checking, dead-dataset warning | Complete |
| Intermediate representation | One instruction per operation; filter conditions kept as expression trees | Complete |
| Dependency analyzer | Links consecutive operations on each dataset | Complete |
| Pipeline DAG | Nodes, edges and topological execution levels | Complete |
| CLI | `--dump-tokens`, `--dump-ast`, `--dump-symbols`, `--dump-ir`, `--dump-dag` | Complete |
| Tests | 34 CTest cases checking exit code, stdout and stderr exactly | Complete |
| CI | GitHub Actions builds with warnings as errors and runs the suite | Complete |

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

1. Flex tokenizes the source and records each token's line.
2. Bison parses the tokens into the custom AST.
3. The semantic analyzer builds the symbol table, validates dataset usage and type-checks conditions.
4. The IR generator lowers AST statements into uniform instructions.
5. Dependency analysis connects operations that act on the same dataset.
6. The DAG computes topological execution levels for future scheduling.

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

The resulting graph has seven nodes and six edges, forming a linear dependency chain for the
`sales` dataset. `tests/valid/multi_dataset.pipe` shows two independent datasets forming parallel
branches, with two nodes per execution level.

## Diagnostics

```text
lexical error at line 3: unexpected character '@'
lexical error at line 3: unterminated string literal
syntax error at line 3: unexpected identifier, expecting where
semantic error at line 2: dataset 'students' is used by an operation before it is declared with input
semantic error at line 3: duplicate input declaration for dataset 'sales'; first declared at line 2
semantic error at line 4: dataset 'sales' is used after its output at line 3
semantic error at line 1: pipeline 'NoOutput' has no output statement
semantic error at line 3: unknown aggregate function 'total'; expected one of: sum, avg, count, min, max
semantic error at line 3: cannot compare number and string with '=='
semantic error at line 3: cannot compare the result of a comparison with '<'; combine conditions with 'and' or 'or'
warning at line 3: dataset 'archive' is never output, so its operations have no effect
```

Line numbers flow through the whole pipeline: Flex tracks `yylineno`, Bison locations (`%locations`
with `%define api.pure full`) carry them through the grammar, each AST statement records its line,
and later stages report it. The compiler reports every independent error in one run. Errors that
are only a consequence of an earlier one are suppressed, such as a syntax error caused by an
unterminated string.

## Test approach

Every test runs `pipelangc` and checks the exit code, stdout and stderr exactly (`tests/run_case.cmake`).
Valid programs are compared with reviewed golden outputs for every stage. Invalid programs must
fail with exactly the expected diagnostics, so extra or cascading messages also fail a test. See
`tests/README.md` for the full inventory.

## Known limitations (by design for Phase 2)

- Datasets have no schema yet, so field names and field types are not checked.
- Transform function names are not checked; there is no function registry yet.
- One pipeline per source file.
- Dependencies only link operations on the same dataset, because no operation reads two datasets.
  Parallelism therefore exists only between independent datasets.

## Phase 3

Graphviz export, optimization passes, execution on real data and parallel scheduling were Phase 3;
see `phase3_progress.md`. Phase 3 also closed two Phase 2 limitations: transform names are now
checked, and field names and types are checked against the input files before execution.
