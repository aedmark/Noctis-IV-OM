set(root "${CMAKE_CURRENT_BINARY_DIR}/goesnet-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")
file(COPY "${STARMAP}" "${GUIDE}" DESTINATION "${root}/data")
execute_process(COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result ERROR_VARIABLE seed_error)
if(NOT seed_result EQUAL 0)
    message(FATAL_ERROR "could not create GOESnet seed: ${seed_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY "${APP}" --goesnet-fixture
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE report)
set(expected "goesnet_fixture help=p14 dl=p15 data=p22 target=remote+local labels=roundtrip catalog=roundtrip shell=absent interchange=absent legacy_save=absent\n")
if(NOT result EQUAL 0 OR NOT output STREQUAL expected)
    message(FATAL_ERROR "GOESnet application fixture failed with ${result}: ${output} ${report}")
endif()
if(NOT EXISTS "${root}/data/current.niv" OR EXISTS "${root}/data/current.bin"
   OR EXISTS "${root}/data/comm.bin" OR EXISTS "${root}/data/GOESfile.txt")
    message(FATAL_ERROR "GOESnet fixture left a legacy interchange artifact")
endif()
