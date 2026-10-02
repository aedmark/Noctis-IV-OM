set(root "${CMAKE_CURRENT_BINARY_DIR}/runtime-paths-application")
set(old "${root}/old portable")
set(profile "${root}/new profile")
set(unrelated "${root}/unrelated working directory")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${old}/data" "${old}/gallery" "${old}/movies/007"
     "${profile}/data" "${unrelated}")
file(COPY "${STARMAP}" DESTINATION "${old}/data")
file(COPY "${GUIDE}" DESTINATION "${profile}/data")
file(WRITE "${old}/data/current.niv" "old save remains a backup")
file(WRITE "${profile}/data/current.niv" "destination save wins")
file(WRITE "${old}/gallery/SNAP0042.BMP" "photo")
file(WRITE "${old}/movies/007/00000001.BMP" "movie")
file(SHA256 "${profile}/data/current.niv" save_before)
file(SHA256 "${profile}/data/GUIDE.BIN" guide_before)
file(SHA256 "${old}/data/STARMAP.BIN" map_before)

execute_process(
    COMMAND "${APP}" --prepare-user-data --user-data-dir "${profile}" --migrate-from "${old}"
    WORKING_DIRECTORY "${unrelated}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE report)
if(NOT result EQUAL 0 OR NOT report MATCHES "\"event\":\"runtime_storage\"")
    message(FATAL_ERROR "profile migration command failed (${result}): ${output} ${report}")
endif()

foreach(path IN ITEMS data/current.niv data/STARMAP.BIN data/GUIDE.BIN
                      gallery/SNAP0042.BMP movies/007/00000001.BMP
                      gallery movies config)
    if(NOT EXISTS "${profile}/${path}")
        message(FATAL_ERROR "profile migration omitted ${path}")
    endif()
endforeach()
file(SHA256 "${profile}/data/current.niv" save_after)
file(SHA256 "${profile}/data/GUIDE.BIN" guide_after)
file(SHA256 "${profile}/data/STARMAP.BIN" map_after)
if(NOT save_after STREQUAL save_before OR NOT guide_after STREQUAL guide_before
   OR NOT map_after STREQUAL map_before OR NOT EXISTS "${old}/data/current.niv")
    message(FATAL_ERROR "profile migration overwrote a collision, changed content, or removed its source")
endif()

execute_process(
    COMMAND "${APP}" --prepare-user-data --user-data-dir "${profile}" --migrate-from "${old}"
    WORKING_DIRECTORY "${unrelated}"
    RESULT_VARIABLE repeat_result ERROR_VARIABLE repeat_report)
if(NOT repeat_result EQUAL 0 OR NOT repeat_report MATCHES "copied=0")
    message(FATAL_ERROR "repeated migration was not an idempotent success: ${repeat_report}")
endif()

execute_process(
    COMMAND "${APP}" --diagnostics --user-data-dir "${profile}"
    WORKING_DIRECTORY "${unrelated}"
    RESULT_VARIABLE diagnostics_result ERROR_VARIABLE diagnostics_report)
if(NOT diagnostics_result EQUAL 0
   OR NOT diagnostics_report MATCHES "\"event\":\"resource_directory\""
   OR NOT diagnostics_report MATCHES "\"event\":\"user_data_directory\"")
    message(FATAL_ERROR "unrelated-directory diagnostics failed: ${diagnostics_report}")
endif()

execute_process(
    COMMAND "${APP}" --prepare-user-data --user-data-dir "${profile}"
            --migrate-from "${root}/missing"
    WORKING_DIRECTORY "${unrelated}"
    RESULT_VARIABLE invalid_result ERROR_VARIABLE invalid_report)
if(NOT invalid_result EQUAL 2 OR NOT invalid_report MATCHES "--migrate-from")
    message(FATAL_ERROR "invalid migration source was not rejected: ${invalid_result} ${invalid_report}")
endif()

message(STATUS "runtime path command, migration, preservation, and working-directory independence verified")
