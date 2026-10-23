# PipeLang test suite

All tests are defined in `tests/CMakeLists.txt` and run through CTest from the project root:

```bash
cd pipelang
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Kinds of test

| Helper | Script | Checks |
|---|---|---|
| `pipelang_test` | `run_case.cmake` | Exit code, plus **exact** stdout and stderr against golden files. stderr must be empty unless a golden file is given, so unexpected warnings and follow-on errors fail the test. |
| `pipelang_equivalence` | `run_equivalence.cmake` | Two runs with different options (e.g. plain vs `-O --jobs 4`) must both succeed with byte-identical output. |
| `pipelang_check` | `run_check.cmake` | For output that varies between runs (timings, HTML reports, written files): the run succeeds and the output contains, or does not contain, given texts. |

Tests run from the project root, so file paths in diagnostics are relative and identical on every
machine.

## Inventory (109 cases; 108 without Graphviz)

**Phase 2: front end**
- `student_*`, `sales_*`: the examples at every stage (summary, tokens, AST, symbols, IR, DAG).
- `multi_dag`, `precedence_ir`, `logical_ir`, `logical_ast`, `comments_ast`,
  `warning_unused_input`.
- `error_*`: 19 invalid programs with exact diagnostics:
  - lexical: bad character, unterminated string or comment;
  - syntax;
  - dataset usage;
  - empty pipeline or no output;
  - unknown aggregate or transform;
  - condition types;
  - several errors at once.

**Phase 3: optimizer**
- `opt_student`, `opt_sales`, `opt_store`: optimization log and optimized IR of the examples.
- `opt_folding`:
  - arithmetic and comparison folding;
  - `and`/`or` simplification;
  - unary minus;
  - always-true filter removed, always-false filter reported.
- `opt_rewrites`: pushdown past an unrelated transform; a sort made redundant by a later sort.
- `opt_group`: a sort made redundant by a group on the same field.
- `opt_barriers`: nothing may move (order-sensitive and field-writing transforms, a clean after a
  transform, sorts observed by output).
- `opt_store_summary`, `opt_store_dag`: summary and DAG after optimization.

**Phase 3: visualization**
- `dot_store`, `dot_sales`: exact Graphviz output.
- `dot_renders_with_graphviz`: the DOT output renders with `dot` (only when Graphviz is installed).
- `report_run`, `report_compile_only`: the HTML report has the expected sections, and only the
  sections that apply.

**Phase 3: execution**
- `run_student`, `run_sales`, `run_store`: results of the examples.
- `run_csv_edge`: quoted commas, `""` escapes, line breaks in quotes, CRLF, blank lines, trimming,
  empty fields, exponents, numeric-looking strings.
- `run_aggregate_*`: sum, avg, count, string min, ungrouped max, aggregate of no rows; null group
  keys.
- `run_transforms`, `run_nulls`: all transforms; null handling and division by zero in conditions.
- `run_opt_*`: results of the optimizer test programs.
- `same_result_optimized_*`, `same_result_parallel_*`: 30 equivalence tests. Every runnable program
  gives identical results with `-O` and with several threads.
- `run_writes_output_files`, `run_timings`, `help`.
- `schema_*`: unknown field, string compared with a number, `sum` of a string column,
  `calculate_grade` without `marks`, a field removed by an aggregate.
- `input_*`: missing file, wrong field count, unterminated quote, duplicate header.
- `usage_*`: bad `--jobs` value, an option that needs `--run`, a missing option value.

## Adding or updating a golden file

```bash
./build/pipelangc --dump-ir examples/student_analysis.pipe > tests/golden/student_ir.golden
./build/pipelangc tests/invalid/no_output.pipe 2> tests/golden/errors/no_output.err
./build/pipelangc --run --data tests/data tests/valid/nulls.pipe > tests/golden/run/nulls.golden
```

Run these from the `pipelang/` directory. Review the diff before committing: a golden file must be
checked by a person, not just regenerated.
