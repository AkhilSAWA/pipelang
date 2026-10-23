cmake_minimum_required(VERSION 3.20)

project(PipeLang VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PIPELANG_WERROR "Treat compiler warnings as errors" OFF)

# Optimized builds by default: the executor processes real data.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

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
    src/diagnostic.cpp
    src/ast.cpp
    src/symbol_table.cpp
    src/semantic.cpp
    src/ir.cpp
    src/dependency.cpp
    src/dag.cpp
    src/value.cpp
    src/table.cpp
    src/functions.cpp
    src/schema.cpp
    src/optimizer.cpp
    src/executor.cpp
    src/dot.cpp
    src/report.cpp
    ${BISON_PipeLangParser_OUTPUTS}
    ${FLEX_PipeLangLexer_OUTPUTS}
)

target_include_directories(pipelangc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${GENERATED_DIR}"
)

set(PIPELANG_WARNINGS
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<AND:$<BOOL:${PIPELANG_WERROR}>,$<CXX_COMPILER_ID:GNU,Clang>>:-Werror>
)
target_compile_options(pipelangc PRIVATE ${PIPELANG_WARNINGS})

# The executor runs independent DAG nodes on a thread pool.
find_package(Threads REQUIRED)
target_link_libraries(pipelangc PRIVATE Threads::Threads)

# Benchmark data generator (not needed by the compiler itself).
add_executable(pipelang_datagen tools/datagen.cpp)
target_compile_options(pipelang_datagen PRIVATE ${PIPELANG_WARNINGS})

enable_testing()
add_subdirectory(tests)
