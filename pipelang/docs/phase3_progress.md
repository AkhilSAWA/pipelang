# PipeLang Phase 3 Progress Report

## Scope

Phase 3 turns the Phase 2 front end, which ends in a validated dependency DAG, into a complete
compiler. It adds:
- checks against the real input columns (schema analysis);
- an optimizer with six passes;
- an execution engine for CSV data, with a parallel scheduler;
- two visual outputs: a Graphviz DOT export and a self-contained HTML report.

Everything is C++20 with no third-party libraries.

## Implemented modules

| Module | File | Purpose |
|---|---|---|
| Values and schemas | `value.hpp/.cpp` | Null / number / string cells, column types, number formatting, ordering |
| CSV tables | `table.hpp/.cpp` | CSV reader (quotes, `""`, line breaks in quotes, CRLF, BOM) with type inference; CSV writer |
| Built-in functions | `functions.hpp/.cpp` | Transform and aggregate registry shared by checker, optimizer and executor |
| Schema analysis | `schema.hpp/.cpp` | Checks fields and column types against the input files before execution |
| Optimizer | `optimizer.hpp/.cpp` | Six output-preserving passes, each logged with a source line |
| Executor | `executor.hpp/.cpp` | Operations, condition evaluator, dependency-driven thread pool, timings |
| Graphviz export | `dot.hpp/.cpp` | DOT file with one cluster per dataset |
| HTML report | `report.hpp/.cpp` | Inline SVG DAG (before/after), optimization log, per-thread timeline, result tables |
| Data generator | `tools/datagen.cpp` | Deterministic large datasets for benchmarking |
| CLI | `main.cpp` | `-O`, `--dump-opt`, `--run`, `--data`, `--out`, `--jobs`, `--timings`, `--emit-dot`, `--report` |

## Compiler flow

1. Front end (Phase 2): lex, parse, semantic analysis, IR, DAG.
2. With `--run`: load every input CSV (in parallel with `--jobs`) and run **schema analysis** on the
   IR as written, so errors point at the user's source lines.
3. With `-O`: **optimize** the IR and rebuild the DAG.
4. Optional DOT export.
5. With `--run`: **execute** on the thread pool, print outputs, optionally write CSV files.
6. Optional HTML report.

## Run-time semantics

These rules are deliberately simple and total, meaning no condition can fail. The optimizer relies
on that.

- **Cells:** an empty CSV field is null. A column is numeric when all its non-empty values are
  numbers.
- **Conditions:**
  - a comparison involving null is false;
  - arithmetic involving null gives null, and so does `x / 0`;
  - `and`/`or` short-circuit.
- **clean:** removes rows with any null, then exact duplicate rows, keeping the first.
- **sort / group:** stable sorts, nulls last. `group` sorts ascending by the key and records it as
  the group key.
- **aggregate:** one row per group in ascending key order (null key last), or one row when not
  grouped.
  - `sum` adds the values in sorted order, so its result does not depend on row order.
  - `avg = sum / count`; `count` counts non-null values.
  - `sum` of no values is 0; `avg`, `min` and `max` of no values are null.
- **Transforms:**
  - `calculate_grade` adds `grade` from `marks`;
  - `uppercase` upper-cases string columns;
  - `add_row_number` numbers rows 1..n.

## Optimizer passes and why each is safe

The optimizer works on each dataset's chain of instructions; dependencies never cross datasets. It
repeats the last four passes until nothing changes. Every change removes an instruction or moves a
filter earlier, so the loop ends.

| Pass | Rule | Why the output cannot change |
|---|---|---|
| constant-folding | Evaluate literal arithmetic and comparisons; `true and x` → `x`, `false and x` → `false`, `x or true` → `true` | Folding uses the same double arithmetic and comparison as the executor, and the folded literal reads back as the identical double. Division by zero is never folded. Dropping one side of `and`/`or` is safe because conditions have no side effects and never fail. |
| dead-code | Remove every instruction of a dataset that is never output; remove filters that are always true | Unused datasets cannot affect any output, and an always-true filter keeps every row. |
| filter-pushdown | Move a filter before `clean`, `sort`, `group`, and before a transform that is row-by-row, ignores row order and writes no field the filter reads | Filtering and stable sorting commute, since both keep the relative order of kept rows. `clean` removes null rows and duplicates; identical rows pass or fail a filter together, so the first copy that survives is the same either way. A transform that doesn't write the filter's fields doesn't change the filter's decision. |
| filter-merge | `filter a; filter b` → `filter a and b` | It keeps exactly the rows where both hold, and `and` skips `b` where `a` is false, as before. |
| redundant-clean | Remove a `clean` if an earlier `clean` was followed only by filter/sort/group | After a clean there are no nulls and no duplicates; dropping or reordering rows keeps that true, so the second clean changes nothing. |
| redundant-sort | Remove a sort on field f if, before anything observes row order (output or `add_row_number`), a sort or group on f follows, or an aggregate follows | A stable sort on f only changes the relative order of rows with different f values. A later sort or group on f decides exactly those pairs again. Filters, cleans, other transforms, and sorts on other fields keep the relative order of the rows they keep. Aggregates do not depend on row order (see `sum` above). |

