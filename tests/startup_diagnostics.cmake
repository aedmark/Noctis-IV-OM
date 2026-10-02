# Exercise the actual application before graphics initialization, in isolated roots.
set(root "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-test")
file(MAKE_DIRECTORY "${root}/valid/res" "${root}/valid/data" "${root}/valid/gallery" "${root}/missing" "${root}/empty/res")
file(COPY "${RESOURCE}" DESTINATION "${root}/valid/res")
file(WRITE "${root}/empty/res/supports.nct" "")
foreach(case IN ITEMS valid missing empty)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "NOCTIS_IV_OM_HOME=${root}/${case}/user"
        "NOCTIS_IV_OM_RESOURCE_DIR=${root}/${case}/res"
        "${APP}" --diagnostics WORKING_DIRECTORY "${root}/${case}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE report)
    if(case STREQUAL "valid")
        set(expected 0)
    else()
        set(expected 1)
    endif()
    if(NOT result STREQUAL "${expected}")
        message(FATAL_ERROR "${case}: expected ${expected}, got ${result}: ${report}")
    endif()
    if(NOT output STREQUAL "" OR NOT report MATCHES "\"event\":\"preflight\"")
        message(FATAL_ERROR "${case}: missing report or unexpected stdout: ${output} ${report}")
    endif()
    # Compiler descriptions contain semicolons, so escape them before splitting lines.
    string(REPLACE ";" "\\;" lines "${report}")
    string(REPLACE "\n" ";" lines "${lines}")
    foreach(line IN LISTS lines)
        if(NOT line STREQUAL "")
            string(JSON event ERROR_VARIABLE error GET "${line}" event)
            if(error)
                message(FATAL_ERROR "Invalid JSON diagnostic: ${line}: ${error}")
            endif()
        endif()
    endforeach()
    file(GLOB_RECURSE writes "${root}/${case}/user/*")
    if(writes)
        message(FATAL_ERROR "Diagnostics wrote runtime data: ${writes}")
    endif()
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    "NOCTIS_IV_OM_RESOURCE_DIR=${root}/missing/res" "${APP}" --invalid
    WORKING_DIRECTORY "${root}/missing"
    RESULT_VARIABLE result ERROR_VARIABLE report)
if(NOT result STREQUAL "2" OR NOT report MATCHES "Usage:")
    message(FATAL_ERROR "Invalid argument did not produce usage/exit 2: ${report}")
endif()
