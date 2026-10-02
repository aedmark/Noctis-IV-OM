set(root "${CMAKE_CURRENT_BINARY_DIR}/movie-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --movie-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)

set(expected "movie_fixture menu=a7f0e8a702f1fa8a space_first=f508ff59599df146 surface_last=8edbd8b71f7b3546 space_frames=3 pause=frozen flashes=both occupied=preserved surface_frames=34 ascent=cutoff\n")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "movie fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "movie fixture mismatch:\nexpected: ${expected}actual: ${output}")
endif()
if(NOT report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "movie fixture did not complete startup diagnostics: ${report}")
endif()
if(NOT EXISTS "${root}/movies/002/keep.txt")
    message(FATAL_ERROR "occupied movie deck was not preserved")
endif()
