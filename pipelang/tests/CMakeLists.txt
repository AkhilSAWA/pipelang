# PipeLang test suite. Every test runs from the project root, so paths in
# diagnostics and outputs are relative and the same on every machine.

set(ROOT "${PROJECT_SOURCE_DIR}")
set(TESTS "${CMAKE_CURRENT_SOURCE_DIR}")
set(WORK "${CMAKE_CURRENT_BINARY_DIR}/work")
file(MAKE_DIRECTORY "${WORK}")

# pipelang_test(<name> [SOURCE <file>] [ARGS <arg>...]
#               [STDOUT <golden>] [STDERR <golden>] [FAILS])
#
# Runs pipelangc and compares stdout/stderr byte-for-byte with golden files
# (paths relative to tests/). stderr must be empty unless STDERR is given.
# FAILS expects a nonzero exit code.
function(pipelang_test name)
    cmake_parse_arguments(ARG "FAILS" "SOURCE;STDOUT;STDERR" "ARGS" ${ARGN})
    list(JOIN ARG_ARGS "|" joined)
    set(stdout_file "")
    set(stderr_file "")
    if(ARG_STDOUT)
        set(stdout_file "${TESTS}/${ARG_STDOUT}")
    endif()
    if(ARG_STDERR)
        set(stderr_file "${TESTS}/${ARG_STDERR}")
    endif()
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:pipelangc>
            -DSRC=${ARG_SOURCE}
            "-DARGS=${joined}"
            -DSTDOUT=${stdout_file}
            -DSTDERR=${stderr_file}
            -DEXPECT_FAILURE=${ARG_FAILS}
            -P ${TESTS}/run_case.cmake
        WORKING_DIRECTORY ${ROOT})
endfunction()

# pipelang_equivalence(<name> SOURCE <file> BASE <arg>... VARIANT <arg>...)
#
# Both runs must succeed with identical stdout and stderr. Used to show that
# optimization and parallel execution never change a pipeline's results.
function(pipelang_equivalence name)
    cmake_parse_arguments(ARG "" "SOURCE" "BASE;VARIANT" ${ARGN})
    list(JOIN ARG_BASE "|" base)
    list(JOIN ARG_VARIANT "|" variant)
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:pipelangc>
            -DSRC=${ARG_SOURCE}
            "-DBASE=${base}"
            "-DVARIANT=${variant}"
            -P ${TESTS}/run_equivalence.cmake
        WORKING_DIRECTORY ${ROOT})
endfunction()

# pipelang_check(<name> [SOURCE <file>] [ARGS <arg>...] [FILE <path>]
#                [CONTAINS <text>...] [NOT_CONTAINS <text>...])
#
# For output that varies between runs (timings, reports): the run must
# succeed, and stdout (or FILE, when given) must contain / not contain the
# given texts.
function(pipelang_check name)
    cmake_parse_arguments(ARG "" "SOURCE;FILE" "ARGS;CONTAINS;NOT_CONTAINS" ${ARGN})
    list(JOIN ARG_ARGS "|" joined)
    list(JOIN ARG_CONTAINS "|" contains)
    list(JOIN ARG_NOT_CONTAINS "|" not_contains)
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:pipelangc>
            -DSRC=${ARG_SOURCE}
            "-DARGS=${joined}"
            -DFILE=${ARG_FILE}
            "-DCONTAINS=${contains}"
            "-DNOT_CONTAINS=${not_contains}"
            -P ${TESTS}/run_check.cmake
        WORKING_DIRECTORY ${ROOT})
endfunction()

set(STUDENT examples/student_analysis.pipe)
set(SALES examples/sales.pipe)
set(STORE examples/store_analytics.pipe)
set(TEST_DATA --data tests/data)

# ===================================================== Phase 2: front end

pipelang_test(student_summary SOURCE ${STUDENT} STDOUT golden/student_summary.golden)
pipelang_test(student_ast     SOURCE ${STUDENT} ARGS --dump-ast     STDOUT golden/student_ast.golden)
pipelang_test(student_symbols SOURCE ${STUDENT} ARGS --dump-symbols STDOUT golden/student_symbols.golden)
pipelang_test(student_ir      SOURCE ${STUDENT} ARGS --dump-ir      STDOUT golden/student_ir.golden)
pipelang_test(student_dag     SOURCE ${STUDENT} ARGS --dump-dag     STDOUT golden/student_dag.golden)

