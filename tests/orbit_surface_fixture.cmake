set(root "${CMAKE_CURRENT_BINARY_DIR}/orbit-surface-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result
    OUTPUT_VARIABLE seed_output
    ERROR_VARIABLE seed_error)
if(NOT seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create orbit-to-surface fixture save: ${seed_output} ${seed_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --orbit-surface-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)

if(BASELINE STREQUAL "msvc-release")
    set(expected "orbit_surface_fixture longitude=1 latitude=60 touchdown=488 returned=538 frames=783 rendered=817 orbit=dd14fcc6528cab25 ground=6d3807b8549045eb outbound=bbab7679df8dcc21 capsule=71ebd1f6440828fd trees=3590648 animals=2133 ruins=35221 capsule_draws=79 ship_position=restored\n")
else()
    set(expected "orbit_surface_fixture longitude=1 latitude=60 touchdown=488 returned=538 frames=783 rendered=817 orbit=dd14fcc6528cab25 ground=fcf1004a13b0cb28 outbound=de202f968cbb2b26 capsule=4be1dac247ffb4f9 trees=3590648 animals=2133 ruins=35221 capsule_draws=79 ship_position=restored\n")
endif()
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "orbit-to-surface fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "orbit-to-surface fixture mismatch:\nexpected: ${expected}actual: ${output}")
endif()
if(NOT report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "orbit-to-surface fixture did not complete startup diagnostics: ${report}")
endif()
if(EXISTS "${root}/data/surface.bin")
    message(FATAL_ERROR "normal orbit-to-surface return unexpectedly left a surface-resume file")
endif()
if(EXISTS "${root}/data/surface.niv")
    message(FATAL_ERROR "normal orbit-to-surface return unexpectedly left a native surface-resume file")
endif()
