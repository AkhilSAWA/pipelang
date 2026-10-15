# PipeLang development log

## 2026-09-17 — Phase 2 scaffold
- Set up CMake project with Flex + Bison targets and a `generated/` build dir.
- Wrote the initial lexer covering all keywords, operators, literals, and identifiers from the spec.
- Wrote a Bison grammar that produces an AST (`Pipeline` → list of `Stmt` with optional `Expr` predicate).
- Added `Driver`, CLI (`pipelangc`), two example programs, and CTest wiring.
- Next: symbol table + semantic analysis (Phase 2 step 5–6).
# PipeLang Development Log

## 2026-09-20 — Phase 2 buildable prototype repair

- Rewired the C++20 CMake project to generate and compile Flex and Bison sources.
- Added `pipelangc` with `--dump-tokens` and `--dump-ast` modes.
- Added a custom AST for all eight core PipeLang operations.
- Added token support for keywords, identifiers, integer/float/string literals, operators, punctuation, and comments.
- Added an LALR grammar for pipelines and expressions.
- Added example pipelines, CTest smoke tests, and a one-command build script.

## Next Phase 2 tasks

- Add source locations to AST nodes and diagnostics.
- Add a symbol table and input-before-use semantic validation.
- Lower AST operations to a custom intermediate representation.
- Create dependency edges and a DAG for ordering and parallelization analysis.
## 2026-10-15 — Hardening: source lines, golden tests, CI
- Enabled Bison `%locations` with the pure parser; the lexer now updates
  `yylloc` from `yylineno`, and `yyerror` reports the offending line.
- Added `Stmt::line` to the AST; every statement factory takes the line
  from the grammar rule location.
- Semantic errors and duplicate-input messages now report source lines
  (with the first declaration line for duplicates); the symbol table
  stores declaration lines.
- Expanded the test suite from 6 to 15 CTest cases: exact-output golden
  tests for tokens/AST/IR/DAG/symbols, parallel-branch DAG, arithmetic
  precedence, comment handling, undeclared output, invalid character,
  and missing-where syntax error. Invalid tests assert the diagnostic
  line via PASS_REGULAR_EXPRESSION (note: WILL_FAIL inverts a regex
  match, so the two must not be combined).
- Added a GitHub Actions workflow that builds and runs the suite on
  every push and pull request.
