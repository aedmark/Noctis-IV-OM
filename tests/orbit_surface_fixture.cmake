set(root "${CMAKE_CURRENT_BINARY_DIR}/orbit-surface-fixture-test")
file(REMOVE_RECURSE "${root}")
set(record_root "${root}/record")
set(replay_root "${root}/replay")
file(MAKE_DIRECTORY "${record_root}/res" "${record_root}/data" "${record_root}/gallery")
file(MAKE_DIRECTORY "${replay_root}/res" "${replay_root}/data" "${replay_root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${record_root}/res")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${replay_root}/res")

execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${record_root}/data/current.bin"
    RESULT_VARIABLE seed_result
    OUTPUT_VARIABLE seed_output
    ERROR_VARIABLE seed_error)
if(NOT seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create orbit-to-surface fixture save: ${seed_output} ${seed_error}")
endif()
execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${replay_root}/data/current.bin"
    RESULT_VARIABLE replay_seed_result
    OUTPUT_VARIABLE replay_seed_output
    ERROR_VARIABLE replay_seed_error)
if(NOT replay_seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create replay fixture save: ${replay_seed_output} ${replay_seed_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --orbit-surface-fixture --record-input "${root}/journey.nirp"
    WORKING_DIRECTORY "${record_root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --orbit-surface-fixture --replay-input "${root}/journey.nirp"
        --fixture-presentation-interval 4
    WORKING_DIRECTORY "${replay_root}"
    RESULT_VARIABLE replay_result
    OUTPUT_VARIABLE replay_output
    ERROR_VARIABLE replay_report)

if(BASELINE STREQUAL "msvc-release")
    set(expected "orbit_surface_fixture longitude=1 latitude=60 touchdown=488 returned=538 frames=783 rendered=817 orbit=dd14fcc6528cab25 ground=3bb49530cf858373 outbound=c89aedbf17614669 capsule=97213ad051e1c155 trees=3590648 animals=2133 ruins=35221 capsule_draws=79 ship_position=restored\n")
    set(replay_expected "orbit_surface_fixture longitude=1 latitude=60 touchdown=488 returned=538 frames=783 rendered=205 orbit=dd14fcc6528cab25 ground=3bb49530cf858373 outbound=c89aedbf17614669 capsule=97213ad051e1c155 trees=3590648 animals=2133 ruins=35221 capsule_draws=79 ship_position=restored\n")
else()
    set(expected "orbit_surface_fixture longitude=1 latitude=60 touchdown=488 returned=538 frames=783 rendered=817 orbit=dd14fcc6528cab25 ground=fcf1004a13b0cb28 outbound=de202f968cbb2b26 capsule=4be1dac247ffb4f9 trees=3590648 animals=2133 ruins=35221 capsule_draws=79 ship_position=restored\n")
    set(replay_expected "orbit_surface_fixture longitude=1 latitude=60 touchdown=488 returned=538 frames=783 rendered=205 orbit=dd14fcc6528cab25 ground=fcf1004a13b0cb28 outbound=de202f968cbb2b26 capsule=4be1dac247ffb4f9 trees=3590648 animals=2133 ruins=35221 capsule_draws=79 ship_position=restored\n")
endif()
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "orbit-to-surface fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "orbit-to-surface fixture mismatch:\nexpected: ${expected}actual: ${output}")
endif()
if(NOT replay_result STREQUAL "0")
    message(FATAL_ERROR "orbit-to-surface replay failed with ${replay_result}: ${replay_output} ${replay_report}")
endif()
if(NOT replay_output STREQUAL replay_expected)
    message(FATAL_ERROR "orbit-to-surface replay mismatch:\nexpected: ${replay_expected}actual: ${replay_output}")
endif()
if(NOT report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "orbit-to-surface fixture did not complete startup diagnostics: ${report}")
endif()
if(NOT replay_report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "orbit-to-surface replay did not complete startup diagnostics: ${replay_report}")
endif()
if(NOT EXISTS "${root}/journey.nirp")
    message(FATAL_ERROR "orbit-to-surface fixture did not publish its input recording")
endif()
if(EXISTS "${record_root}/data/surface.bin" OR EXISTS "${replay_root}/data/surface.bin")
    message(FATAL_ERROR "normal orbit-to-surface return unexpectedly left a surface-resume file")
endif()
if(EXISTS "${record_root}/data/surface.niv" OR EXISTS "${replay_root}/data/surface.niv")
    message(FATAL_ERROR "normal orbit-to-surface return unexpectedly left a native surface-resume file")
endif()
