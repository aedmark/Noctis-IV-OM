set(root "${CMAKE_CURRENT_BINARY_DIR}/persistence-fixture-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/res" "${root}/data" "${root}/gallery")
file(COPY "${RESOURCE_DIR}/" DESTINATION "${root}/res")
file(COPY "${STARMAP}" "${GUIDE}" DESTINATION "${root}/data")
execute_process(COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
    RESULT_VARIABLE seed_result ERROR_VARIABLE seed_error)
if(NOT seed_result EQUAL 0)
    message(FATAL_ERROR "could not create persistence seed: ${seed_error}")
endif()

# Every phase sees the same universe clock. unfreeze() charges the internal
# lamp (and other active systems) for the wall-clock seconds between a save and
# the next load, so a free-running clock makes clean-restart report 19999 power
# whenever the two processes straddle a second boundary.
foreach(phase IN ITEMS advance verify deplete standard clean-start clean-restart legacy-unsynced)
    if(phase STREQUAL "advance")
        set(drive_arg "--omega-drive")
    elseif(phase STREQUAL "standard")
        set(drive_arg "--standard-drive")
    else()
        set(drive_arg)
    endif()
    if(phase STREQUAL "clean-start")
        file(REMOVE "${root}/data/current.niv" "${root}/data/current.bin")
    elseif(phase STREQUAL "legacy-unsynced")
        file(REMOVE "${root}/data/current.niv")
        execute_process(COMMAND "${PYTHON}" "${SEED_SCRIPT}" "${SEED}" "${root}/data/current.bin"
            RESULT_VARIABLE legacy_seed_result ERROR_VARIABLE legacy_seed_error)
        if(NOT legacy_seed_result EQUAL 0)
            message(FATAL_ERROR "could not re-seed current.bin: ${legacy_seed_error}")
        endif()
        execute_process(COMMAND "${PYTHON}" -c "
import struct
with open('${root}/data/current.bin', 'r+b') as f:
    f.seek(6)
    f.write(struct.pack('<b', 120))
    f.seek(27)
    f.write(struct.pack('<h', 20000))
    f.seek(235)
    f.write(struct.pack('<d', 28.16))
"           RESULT_VARIABLE patch_result ERROR_VARIABLE patch_error)
        if(NOT patch_result EQUAL 0)
            message(FATAL_ERROR "could not patch current.bin with unsynced secs: ${patch_error}")
        endif()
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
            "${APP}" --persistence-fixture "${phase}" ${drive_arg}
            --fixture-universe-seconds 1000000000
        WORKING_DIRECTORY "${root}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE report)
    if(phase STREQUAL "advance")
        set(expected "persistence_fixture phase=advance remote=balas local=felysia preferences=4 panel=3 omega=on plus=restored\n")
    elseif(phase STREQUAL "verify")
        set(expected "persistence_fixture phase=verify remote=balas local=felysia preferences=4 panel=3 omega=on plus=restored\n")
    elseif(phase STREQUAL "deplete")
        set(expected "persistence_fixture phase=deplete power=15000 lithium=0 save=stored\n")
    elseif(phase STREQUAL "standard")
        set(expected "persistence_fixture phase=standard power=20000 lithium=120 save=restored\n")
    elseif(phase STREQUAL "clean-start")
        set(expected "persistence_fixture phase=clean-start power=20000 lithium=120 save=stored\n")
    elseif(phase STREQUAL "clean-restart")
        set(expected "persistence_fixture phase=clean-restart power=20000 lithium=120 save=restored\n")
    elseif(phase STREQUAL "legacy-unsynced")
        set(expected "persistence_fixture phase=legacy-unsynced power=20000 lithium=120 save=restored\n")
    endif()
    if(NOT result EQUAL 0 OR NOT output STREQUAL expected)
        message(FATAL_ERROR "persistence ${phase} failed with ${result}: ${output} ${report}")
    endif()
    if(NOT EXISTS "${root}/data/current.niv")
        message(FATAL_ERROR "persistence ${phase} did not retain native state")
    endif()
    if(phase STREQUAL "advance" OR phase STREQUAL "legacy-unsynced")
        file(REMOVE "${root}/data/current.bin")
    endif()
endforeach()

file(SIZE "${root}/data/current.niv" native_size)
if(NOT native_size EQUAL 401 OR EXISTS "${root}/data/current.niv.tmp")
    message(FATAL_ERROR "persistence journey left an invalid or temporary save")
endif()
