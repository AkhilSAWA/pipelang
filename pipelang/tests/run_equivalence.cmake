# Runs pipelangc twice and requires identical results.
#
# Usage (registered by pipelang_equivalence() in tests/CMakeLists.txt):
#   cmake -DEXE=<pipelangc> -DSRC=<source.pipe> -DBASE=<a|b|...>
#         -DVARIANT=<a|b|...> -P tests/run_equivalence.cmake
#
# Both runs must exit with 0 and produce byte-identical stdout and stderr,
# and the output must not be empty.

if(NOT DEFINED EXE OR NOT DEFINED SRC)
    message(FATAL_ERROR "run_equivalence.cmake requires EXE and SRC")
endif()

function(run_pipelang label argument_text)
    string(REPLACE "|" ";" arguments "${argument_text}")
    execute_process(
        COMMAND "${EXE}" ${arguments} "${SRC}"
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "${label} run (${arguments}) exited with ${rc}\n${err}")
    endif()
    set(${label}_stdout "${out}" PARENT_SCOPE)
    set(${label}_stderr "${err}" PARENT_SCOPE)
endfunction()

run_pipelang(base "${BASE}")
run_pipelang(variant "${VARIANT}")

if(base_stdout STREQUAL "")
    message(FATAL_ERROR "the base run produced no output")
endif()
if(NOT base_stdout STREQUAL variant_stdout)
    message(FATAL_ERROR "stdout differs\n--- ${BASE} ---\n${base_stdout}\n--- ${VARIANT} ---\n${variant_stdout}")
endif()
if(NOT base_stderr STREQUAL variant_stderr)
    message(FATAL_ERROR "stderr differs\n--- ${BASE} ---\n${base_stderr}\n--- ${VARIANT} ---\n${variant_stderr}")
endif()
