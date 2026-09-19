cmake_minimum_required(VERSION 3.20)

project(PipeLang VERSION 0.1.0 LANGUAGES CXX)

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

add_test(
    NAME parse_student_analysis
    COMMAND pipelangc
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/student_analysis.pipe"
)

add_test(
    NAME parse_sales
    COMMAND pipelangc
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.pipe"
)