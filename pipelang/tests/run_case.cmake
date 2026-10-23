# Runs pipelangc once and checks the result exactly.
#
# Usage (registered by pipelang_test() in tests/CMakeLists.txt):
#   cmake -DEXE=<pipelangc> [-DSRC=<source.pipe>] [-DARGS=<a|b|...>]
#         [-DSTDOUT=<expected stdout>] [-DSTDERR=<expected stderr>]
#         [-DEXPECT_FAILURE=ON] -P tests/run_case.cmake
#
# - The exit code must be 0, or nonzero when EXPECT_FAILURE is ON.
# - stdout must equal the STDOUT file when one is given.
# - stderr must equal the STDERR file, or be empty when none is given, so
#   unexpected warnings and follow-on errors are caught too.
# ARGS are separated by "|" because ";" would be split by add_test().

if(NOT DEFINED EXE)
    message(FATAL_ERROR "run_case.cmake requires EXE")
endif()

string(REPLACE "|" ";" arguments "${ARGS}")
if(SRC)
    list(APPEND arguments "${SRC}")
endif()

execute_process(
    COMMAND "${EXE}" ${arguments}
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr
    RESULT_VARIABLE rc)

# Compare text independently of the platform's line endings.
function(read_expected path out_var)
    file(READ "${path}" content)
    string(REPLACE "\r\n" "\n" content "${content}")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

string(REPLACE "\r\n" "\n" actual_stdout "${actual_stdout}")
string(REPLACE "\r\n" "\n" actual_stderr "${actual_stderr}")

set(failures "")

if(EXPECT_FAILURE)
    if(rc EQUAL 0)
        string(APPEND failures "expected a nonzero exit code, got 0\n")
    endif()
elseif(NOT rc EQUAL 0)
    string(APPEND failures "expected exit code 0, got ${rc}\n")
endif()

if(STDOUT)
    read_expected("${STDOUT}" expected_stdout)
    if(NOT actual_stdout STREQUAL expected_stdout)
        string(APPEND failures
            "stdout differs from ${STDOUT}\n--- actual stdout ---\n${actual_stdout}\n")
    endif()
endif()

set(expected_stderr "")
if(STDERR)
    read_expected("${STDERR}" expected_stderr)
endif()
if(NOT actual_stderr STREQUAL expected_stderr)
    string(APPEND failures
        "stderr differs from expected\n--- actual stderr ---\n${actual_stderr}\n")
endif()

if(failures)
    message(FATAL_ERROR "${arguments}\n${failures}")
endif()
