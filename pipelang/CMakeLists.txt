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

add_test(NAME parse_student_analysis COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/examples/student_analysis.pipe")
add_test(NAME parse_sales COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe")
add_test(NAME dump_sales_ir COMMAND pipelangc --dump-ir "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe")
add_test(NAME dump_sales_dag COMMAND pipelangc --dump-dag "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe")

add_test(NAME semantic_use_before_input COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/use_before_input.pipe")
set_tests_properties(semantic_use_before_input PROPERTIES WILL_FAIL TRUE)

add_test(NAME semantic_duplicate_input COMMAND pipelangc "${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/duplicate_input.pipe")
set_tests_properties(semantic_duplicate_input PROPERTIES WILL_FAIL TRUE)
