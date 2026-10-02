set(root "${CMAKE_CURRENT_BINARY_DIR}/surface-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result
    OUTPUT_VARIABLE seed_output
    ERROR_VARIABLE seed_error)
if(NOT seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create FELYSIA fixture save: ${seed_output} ${seed_error}")
endif()

file(STRINGS "${FIXTURES}" fixtures REGEX "^[^#]")
foreach(fixture IN LISTS fixtures)
    string(REGEX REPLACE " +" ";" fields "${fixture}")
    list(GET fields 0 case_name)
    list(GET fields 1 body_type)
    list(GET fields 2 seed)
    list(GET fields 3 scenario)
    list(GET fields 4 texture_scale)
    list(GET fields 5 height_hash)
    list(GET fields 6 texture_hash)
    list(GET fields 7 object_hash)
    list(GET fields 8 minimum)
    list(GET fields 9 maximum)
    set(expected "surface_fixture case=${case_name} type=${body_type} seed=${seed} scenario=${scenario} texture_scale=${texture_scale} height=${height_hash} texture=${texture_hash} objects=${object_hash} min=${minimum} max=${maximum}\n")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
            "${APP}" --surface-fixture "${case_name}"
        WORKING_DIRECTORY "${root}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE report)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "surface fixture ${case_name} failed with ${result}: ${output} ${report}")
    endif()
    if(NOT output STREQUAL expected)
        message(FATAL_ERROR "surface fixture ${case_name} mismatch:\nexpected: ${expected}actual: ${output}")
    endif()
    if(NOT report MATCHES "\"event\":\"preflight\"")
        message(FATAL_ERROR "surface fixture ${case_name} did not complete startup diagnostics: ${report}")
    endif()
endforeach()
