cmake_minimum_required(VERSION 3.20)

project(PipeLang VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

BISON_TARGET(PipeLangParser
    "${CMAKE_CURRENT_SOURCE_DIR}/grammar/parser.y"
    "${GENERATED_DIR}/parser.cpp"
    DEFINES_FILE "${GENERATED_DIR}/parser.hpp"
)

FLEX_TARGET(PipeLangLexer
    "${CMAKE_CURRENT_SOURCE_DIR}/grammar/lexer.l"
    "${GENERATED_DIR}/lexer.cpp"
)

ADD_FLEX_BISON_DEPENDENCY(PipeLangLexer PipeLangParser)

add_executable(pipelangc
    src/main.cpp
    src/driver.cpp
    src/ast.cpp
    src/symbol_table.cpp
    src/semantic.cpp
    src/ir.cpp
    src/dependency.cpp
    src/dag.cpp
    ${BISON_PipeLangParser_OUTPUTS}
    ${FLEX_PipeLangLexer_OUTPUTS}
)

target_include_directories(pipelangc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${GENERATED_DIR}"
)

target_compile_options(pipelangc PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

enable_testing()

# Golden-output test: runs pipelangc and compares its output against
# tests/golden/<name>.golden, so changes in any stage are caught.
function(pipelang_golden_test name flag source golden)
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:pipelangc>
            -DFLAG=${flag}
            -DSRC=${source}
            -DGOLDEN=${golden}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()

# Valid programs: parsing plus exact-output checks per stage.
add_test(NAME parse_student_analysis COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/examples/student_analysis.pipe")
add_test(NAME parse_sales COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe")

pipelang_golden_test(golden_student_ir --dump-ir
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/student_analysis.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/student_ir.golden")
pipelang_golden_test(golden_student_dag --dump-dag
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/student_analysis.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/student_dag.golden")
pipelang_golden_test(golden_student_symbols --dump-symbols
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/student_analysis.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/student_symbols.golden")
pipelang_golden_test(golden_sales_ir --dump-ir
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/sales_ir.golden")
pipelang_golden_test(golden_sales_dag --dump-dag
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/sales_dag.golden")

# Independent datasets must form parallel DAG branches.
pipelang_golden_test(golden_multi_dag --dump-dag
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/valid/multi_dataset.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/multi_dag.golden")
# Multiplication must bind tighter than addition and comparisons.
pipelang_golden_test(golden_precedence_ir --dump-ir
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/valid/precedence.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/precedence_ir.golden")
# All three comment styles must be ignored.
pipelang_golden_test(golden_comments_ast --dump-ast
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/valid/comments.pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/comments_ast.golden")

# Invalid programs: must fail with a line-accurate diagnostic.
add_test(NAME semantic_use_before_input COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/use_before_input.pipe")
set_tests_properties(semantic_use_before_input PROPERTIES PASS_REGULAR_EXPRESSION "semantic error at line 2")

add_test(NAME semantic_duplicate_input COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/duplicate_input.pipe")
set_tests_properties(semantic_duplicate_input PROPERTIES PASS_REGULAR_EXPRESSION "semantic error at line 3")

add_test(NAME semantic_undeclared_output COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/undeclared_output.pipe")
set_tests_properties(semantic_undeclared_output PROPERTIES PASS_REGULAR_EXPRESSION "semantic error at line 3")

add_test(NAME lexical_invalid_char COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/invalid_char.pipe")
set_tests_properties(lexical_invalid_char PROPERTIES PASS_REGULAR_EXPRESSION "line 3: unexpected character '@'")

add_test(NAME syntax_missing_where COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/missing_where.pipe")
set_tests_properties(syntax_missing_where PROPERTIES PASS_REGULAR_EXPRESSION "line 3: syntax error")
