# PipeLang

A domain-specific compiler for describing and optimizing data-processing
pipelines. Individual Compiler Design Laboratory project.

**Phase 2 prototype** — this scaffold gives you a working lexer + parser +
AST + CLI so you can grow it into the full pipeline described in
`# PipeLang — Project Context.md`.

```
PipeLang Source
  → Flex Lexer         ← implemented (grammar/lexer.l)
  → Bison Parser       ← implemented (grammar/parser.y)
  → AST                ← implemented (include/pipelang/ast.hpp)
  → Semantic Analysis  ← TODO (Phase 2 step 6)
  → IR                 ← TODO
  → Dependency + DAG   ← TODO
  → Optimization       ← Phase 3
  → Execution          ← Phase 3
```

## Layout

```
pipelang/
├── CMakeLists.txt            build config (finds Flex + Bison)
├── grammar/
│   ├── lexer.l               Flex tokens
│   └── parser.y              Bison grammar → AST
├── include/pipelang/
│   ├── ast.hpp               AST node types
│   └── driver.hpp            compiler driver (owns lexer + result)
├── src/
│   ├── main.cpp              CLI entry point
│   ├── driver.cpp            Flex ↔ Bison glue
│   └── ast.cpp               AST factories + pretty-printer
├── examples/
│   ├── student_analysis.pipe canonical spec example
│   └── sales.pipe            richer example
├── tests/                    CTest driven acceptance tests
├── scripts/build.sh          one-shot build
└── docs/                     design docs (Phase 1 outputs go here)
```

## Prerequisites

- CMake ≥ 3.16
- A C++20 compiler (g++ ≥ 10 or clang++ ≥ 12)
- Flex ≥ 2.6
- Bison ≥ 3.5
- (later) Graphviz `dot` in PATH for DAG rendering

On Ubuntu / WSL:

```bash
sudo apt update
sudo apt install build-essential cmake flex bison graphviz
```

## Build & run

```bash
./scripts/build.sh
./build/pipelangc examples/student_analysis.pipe --dump-ast
```

Expected output ends with `OK: parsed '…'` and a pretty-printed AST.

## Next implementation steps (Phase 2 checklist)

1. ~~C++20 + CMake project setup~~ ✅
2. ~~Flex lexer~~ ✅
3. ~~Bison parser~~ ✅ (grammar covers all Phase-2 ops)
4. ~~AST~~ ✅
5. **Symbol table** — add `include/pipelang/symbol_table.hpp`; track dataset
   names and their inferred column sets.
6. **Semantic analysis** — walk the AST, verify every op refers to a declared
   dataset, and type-check filter predicates.
7. **IR** — lower AST statements into a flat list of `IROp { kind, dataset,
   args, predicate }` records under `include/pipelang/ir.hpp`.
8. **Dependency analysis** — for each IR op, compute reads/writes on datasets
   and build predecessor edges.
9. **DAG** — materialize the dependency graph; emit Graphviz `.dot` output
   for `--dump-dag`.

Phase 3 then layers optimization passes, an interpreter execution engine,
and optional `std::thread`/`std::async` parallel execution for independent
DAG nodes.

## Development log

Keep a running log at `docs/devlog.md` — one dated entry per work session,
noting decisions and blockers. This is called out in the project guidelines.
