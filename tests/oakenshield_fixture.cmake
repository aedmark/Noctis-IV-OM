set(root "${CMAKE_CURRENT_BINARY_DIR}/oakenshield-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --oakenshield-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)

if(NOT result STREQUAL "0")
    message(FATAL_ERROR "oakenshield fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT output MATCHES "^oakenshield_fixture touchdown=[0-9]+ sky_stars=ok terrain=ok resume=ok palette=valid abort=ok status=ok")
    message(FATAL_ERROR "oakenshield fixture unexpected output:\n${output}\nreport:\n${report}")
endif()
if(NOT report MATCHES "\"event\":\"preflight\"")
    message(FATAL_ERROR "oakenshield fixture did not complete startup diagnostics: ${report}")
endif()
