# PipeLang test suite

All tests are registered in the top-level `CMakeLists.txt` and run through CTest.

```bash
cd pipelang
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Test inventory (15 cases)

Valid programs (exit 0):
- `parse_student_analysis`, `parse_sales` — smoke tests on the examples.
- `golden_student_ir`, `golden_student_dag`, `golden_student_symbols` —
  exact-output checks for the student example at each stage.
- `golden_sales_ir`, `golden_sales_dag` — exact-output checks for the
  sales example.
- `golden_multi_dag` — two independent datasets must produce parallel
  DAG branches (`tests/valid/multi_dataset.pipe`).
- `golden_precedence_ir` — `a + b * 2 > 10` must lower to
  `((a + (b * 2)) > 10)` (`tests/valid/precedence.pipe`).
- `golden_comments_ast` — `//`, `#`, and `/* */` comments must be
  ignored (`tests/valid/comments.pipe`).

Invalid programs (must fail with a line-accurate diagnostic):
- `semantic_use_before_input` — operation before `input` (line 2).
- `semantic_duplicate_input` — duplicate `input` (line 3).
- `semantic_undeclared_output` — `output` of an undeclared dataset (line 3).
- `lexical_invalid_char` — `@` character (line 3).
- `syntax_missing_where` — missing `where` keyword (line 3).

## Golden-output tests

Golden tests run `pipelangc` through `tests/compare_output.cmake` and
compare its exact output against a file in `tests/golden/`. If a
compiler stage changes its output, the test fails until the golden file
is updated deliberately.

To regenerate a golden file after an intentional change:

```bash
./build/pipelangc --dump-ir examples/student_analysis.pipe > tests/golden/student_ir.golden
```

Review the diff before committing — golden files must be verified by a
human, not just regenerated.
