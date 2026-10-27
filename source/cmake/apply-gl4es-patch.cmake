if(NOT DEFINED GL4ES_SOURCE_DIR)
    message(FATAL_ERROR "GL4ES_SOURCE_DIR was not provided")
endif()

if(NOT DEFINED GL4ES_PATCH)
    message(FATAL_ERROR "GL4ES_PATCH was not provided")
endif()

set(GL4ES_ENABLE_C "${GL4ES_SOURCE_DIR}/src/gl/enable.c")

file(READ "${GL4ES_ENABLE_C}" GL4ES_ENABLE_CONTENT)

string(FIND
    "${GL4ES_ENABLE_CONTENT}"
    "proxy_GO(GL_SAMPLE_ALPHA_TO_COVERAGE, sample_alpha_to_coverage);"
    PATCHED_POSITION
)

if(NOT PATCHED_POSITION EQUAL -1)
    message(STATUS "GL4ES alpha-to-coverage patch already applied")
    return()
endif()

execute_process(
    COMMAND git -C "${GL4ES_SOURCE_DIR}"
            apply --whitespace=nowarn "${GL4ES_PATCH}"
    RESULT_VARIABLE PATCH_RESULT
    OUTPUT_VARIABLE PATCH_OUTPUT
    ERROR_VARIABLE PATCH_ERROR
)

if(NOT PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR
        "Failed to apply GL4ES alpha-to-coverage patch:\n"
        "${PATCH_OUTPUT}${PATCH_ERROR}"
    )
endif()

message(STATUS "Applied GL4ES alpha-to-coverage patch")