pipelang_test(sales_summary SOURCE ${SALES} STDOUT golden/sales_summary.golden)
pipelang_test(sales_tokens  SOURCE ${SALES} ARGS --dump-tokens STDOUT golden/sales_tokens.golden)
pipelang_test(sales_ast     SOURCE ${SALES} ARGS --dump-ast    STDOUT golden/sales_ast.golden)
pipelang_test(sales_ir      SOURCE ${SALES} ARGS --dump-ir     STDOUT golden/sales_ir.golden)
pipelang_test(sales_dag     SOURCE ${SALES} ARGS --dump-dag    STDOUT golden/sales_dag.golden)

# Independent datasets must form parallel DAG branches.
pipelang_test(multi_dag SOURCE tests/valid/multi_dataset.pipe ARGS --dump-dag
    STDOUT golden/multi_dag.golden)
# Multiplication binds tighter than addition, which binds tighter than comparison.
pipelang_test(precedence_ir SOURCE tests/valid/precedence.pipe ARGS --dump-ir
    STDOUT golden/precedence_ir.golden)
# `and` binds tighter than `or`; unary minus and string literals are kept.
pipelang_test(logical_ir SOURCE tests/valid/logical.pipe ARGS --dump-ir
    STDOUT golden/logical_ir.golden)
pipelang_test(logical_ast SOURCE tests/valid/logical.pipe ARGS --dump-ast
    STDOUT golden/logical_ast.golden)
# All three comment styles are ignored.
pipelang_test(comments_ast SOURCE tests/valid/comments.pipe ARGS --dump-ast
    STDOUT golden/comments_ast.golden)
# A dataset that is never output compiles with a warning.
pipelang_test(warning_unused_input SOURCE tests/valid/unused_input.pipe
    STDOUT golden/unused_input.golden STDERR golden/unused_input.err)

# Invalid programs: must fail with exactly the expected diagnostics.
foreach(case
        invalid_char unterminated_string unterminated_comment
        missing_where
        use_before_input duplicate_input undeclared_output
        empty_pipeline no_output use_after_output duplicate_output
        unknown_aggregate unknown_transform chained_comparison type_mismatch
        string_arithmetic non_boolean_filter division_by_zero
        multiple_errors)
    pipelang_test(error_${case} SOURCE tests/invalid/${case}.pipe FAILS
        STDERR golden/errors/${case}.err)
endforeach()

# ===================================================== Phase 3: optimizer

pipelang_test(opt_student SOURCE ${STUDENT} ARGS --dump-opt STDOUT golden/opt/student.golden)
pipelang_test(opt_sales   SOURCE ${SALES}   ARGS --dump-opt STDOUT golden/opt/sales.golden)
pipelang_test(opt_store   SOURCE ${STORE}   ARGS --dump-opt STDOUT golden/opt/store.golden
    STDERR golden/store_warning.err)
# Constant folding: arithmetic, comparisons, and/or simplification,
# always-true filters removed, always-false filters reported.
pipelang_test(opt_folding SOURCE tests/valid/opt_folding.pipe ARGS --dump-opt
    STDOUT golden/opt/folding.golden)
# Pushdown past a transform that does not write the filtered field, and a
# sort made redundant by a later sort on the same field.
pipelang_test(opt_rewrites SOURCE tests/valid/opt_rewrites.pipe ARGS --dump-opt
    STDOUT golden/opt/rewrites.golden)
# A sort made redundant by a group on the same field.
pipelang_test(opt_group SOURCE tests/valid/opt_group.pipe ARGS --dump-opt
    STDOUT golden/opt/group.golden)
# Nothing may move: order-sensitive and field-writing transforms, a clean
# after a transform, and sorts observed by output or row numbering.
pipelang_test(opt_barriers SOURCE tests/valid/opt_barriers.pipe ARGS --dump-opt
    STDOUT golden/opt/barriers.golden)
pipelang_test(opt_store_summary SOURCE ${STORE} ARGS -O STDOUT golden/opt/store_summary.golden
    STDERR golden/store_warning.err)
pipelang_test(opt_store_dag SOURCE ${STORE} ARGS -O --dump-dag STDOUT golden/opt/store_dag.golden
    STDERR golden/store_warning.err)

# =============================================== Phase 3: visualization

pipelang_test(dot_store SOURCE ${STORE} ARGS -O --emit-dot - STDOUT golden/store_dag.dot
    STDERR golden/store_warning.err)
pipelang_test(dot_sales SOURCE ${SALES} ARGS --emit-dot - STDOUT golden/sales_dag.dot)
find_program(DOT_EXECUTABLE dot)
if(DOT_EXECUTABLE)
    # The DOT output (pinned by the tests above) must render with Graphviz.
    add_test(NAME dot_renders_with_graphviz
        COMMAND ${DOT_EXECUTABLE} -Tsvg ${TESTS}/golden/store_dag.dot -o ${WORK}/store_dag.svg)
