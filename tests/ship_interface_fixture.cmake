set(root "${CMAKE_CURRENT_BINARY_DIR}/ship-interface-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --ship-interface-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)

set(expected "ship_interface_fixture screens=3 menus=4 device_pages=5 hud_panels=3 preferences=7 overlays=2 status=extended save=restored\n")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "ship-interface fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "ship-interface fixture mismatch:\nexpected: ${expected}actual: ${output}")
endif()
if(NOT report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "ship-interface fixture did not complete startup diagnostics: ${report}")
endif()
