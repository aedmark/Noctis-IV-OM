set(root "${CMAKE_CURRENT_BINARY_DIR}/landing-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result
    OUTPUT_VARIABLE seed_output
    ERROR_VARIABLE seed_error)
if(NOT seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create FELYSIA landing fixture save: ${seed_output} ${seed_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --landing-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)

set(expected "landing_fixture request=1 longitude=1 latitude=60 touchdown=488 returned=553 frames=799 ship_position=restored\n")
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "landing fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "landing fixture mismatch:\nexpected: ${expected}actual: ${output}")
endif()
if(NOT report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "landing fixture did not complete startup diagnostics: ${report}")
endif()
if(EXISTS "${root}/data/surface.bin")
    message(FATAL_ERROR "normal capsule return unexpectedly left a surface-resume file")
endif()
if(EXISTS "${root}/data/surface.niv")
    message(FATAL_ERROR "normal capsule return unexpectedly left a native surface-resume file")
endif()
