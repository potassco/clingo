if(NOT DEFINED GIT_EXECUTABLE OR NOT DEFINED SOURCE_DIR OR NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "Missing Git hash script arguments")
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short=8 HEAD
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE git_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE git_result
)

if(NOT git_result EQUAL 0 OR git_hash STREQUAL "")
    set(contents
"#pragma once
")
else()
    set(contents
"#pragma once

#define CLINGO_GIT_HASH ${git_hash}
")
endif()

if(EXISTS "${OUTPUT_FILE}")
    file(READ "${OUTPUT_FILE}" old_contents)
else()
    set(old_contents "")
endif()

if(NOT contents STREQUAL old_contents)
    file(WRITE "${OUTPUT_FILE}" "${contents}")
endif()
