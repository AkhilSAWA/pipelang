# PipeLang development log

## 2026-09-17 — Phase 2 scaffold
- Set up CMake project with Flex + Bison targets and a `generated/` build dir.
- Wrote the initial lexer covering all keywords, operators, literals, and identifiers from the spec.
- Wrote a Bison grammar that produces an AST (`Pipeline` → list of `Stmt` with optional `Expr` predicate).
- Added `Driver`, CLI (`pipelangc`), two example programs, and CTest wiring.
- Next: symbol table + semantic analysis (Phase 2 step 5–6).
