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