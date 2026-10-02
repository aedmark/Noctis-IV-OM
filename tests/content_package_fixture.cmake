if(NOT DEFINED PYTHON OR NOT DEFINED VERIFIER OR NOT DEFINED MANIFEST OR NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "PYTHON, VERIFIER, MANIFEST, and SOURCE_DIR are required")
endif()

set(stage "${CMAKE_CURRENT_BINARY_DIR}/content-package-fixture")
file(REMOVE_RECURSE "${stage}")

execute_process(
    COMMAND "${PYTHON}" "${VERIFIER}" --manifest "${MANIFEST}" --source "${SOURCE_DIR}" --stage "${stage}"
    RESULT_VARIABLE first_result OUTPUT_VARIABLE first_output ERROR_VARIABLE first_error)
if(NOT first_result EQUAL 0)
    message(FATAL_ERROR "initial content staging failed: ${first_output}${first_error}")
endif()

file(SHA256 "${SOURCE_DIR}/STARMAP.BIN" source_starmap_hash)
file(SHA256 "${stage}/STARMAP.BIN" staged_starmap_hash)
file(SHA256 "${SOURCE_DIR}/GUIDE.BIN" source_guide_hash)
file(SHA256 "${stage}/GUIDE.BIN" staged_guide_hash)
if(NOT source_starmap_hash STREQUAL staged_starmap_hash OR NOT source_guide_hash STREQUAL staged_guide_hash)
    message(FATAL_ERROR "staged content differs from the verified source")
endif()

file(WRITE "${stage}/GUIDE.BIN" "player catalog must survive")
execute_process(
    COMMAND "${PYTHON}" "${VERIFIER}" --manifest "${MANIFEST}" --source "${SOURCE_DIR}" --stage "${stage}"
    RESULT_VARIABLE second_result OUTPUT_VARIABLE second_output ERROR_VARIABLE second_error)
if(NOT second_result EQUAL 0)
    message(FATAL_ERROR "repeat content staging failed: ${second_output}${second_error}")
endif()
file(READ "${stage}/GUIDE.BIN" preserved_guide)
if(NOT preserved_guide STREQUAL "player catalog must survive")
    message(FATAL_ERROR "repeat staging overwrote the player's guide")
endif()

file(REMOVE_RECURSE "${stage}")
message(STATUS "content package verified and existing runtime catalogs preserved")
