set(root "${CMAKE_CURRENT_BINARY_DIR}/environment-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result
    OUTPUT_VARIABLE seed_output
    ERROR_VARIABLE seed_error)
if(NOT seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create environment fixture save: ${seed_output} ${seed_error}")
endif()

file(STRINGS "${FIXTURES}" fixtures REGEX "^[^#]")
foreach(fixture IN LISTS fixtures)
    string(REGEX REPLACE " +" ";" fields "${fixture}")
    list(GET fields 0 case_name)
    list(GET fields 1 longitude)
    list(GET fields 2 latitude)
    list(GET fields 3 scenario)
    list(GET fields 4 rainy)
    list(GET fields 5 waves_in)
    list(GET fields 6 waves_out)
    list(GET fields 7 frame_hash)
    set(expected "environment_fixture case=${case_name} longitude=${longitude} latitude=${latitude} scenario=${scenario} rainy=${rainy} waves=${waves_in}/${waves_out} frame=${frame_hash}\n")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
            "${APP}" --environment-fixture "${case_name}" "${longitude}" "${latitude}"
        WORKING_DIRECTORY "${root}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE report)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "environment fixture ${case_name} ${longitude}:${latitude} failed with ${result}: ${output} ${report}")
    endif()
    if(NOT output STREQUAL expected)
        message(FATAL_ERROR "environment fixture ${case_name} ${longitude}:${latitude} mismatch:\nexpected: ${expected}actual: ${output}")
    endif()
    if(NOT report MATCHES "\"event\":\"preflight\"")
        message(FATAL_ERROR "environment fixture ${case_name} ${longitude}:${latitude} did not complete startup diagnostics: ${report}")
    endif()
endforeach()
