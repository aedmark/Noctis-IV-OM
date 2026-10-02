if(NOT DEFINED PACKAGE OR NOT EXISTS "${PACKAGE}")
    message(FATAL_ERROR "PACKAGE must name an existing Linux .tar.gz archive")
endif()

get_filename_component(PACKAGE "${PACKAGE}" ABSOLUTE)
get_filename_component(package_directory "${PACKAGE}" DIRECTORY)
set(work "${package_directory}/linux-package-verification")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")
file(ARCHIVE_EXTRACT INPUT "${PACKAGE}" DESTINATION "${work}")

file(GLOB roots LIST_DIRECTORIES true RELATIVE "${work}" "${work}/*")
list(LENGTH roots root_count)
if(NOT root_count EQUAL 1 OR NOT IS_DIRECTORY "${work}/${roots}")
    message(FATAL_ERROR "package must contain exactly one top-level directory: ${roots}")
endif()
set(root "${work}/${roots}")

set(required_files
    nivlr
    README.md
    COMMUNITY_TESTING.md
    KNOWN_ISSUES.md
    LICENSE
    WTOF-LICENSE.md
    CONTRIBUTORS.md
    THIRD_PARTY_NOTICES.md
    licenses/raylib.txt
    licenses/glm.txt
    res/supports.nct
    defaults/STARMAP.BIN
    defaults/GUIDE.BIN)
foreach(path IN LISTS required_files)
    if(NOT EXISTS "${root}/${path}" OR IS_DIRECTORY "${root}/${path}")
        message(FATAL_ERROR "required package file is missing: ${path}")
    endif()
endforeach()

foreach(path IN ITEMS defaults res licenses)
    if(NOT IS_DIRECTORY "${root}/${path}")
        message(FATAL_ERROR "required package directory is missing: ${path}")
    endif()
endforeach()

file(GLOB top_level LIST_DIRECTORIES true RELATIVE "${root}" "${root}/*")
set(expected_top_level
    CONTRIBUTORS.md
    COMMUNITY_TESTING.md
    KNOWN_ISSUES.md
    LICENSE
    README.md
    THIRD_PARTY_NOTICES.md
    WTOF-LICENSE.md
    defaults
    licenses
    nivlr
    res)
list(SORT top_level)
list(SORT expected_top_level)
if(NOT top_level STREQUAL expected_top_level)
    message(FATAL_ERROR
        "unexpected top-level package contents\nexpected: ${expected_top_level}\nactual: ${top_level}")
endif()

file(GLOB_RECURSE forbidden RELATIVE "${root}"
    "${root}/*.EXE"
    "${root}/*.ZIP"
    "${root}/*.cpp"
    "${root}/*.h")
if(forbidden)
    message(FATAL_ERROR "development or legacy files leaked into package: ${forbidden}")
endif()

execute_process(
    COMMAND "${root}/nivlr" --diagnostics
    WORKING_DIRECTORY "${root}"
    RESULT_VARIABLE diagnostics_result
    OUTPUT_VARIABLE diagnostics_out
    ERROR_VARIABLE diagnostics_err)
if(NOT diagnostics_result EQUAL 0)
    message(FATAL_ERROR
        "packaged diagnostics failed (${diagnostics_result})\n${diagnostics_out}\n${diagnostics_err}")
endif()
if(NOT diagnostics_err MATCHES "\\\"preflight\\\".*\\\"passed; graphics and gameplay not tested\\\"")
    message(FATAL_ERROR "packaged diagnostics did not report a passing preflight\n${diagnostics_err}")
endif()

set(profile "${work}/player-profile")
execute_process(
    COMMAND "${root}/nivlr" --prepare-user-data --user-data-dir "${profile}"
    WORKING_DIRECTORY "${work}"
    RESULT_VARIABLE prepare_result
    OUTPUT_VARIABLE prepare_out
    ERROR_VARIABLE prepare_err)
if(NOT prepare_result EQUAL 0)
    message(FATAL_ERROR
        "packaged player-data preparation failed (${prepare_result})\n${prepare_out}\n${prepare_err}")
endif()
foreach(path IN ITEMS data/STARMAP.BIN data/GUIDE.BIN gallery movies config)
    if(NOT EXISTS "${profile}/${path}")
        message(FATAL_ERROR "packaged player-data preparation omitted: ${path}")
    endif()
endforeach()

message(STATUS "verified Linux package: ${PACKAGE}")