The optimizer is conservative where a rule would need more information. For example, it never moves
a filter past `uppercase`, past `add_row_number`, or past an aggregate. `tests/valid/opt_barriers.pipe`
checks that nothing moves in those cases.

Results on the examples:
- `sales.pipe`: 7 → 6 instructions. The sort before `group` and `aggregate` is removed, and the
  filter runs before `clean`.
- `store_analytics.pipe`: 18 → 12 instructions, from 8 optimization steps covering all six passes.
- `benchmark.pipe`: 28 → 25 instructions. Three dead sorts are removed and four filters are pushed
  down.

## Parallel execution

Every DAG node becomes a task. A pool of `--jobs` worker threads takes ready tasks from a shared
queue. A task becomes ready as soon as the node it depends on has finished, rather than waiting for
a whole DAG level, so a long chain doesn't hold up the others.

Two nodes that touch the same dataset are always linked by an edge, so no two running tasks ever
share a table. With `--jobs 1`, nodes simply run in program order. Results are identical either way.
The tests check this, and ThreadSanitizer found no data races in parallel runs with 2 to 8 threads.

## Visual output

- **Graphviz DOT** (`--emit-dot`): one dashed cluster per dataset; nodes coloured by operation and
  labelled with id, opcode and details (`(amount > 100)`, `by region`, `sum(amount)`). `scripts/demo.sh`
  renders it to SVG and PNG.
- **HTML report** (`--report`): one self-contained file with inline CSS and SVG, no scripts or
  network access, and light and dark themes. It contains:
  - a summary of counts and timings;
  - the source listing with line numbers;
  - the DAG before and after optimization, drawn as dataset lanes × execution levels;
  - the optimization log and the IR before and after;
  - an execution timeline with one lane per worker thread;
  - node timings, the input files, and the result tables.

## Measured performance

`sh scripts/benchmark.sh 200000 4`: four generated datasets with 200,000 rows each, on a 12-core
machine running Ubuntu 22.04 under WSL, g++ 11.4, Release build. The figures are the median of 3
runs and exclude CSV loading.

| Configuration | Pipeline run time |
|---|---|
| sequential, unoptimized | 1771 ms |
| sequential, optimized (`-O`) | 1076 ms (1.6× faster) |
| 4 threads, unoptimized | 570 ms (3.1× faster) |
| 4 threads, optimized | 408 ms (4.3× faster) |

The numbers vary from machine to machine. The speed-up from threads is bounded by the number of
independent datasets: four here.

## Testing and verification

- **109 CTest cases**, all passing (`tests/README.md` lists them):
  - exact-output golden tests for every stage, the optimizer log, DOT output and execution results;
  - exact-diagnostic tests for compile, schema, input-file and command-line errors;
  - 30 equivalence tests showing that `-O` and `--jobs N` produce byte-identical results;
  - content checks for the HTML report, timings and written files;
  - a Graphviz render of the DOT output, when `dot` is installed.
- **Differential testing** (done during development; the harness is not part of the project): 2,300
  randomly generated valid pipelines with random data. They covered 1–3 datasets, all operations,
  nested arithmetic and logical conditions, nulls and duplicates. Each was run plain, with `-O`, and
  with `-O --jobs 4`, and compared with an independent reference implementation of the semantics
  above. There were 0 mismatches, and every optimizer pass fired hundreds of times.
- **Sanitizers:**
  - the whole suite passes under AddressSanitizer + UndefinedBehaviorSanitizer;
  - parallel runs are clean under ThreadSanitizer.

## Known limitations

- Each pipeline processes datasets independently. There is no join, so threads can only speed up
  pipelines with several datasets.
- Tables are held in memory, so very large inputs need enough RAM.
- The built-in function set is fixed: three transforms and five aggregates.
- Numbers are doubles, so integers above 2^53 lose precision.