endif()

pipelang_check(report_run SOURCE ${STORE}
    ARGS -O --run --jobs 2 --report ${WORK}/store_report.html
    FILE ${WORK}/store_report.html
    CONTAINS "<title>StoreAnalytics" "Before optimization (18 nodes)"
             "After optimization (12 nodes)" "redundant-sort" "Execution timeline"
             "Node timings" "sum_amount" "IYER, KAVYA" "</html>")
pipelang_check(report_compile_only SOURCE ${SALES}
    ARGS --report ${WORK}/sales_report.html
    FILE ${WORK}/sales_report.html
    CONTAINS "<title>SalesReport" "Dependency DAG" "Intermediate representation" "</html>"
    NOT_CONTAINS "Execution timeline" "After optimization")

# ==================================================== Phase 3: execution

pipelang_test(run_student SOURCE ${STUDENT} ARGS --run STDOUT golden/run/student.golden)
pipelang_test(run_sales   SOURCE ${SALES}   ARGS --run STDOUT golden/run/sales.golden)
pipelang_test(run_store   SOURCE ${STORE}   ARGS --run STDOUT golden/run/store.golden
    STDERR golden/store_warning.err)

# CSV parsing: quoted commas, "" escapes, line breaks in quotes, CRLF,
# blank lines, trimming, empty fields, exponents, numeric-looking strings.
set(DATA_CASES
    csv_edge
    aggregate_sum aggregate_avg aggregate_count aggregate_min_string
    aggregate_max_ungrouped aggregate_empty
    transforms nulls opt_barriers opt_rewrites opt_group)
foreach(case ${DATA_CASES})
    pipelang_test(run_${case} SOURCE tests/valid/${case}.pipe ARGS --run ${TEST_DATA}
        STDOUT golden/run/${case}.golden)
endforeach()

# Optimized and parallel runs must give exactly the unoptimized results.
foreach(example student_analysis sales store_analytics)
    pipelang_equivalence(same_result_optimized_${example} SOURCE examples/${example}.pipe
        BASE --run VARIANT --run -O)
    pipelang_equivalence(same_result_parallel_${example} SOURCE examples/${example}.pipe
        BASE --run VARIANT --run -O --jobs 4)
endforeach()
foreach(case ${DATA_CASES})
    pipelang_equivalence(same_result_optimized_${case} SOURCE tests/valid/${case}.pipe
        BASE --run ${TEST_DATA} VARIANT --run ${TEST_DATA} -O)
    pipelang_equivalence(same_result_parallel_${case} SOURCE tests/valid/${case}.pipe
        BASE --run ${TEST_DATA} VARIANT --run ${TEST_DATA} -O --jobs 3)
endforeach()

pipelang_check(run_writes_output_files SOURCE ${SALES}
    ARGS --run --out ${WORK}/out_sales
    FILE ${WORK}/out_sales/sales.csv
    CONTAINS "region,sum_revenue" "East,3550.75" "West,2300.4")
pipelang_check(run_timings SOURCE ${STORE} ARGS -O --run --jobs 2 --timings
    CONTAINS "Execution timings" "Workers: 2" "Load inputs:" "Run pipeline:" "AGGREGATE")
pipelang_check(help SOURCE "" ARGS --help CONTAINS "Usage:" "--report <file>" "--jobs <n>")

# Errors found before execution, against the real input columns.
foreach(case missing_column column_type sum_string grade_needs_marks field_after_aggregate)
    pipelang_test(schema_${case} SOURCE tests/runtime/${case}.pipe ARGS --run ${TEST_DATA}
        FAILS STDERR golden/runtime/${case}.err)
endforeach()
# Unreadable or malformed input files.
foreach(case missing_file ragged unterminated dupheader)
    pipelang_test(input_${case} SOURCE tests/runtime/${case}.pipe ARGS --run ${TEST_DATA}
        FAILS STDERR golden/runtime/${case}.err)
endforeach()

# Command-line errors.
pipelang_test(usage_bad_jobs SOURCE ${SALES} ARGS --run --jobs abc FAILS
    STDERR golden/usage/bad_jobs.err)
pipelang_test(usage_needs_run SOURCE ${SALES} ARGS --timings FAILS
    STDERR golden/usage/needs_run.err)
pipelang_test(usage_missing_value SOURCE "" ARGS ${SALES} --report FAILS
    STDERR golden/usage/missing_value.err)
