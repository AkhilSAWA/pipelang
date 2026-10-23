# Runs pipelangc and checks that its output contains (or does not contain)
# given texts. Used where exact output varies between runs, e.g. timings
# and HTML reports.
#
# Usage (registered by pipelang_check() in tests/CMakeLists.txt):
#   cmake -DEXE=<pipelangc> [-DSRC=<source.pipe>] [-DARGS=<a|b|...>]
#         [-DFILE=<path>] [-DCONTAINS=<t1|t2|...>] [-DNOT_CONTAINS=<t1|...>]
#         -P tests/run_check.cmake
#
# The run must exit with 0. The texts are searched in FILE when given
# (the file must be newly written by this run), otherwise in stdout.

if(NOT DEFINED EXE)
    message(FATAL_ERROR "run_check.cmake requires EXE")
endif()

string(REPLACE "|" ";" arguments "${ARGS}")
if(SRC)
    list(APPEND arguments "${SRC}")
endif()

if(FILE)
    file(REMOVE "${FILE}")
endif()

execute_process(
    COMMAND "${EXE}" ${arguments}
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr
    RESULT_VARIABLE rc)

if(NOT rc EQUAL 0)
    message(FATAL_ERROR "pipelangc ${arguments} exited with ${rc}\n${actual_stderr}")
endif()

if(FILE)
    if(NOT EXISTS "${FILE}")
        message(FATAL_ERROR "expected the run to write ${FILE}")
    endif()
    file(READ "${FILE}" haystack)
else()
    set(haystack "${actual_stdout}")
endif()

set(failures "")
string(REPLACE "|" ";" wanted "${CONTAINS}")
foreach(text IN LISTS wanted)
    string(FIND "${haystack}" "${text}" position)
    if(position EQUAL -1)
        string(APPEND failures "missing: ${text}\n")
    endif()
endforeach()
string(REPLACE "|" ";" unwanted "${NOT_CONTAINS}")
foreach(text IN LISTS unwanted)
    string(FIND "${haystack}" "${text}" position)
    if(NOT position EQUAL -1)
        string(APPEND failures "unexpected: ${text}\n")
    endif()
endforeach()

if(failures)
    message(FATAL_ERROR "pipelangc ${arguments}\n${failures}")
endif()
