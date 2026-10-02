set(root "${CMAKE_CURRENT_BINARY_DIR}/native-save-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")

execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result
    OUTPUT_VARIABLE seed_output
    ERROR_VARIABLE seed_error)
if(NOT seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create native-save fixture seed: ${seed_output} ${seed_error}")
endif()

set(expected "native_save_fixture version=1 bytes=401 state=restored surface=absent\n")
foreach(pass IN ITEMS legacy native)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
            "${APP}" --native-save-fixture
        WORKING_DIRECTORY "${root}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE report)
    if(NOT result STREQUAL "0" OR NOT output STREQUAL expected)
        message(FATAL_ERROR "${pass} native-save application pass failed: ${result}: ${output} ${report}")
    endif()
    if(NOT EXISTS "${root}/data/current.niv")
        message(FATAL_ERROR "${pass} pass did not produce data/current.niv")
    endif()
    file(SIZE "${root}/data/current.niv" native_size)
    if(NOT native_size STREQUAL "401")
        message(FATAL_ERROR "native save has unexpected size ${native_size}")
    endif()
    if(pass STREQUAL "legacy")
        file(REMOVE "${root}/data/current.bin")
    endif()
endforeach()

execute_process(COMMAND "${PYTHON}" "${SURFACE_SEED_SCRIPT}" "${root}/data/surface.bin" 40
    RESULT_VARIABLE surface_seed_result ERROR_VARIABLE surface_seed_error)
if(NOT surface_seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create legacy surface fixture: ${surface_seed_error}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --native-save-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)
set(surface_expected "native_save_fixture version=1 bytes=401 state=restored surface=migrated-dos-40 bytes=65\n")
if(NOT result STREQUAL "0" OR NOT output STREQUAL surface_expected OR NOT EXISTS "${root}/data/surface.niv")
    message(FATAL_ERROR "legacy surface application migration failed: ${result}: ${output} ${report}")
endif()

file(WRITE "${root}/data/surface.niv" "damaged")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --native-save-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)
if(NOT result STREQUAL "1" OR NOT output STREQUAL "" OR NOT report MATCHES "\"event\":\"surface_save\"")
    message(FATAL_ERROR "damaged native surface save was not rejected safely: ${result}: ${output} ${report}")
endif()
file(REMOVE "${root}/data/surface.niv" "${root}/data/surface.bin")

file(WRITE "${root}/data/current.niv" "damaged")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --native-save-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE report)
if(NOT result STREQUAL "1" OR NOT output STREQUAL "" OR NOT report MATCHES "\"event\":\"native_save\"")
    message(FATAL_ERROR "damaged native save was not rejected safely: ${result}: ${output} ${report}")
endif()

set(blocked_root "${CMAKE_CURRENT_BINARY_DIR}/native-save-blocked-migration-test")
file(REMOVE_RECURSE "${blocked_root}")
file(MAKE_DIRECTORY "${blocked_root}/res" "${blocked_root}/data/current.niv.tmp" "${blocked_root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${blocked_root}/res")
execute_process(
    COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${blocked_root}/data/current.bin"
    RESULT_VARIABLE blocked_seed_result ERROR_VARIABLE blocked_seed_error)
if(NOT blocked_seed_result STREQUAL "0")
    message(FATAL_ERROR "could not create blocked migration seed: ${blocked_seed_error}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
        "${APP}" --native-save-fixture
    WORKING_DIRECTORY "${blocked_root}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE report)
if(NOT result STREQUAL "1" OR NOT output STREQUAL ""
   OR NOT report MATCHES "\"event\":\"legacy_migration\""
   OR EXISTS "${blocked_root}/data/current.niv"
   OR NOT EXISTS "${blocked_root}/data/current.bin"
   OR NOT IS_DIRECTORY "${blocked_root}/data/current.niv.tmp")
    message(FATAL_ERROR "interrupted application migration was not atomic: ${result}: ${output} ${report}")
endif()
