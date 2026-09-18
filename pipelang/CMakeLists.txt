cmake_minimum_required(VERSION 3.16)
project(PipeLang
    VERSION 0.1.0
    DESCRIPTION "A DSL compiler for data-processing pipelines"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Standard / warnings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# Flex + Bison
# ---------------------------------------------------------------------------
find_package(FLEX  REQUIRED)
find_package(BISON REQUIRED)

set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

BISON_TARGET(PipeLangParser
    ${CMAKE_SOURCE_DIR}/grammar/parser.y
    ${GEN_DIR}/parser.cpp
    DEFINES_FILE ${GEN_DIR}/parser.hpp)

FLEX_TARGET(PipeLangLexer
    ${CMAKE_SOURCE_DIR}/grammar/lexer.l
    ${GEN_DIR}/lexer.cpp)

ADD_FLEX_BISON_DEPENDENCY(PipeLangLexer PipeLangParser)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(PIPELANG_SOURCES
    src/main.cpp
    src/ast.cpp
    src/driver.cpp
    ${BISON_PipeLangParser_OUTPUTS}
    ${FLEX_PipeLangLexer_OUTPUTS}
)

add_executable(pipelangc ${PIPELANG_SOURCES})

target_include_directories(pipelangc PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GEN_DIR}
)

# ---------------------------------------------------------------------------
# Tests (simple CTest wiring — runs the compiler on example programs)
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME lex_student_analysis
         COMMAND pipelangc ${CMAKE_SOURCE_DIR}/examples/student_analysis.pipe --dump-tokens)
add_test(NAME parse_student_analysis
         COMMAND pipelangc ${CMAKE_SOURCE_DIR}/examples/student_analysis.pipe --dump-ast)
