# PipeLang

PipeLang is a C++20 domain-specific language and compiler for data-processing pipelines. It parses a
pipeline, checks it, builds a dependency DAG, optimizes it, and runs it on CSV data, using several
threads where datasets are independent. It can also draw the result as a Graphviz graph and as a
self-contained HTML report.

```text
PipeLang source -> Flex lexer -> Bison parser -> AST
                -> symbol table + semantic analysis -> IR -> dependency DAG     (Phase 2)
                -> schema check -> optimizer -> parallel executor               (Phase 3)
                -> CSV results, Graphviz DOT, HTML report                        (Phase 3)
```

## Requirements

- CMake 3.20+, a C++20 compiler, Flex 2.6+, Bison 3.0+
- Optional: Graphviz (`dot`) to render the DAG as SVG/PNG

Ubuntu/Debian (or WSL on Windows):

```bash
sudo apt install build-essential cmake flex bison graphviz
```

## Build and test

Run from this `pipelang/` directory:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Builds are optimized (Release) unless you pass `-DCMAKE_BUILD_TYPE=...`. Add `-DPIPELANG_WERROR=ON`
to treat warnings as errors (CI does this).

## Quick demo (visual output)

```bash
sh scripts/demo.sh
```

This compiles, optimizes and runs `examples/store_analytics.pipe` on 4 threads, then writes:

| File | What it shows |
|---|---|
| `demo/store_report.html` | Open in a browser: source, before/after DAG, optimization log, execution timeline per thread, result tables |
| `demo/store_dag.svg`, `.png` | The optimized DAG rendered by Graphviz (if installed) |
| `demo/store_dag.dot` | Graphviz source |
| `demo/results/*.csv` | The output datasets |

`sh scripts/benchmark.sh [rows] [threads]` generates four large datasets and measures sequential vs
parallel and unoptimized vs optimized execution.

## Usage

```bash
./build/pipelangc examples/sales.pipe                          # compile, print a summary
./build/pipelangc --dump-ir examples/sales.pipe                # show a compiler stage
./build/pipelangc --dump-opt examples/sales.pipe               # optimization log + optimized IR
./build/pipelangc --run examples/sales.pipe                    # run on examples/data/*.csv
./build/pipelangc -O --run --jobs 4 --timings examples/store_analytics.pipe
./build/pipelangc -O --run --report report.html --emit-dot dag.dot examples/store_analytics.pipe
```

| Option | Meaning |
|---|---|
| `--dump-tokens`, `--dump-ast`, `--dump-symbols`, `--dump-ir`, `--dump-dag` | print a compiler stage |
| `-O`, `--optimize` | apply the optimization passes |
| `--dump-opt` | print what the optimizer changed and the optimized IR (implies `-O`) |
| `--run` | execute the pipeline and print each output dataset as CSV |
| `--data <dir>` | where `<dataset>.csv` files are read from (default: `data/` next to the source file) |
| `--out <dir>` | also write each output dataset to `<dir>/<dataset>.csv` |
| `--jobs <n>` | worker threads (default 1; 0 = one per CPU core) |
| `--timings` | print per-node execution times |
| `--emit-dot <file>` | write the DAG as Graphviz DOT (`-` = stdout) |
| `--report <file>` | write the HTML report (includes execution sections with `--run`) |

Exit code: 0 on success (warnings allowed), 1 on a compile, data or runtime error, 2 on a
command-line error.

## Language

```pipelang
pipeline StoreAnalytics {
    input orders
    clean orders
    filter orders where amount > 100 and status == "delivered"
    group orders by region
    aggregate orders by amount sum
    output orders
}
```

| Operation | Syntax | Meaning at run time |
|---|---|---|
| Input | `input d` | read `d.csv` |
| Clean | `clean d` | drop rows with an empty cell, then exact duplicate rows (first kept) |
| Filter | `filter d where condition` | keep rows where the condition is true |
| Transform | `transform d function` | apply a built-in function (below) |
| Sort | `sort d by field [ascending\|descending]` | stable sort; empty values last |
| Group | `group d by field` | stable sort by the field and mark it as the group key |
| Aggregate | `aggregate d by field function` | one row per group (or one row in total): the key and `<function>_<field>` |
| Output | `output d` | the dataset's final result |

**Aggregate functions:** `sum`, `avg` (number columns), `count` (non-empty values), `min`, `max`.
Groups are listed in ascending key order.

**Transforms:**
- `calculate_grade` adds `grade` from `marks`: A ≥ 90, B ≥ 80, C ≥ 70, D ≥ 60, else F.
- `uppercase` upper-cases every string column.
- `add_row_number` adds `row_number` 1..n in the current row order.

**Conditions** use fields, number and string literals, parentheses, unary `-`, then `* /`, then
`+ -`, then `> < >= <=`, then `== !=`, then `and`, then `or` (highest to lowest precedence). A
comparison with an empty value is false. Arithmetic with an empty value, or division by zero at run
time, gives an empty value.

**Data:** CSV with a header row. Quoted fields may contain commas, `""` and line breaks. A column is
a number column when every non-empty value is a number; otherwise it is a string column.

## What the compiler checks

- **Lexical and syntax:** unexpected characters, unterminated strings/comments, grammar errors.
- **Semantic (no data needed):**
  - datasets are declared once and before use, and nothing follows their `output`;
  - the pipeline has an output;
  - function names are known;
  - conditions are type-checked: a comparison is required, arithmetic needs numbers, numbers and
    strings can't be compared, comparisons can't be chained, and a literal division by zero is
    rejected.
- **Schema (with `--run`, before anything executes):**
  - every field exists in the dataset at that point, and column types fit the condition;
  - `sum`/`avg` get number columns, and `calculate_grade` finds a number column `marks`;
  - transforms and aggregates change the columns exactly as they do at run time.
- **Warning:** a dataset that is never output.

Every diagnostic has a source line, e.g.
`semantic error at line 3: field 'points' is not a column of dataset 'scores' (columns: name, team, marks, bonus)`.

## Optimizer

| Pass | Rewrite |
|---|---|
| constant-folding | `amount > 50 * 2` → `amount > 100`; `1 < 2 and x` → `x` |
| dead-code | removes datasets that are never output and always-true filters |
| filter-pushdown | moves filters before clean, sort, group and unrelated transforms |
| filter-merge | `filter a` + `filter b` → `filter a and b` |
| redundant-clean | removes a clean when the rows are already clean |
| redundant-sort | removes a sort that a later sort/group on the same field, or an aggregate, makes irrelevant |

Every pass keeps the output byte-for-byte identical. The tests check this by running each example
with and without `-O` and with several threads. `docs/phase3_progress.md` explains why each rewrite
is safe.

## Layout

```text
grammar/   lexer.l (Flex), parser.y (Bison)
include/   public headers, one per module
src/       front end: driver, ast, diagnostic, symbol_table, semantic, ir, dependency, dag
           back end:  value, table (CSV), functions, schema, optimizer, executor, dot, report
           main.cpp (CLI)
tools/     datagen.cpp: benchmark data generator
examples/  sample pipelines, examples/data/*.csv, benchmark/
scripts/   build.sh, demo.sh, benchmark.sh
tests/     valid/, invalid/, runtime/ programs, data/ fixtures, golden/ expected outputs
docs/      phase2_progress.md, phase3_progress.md, devlog.md
```
