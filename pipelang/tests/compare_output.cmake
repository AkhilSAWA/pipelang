# Compares pipelangc output against a golden file.
#
# Usage (as a CTest case):
#   cmake -DEXE=<pipelangc> -DFLAG=<--dump-...> -DSRC=<source.pipe>
#         -DGOLDEN=<expected output> -P tests/compare_output.cmake
#
# Fails when the exit code is nonzero or the exact output differs.

if(NOT DEFINED EXE OR NOT DEFINED FLAG OR NOT DEFINED SRC OR NOT DEFINED GOLDEN)
    message(FATAL_ERROR "compare_output.cmake requires EXE, FLAG, SRC, GOLDEN")
endif()

execute_process(
    COMMAND "${EXE}" "${FLAG}" "${SRC}"
    OUTPUT_VARIABLE actual
    RESULT_VARIABLE rc)

if(NOT rc EQUAL 0)
    message(FATAL_ERROR "pipelangc exited with code ${rc}")
endif()

file(READ "${GOLDEN}" expected)

if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "golden output mismatch for ${FLAG} ${SRC}")
endif()
