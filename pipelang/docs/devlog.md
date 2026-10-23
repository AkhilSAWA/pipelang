# PipeLang development log

## 2026-09-17: Phase 2 scaffold
- Set up the CMake project with Flex and Bison targets and a `generated/` build directory.
- Wrote the initial lexer covering all keywords, operators, literals and identifiers from the spec.
- Wrote a Bison grammar that produces an AST (`Pipeline` → list of `Stmt` with an optional `Expr` predicate).
- Added `Driver`, the `pipelangc` CLI, two example programs and CTest wiring.

## 2026-09-20: Buildable prototype repair
- Rewired the C++20 CMake project to generate and compile the Flex and Bison sources.
- Added the `--dump-tokens` and `--dump-ast` modes.
- Added the AST for all eight core operations and an LALR grammar for pipelines and expressions.
- Added example pipelines, CTest smoke tests and a one-command build script.

## 2026-10-15: Semantic analysis, IR, DAG, source lines, golden tests, CI
- Added the symbol table, semantic analyzer (input before use, duplicate input), IR, dependency
  analysis and DAG with execution levels, plus `--dump-symbols`, `--dump-ir` and `--dump-dag`.
- Enabled Bison `%locations` with the pure parser. The lexer updates `yylloc` from `yylineno`,
  `yyerror` reports the offending line, and `Stmt::line` carries lines into semantic errors.
- Grew the suite from 6 to 15 CTest cases with exact-output golden tests, and added a GitHub
  Actions workflow.

## 2026-10-23: Phase 2 review and hardening
- **Diagnostics:** new `diagnostic` module with one format for every stage
  (`lexical|syntax|semantic error at line N: ...`, `warning at line N: ...`). The lexer now reports
  a bad character and keeps scanning instead of stopping. Syntax errors that follow a lexical error
  are suppressed as cascades. Token aliases make Bison messages readable
  (`expecting where`, not `expecting WHERE`).
- **Lexer:** unterminated strings and block comments are reported; strings can no longer span
  lines; `--dump-tokens` now lists every token, keywords and operators included. Removed the
  unused `,` token.
- **Grammar:** added `and`, `or` and unary minus with explicit precedence. Repeated actions were
  replaced by small helpers, and a `%destructor` frees discarded values on syntax errors (checked
  with AddressSanitizer).
- **AST:** `Stmt` has explicit `field` and `function` members. Aggregate used to encode both as
  `"revenue using sum"` and the IR split the string again. `Pipeline` records its line.
- **Semantic analysis:** added these errors: empty pipeline, no output, use after output,
  duplicate output, unknown aggregate function. Added type checking of filter conditions:
  comparison required, numeric arithmetic, number vs string, chained comparisons, division by
  literal zero. A dataset that is never output gives a warning. Independent errors are all reported.
- **Symbol table:** keeps declaration order without sorting, and records declaration and output lines.
- **IR:** filter conditions are copied as expression trees instead of strings, so string literals
  keep their quotes and Phase 3 can evaluate or rewrite conditions. Instructions record their line.
- **DAG:** execution levels are computed once at construction.
- **Dead code removed:** `print_dependencies` (never called), `Driver::tokens()` (unused), the
  `INVALID` and `COMMA` tokens, and the old string-splitting in IR generation.
- **Tests:** new `tests/run_case.cmake` checks exit code, stdout and stderr exactly (the old
  regex tests ignored the exit code). The suite grew from 15 to 34 cases.
- **Build/CI:** `PIPELANG_WERROR` option; CI builds with warnings as errors.
- **Cleanup:** removed the stale `build/` directory; the root README now points to `pipelang/`
  instead of duplicating an outdated copy.

## 2026-10-23: Phase 3 — optimization, execution and visualization
- **Runtime core:** `value` (null/number/string cells, number formatting), `table` (CSV reader
  with quotes, `""`, line breaks in quotes, CRLF, BOM, type inference; CSV writer), and a
  `functions` registry (`calculate_grade`, `uppercase`, `add_row_number`; `sum`, `avg`, `count`,
  `min`, `max`). Transform names are now checked at compile time.
- **Schema analysis:** before execution, fields and column types are checked against the input
  files, following how transforms and aggregates change the columns. Errors use source lines.
- **Optimizer:** constant folding, dead-code elimination, filter pushdown, filter merging,
  redundant-clean and redundant-sort removal, each logged (`--dump-opt`). The reason each pass
  preserves the output is documented in `phase3_progress.md`.
- **Executor:** all eight operations, with a compiled condition evaluator. A dependency-driven
  thread pool (`--jobs`) runs independent datasets in parallel. Per-node timings (`--timings`),
  output files (`--out`).
- **Visualization:** Graphviz DOT export (`--emit-dot`), and a self-contained HTML report
  (`--report`) with before/after DAGs, the optimization log, a per-thread execution timeline and
  result tables.
- **Tools:** `pipelang_datagen`, `scripts/demo.sh` (all visual outputs in one command),
  `scripts/benchmark.sh`.
- **Tests:** suite moved to `tests/CMakeLists.txt` and grown from 34 to 109 cases, including
  equivalence tests (plain vs `-O` vs threads) and report and DOT checks. Also checked with
  2,300 random differential cases against an independent reference implementation, ASan/UBSan
  and ThreadSanitizer.
- **Build:** Release build by default; Threads linked; CI installs Graphviz.
