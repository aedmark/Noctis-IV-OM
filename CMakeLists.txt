cmake_minimum_required(VERSION 3.21)

project(nivlr VERSION 3.0.0 LANGUAGES C CXX)

# The inherited sources use the widely supported M_PI extension. MSVC keeps
# those constants behind this definition.
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
endif()

# The legacy generators were evaluated without fused multiply-add. Keep source
# expression boundaries meaningful so compatibility-sensitive rounding is not
# changed by target CPU features or compiler defaults.
function(nivlr_require_strict_fp target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /fp:strict)
    endif()
endfunction()

# Linux currently has one explicit, tested presentation backend. Keep the
# bundled GLFW configuration aligned with the documented dependency set rather
# than inheriting backend defaults that may change in a future dependency.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT EMSCRIPTEN)
    set(GLFW_BUILD_X11 ON CACHE BOOL "Build GLFW with the verified X11 backend" FORCE)
    set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build GLFW with native Wayland support" FORCE)
endif()

# The browser build targets WebGL2 so the CRT shader can use GLSL ES 3.00.
if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "raylib platform" FORCE)
    set(GRAPHICS "GRAPHICS_API_OPENGL_ES3" CACHE STRING "raylib graphics API" FORCE)
endif()

# CPM-provided packages (built from source)
include(cmake/CPM.cmake)
CPMAddPackage(
    NAME raylib
    GITHUB_REPOSITORY raysan5/raylib
    GIT_TAG dbc56a87da87d973a9c5baa4e7438a9d20121d28 # Raylib 6.0
)
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # GLM 1.0.3
)


set(SOURCE_FILES
    src/atomic_file.cpp
    src/runtime_paths.cpp
    src/startup_diagnostics.cpp
    src/legacy_numeric.cpp
    src/legacy_prng.cpp
    src/simulation_clock.cpp
    src/engine_state.cpp
    src/input.cpp
    src/input_recording.cpp
    src/indexed_framebuffer.cpp
    src/travel.cpp
    src/native_save.cpp
    src/legacy_save.cpp
    src/ship_interface.cpp
    src/panorama.cpp
    src/plus_controls.cpp
    src/controls_config.cpp
    src/gamepad.cpp
    src/plus_presentation.cpp
    src/audio.cpp
    src/music.cpp
    src/display.cpp
    src/atmospheric_scattering.cpp
    src/stellar_coronal_flares.cpp
    src/upscale.cpp
    src/movie_capture.cpp
    src/video_export.cpp
    src/movie_player.cpp
    src/goesnet_protocol.cpp
    src/goesnet_data.cpp
    src/starmap_exchange.cpp
    src/goesnet_commands.cpp
    src/gallery.cpp
    src/gallery_platform.cpp
    src/gallery_viewer.cpp
    src/flight_log.cpp
    src/bookmarks.cpp
    src/navigation_hud.cpp
    src/noctis.cpp
    src/noctis-1.cpp
    src/noctis-0.cpp
    src/brtl.cpp
    src/star_properties.cpp
    src/system_properties.cpp
    src/galaxy_sector.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
nivlr_require_strict_fp(${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib glm::glm)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shell32 ole32)
endif()

if(EMSCRIPTEN)
    # ASYNCIFY lets the inherited blocking game loops yield to the browser from
    # swapBuffers. Immutable resources and default catalogs are preloaded into
    # the same /res and /defaults layout the desktop packages use; player data
    # lives in an IndexedDB-backed /persistent mount set up by web/pre.js.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sASYNCIFY
        -sASYNCIFY_STACK_SIZE=65536
        -sSTACK_SIZE=1048576
        -sALLOW_MEMORY_GROWTH
        -sINITIAL_MEMORY=134217728
        -sFORCE_FILESYSTEM
        -sEXIT_RUNTIME=0
        -lidbfs.js
        "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js"
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/res@/res"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN@/defaults/STARMAP.BIN"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN@/defaults/GUIDE.BIN")
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
endif()

option(NIVLR_PORTABLE_DEFAULT "Default to portable user data beside executable" OFF)
if(NIVLR_PORTABLE_DEFAULT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NIVLR_PORTABLE_DEFAULT)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_custom_target(verify-content
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_content.py
            --manifest ${CMAKE_CURRENT_SOURCE_DIR}/data/CONTENT_MANIFEST.json
            --source ${CMAKE_CURRENT_SOURCE_DIR}/data
        COMMENT "Verifying pinned starmap and guide content")
    add_test(NAME content_package COMMAND ${CMAKE_COMMAND}
        -DPYTHON=${Python3_EXECUTABLE}
        -DVERIFIER=${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_content.py
        -DMANIFEST=${CMAKE_CURRENT_SOURCE_DIR}/data/CONTENT_MANIFEST.json
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/data
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/content_package_fixture.cmake)

    add_test(NAME startup_diagnostics COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DRESOURCE=${CMAKE_CURRENT_SOURCE_DIR}/res/supports.nct
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/startup_diagnostics.cmake)
    add_executable(brtl_tests tests/brtl_test.cpp src/brtl.cpp)
    target_include_directories(brtl_tests PRIVATE src)
    target_compile_features(brtl_tests PRIVATE cxx_std_20)
    set_target_properties(brtl_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME brtl_determinism COMMAND brtl_tests)

    add_executable(legacy_prng_tests tests/legacy_prng_test.cpp src/legacy_prng.cpp src/brtl.cpp)
    target_include_directories(legacy_prng_tests PRIVATE src)
    target_compile_features(legacy_prng_tests PRIVATE cxx_std_20)
    set_target_properties(legacy_prng_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME legacy_fast_prng_determinism COMMAND legacy_prng_tests)

    add_executable(legacy_numeric_tests tests/legacy_numeric_test.cpp src/legacy_numeric.cpp)
    target_include_directories(legacy_numeric_tests PRIVATE src)
    target_compile_features(legacy_numeric_tests PRIVATE cxx_std_20)
    set_target_properties(legacy_numeric_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME legacy_numeric_semantics COMMAND legacy_numeric_tests)

    add_executable(simulation_clock_tests tests/simulation_clock_test.cpp src/simulation_clock.cpp)
    target_include_directories(simulation_clock_tests PRIVATE src)
    target_compile_features(simulation_clock_tests PRIVATE cxx_std_20)
    set_target_properties(simulation_clock_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME fixed_simulation_clock COMMAND simulation_clock_tests)

    add_executable(engine_state_tests tests/engine_state_test.cpp src/engine_state.cpp)
    target_include_directories(engine_state_tests PRIVATE src)
    target_compile_features(engine_state_tests PRIVATE cxx_std_20)
    set_target_properties(engine_state_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME engine_state COMMAND engine_state_tests)

    add_executable(runtime_paths_tests tests/runtime_paths_test.cpp src/runtime_paths.cpp)
    target_include_directories(runtime_paths_tests PRIVATE src)
    target_compile_features(runtime_paths_tests PRIVATE cxx_std_20)
    set_target_properties(runtime_paths_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    if(WIN32)
        target_link_libraries(runtime_paths_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME runtime_paths COMMAND runtime_paths_tests)
    add_test(NAME runtime_paths_application COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DSTARMAP=${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
        -DGUIDE=${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime_paths_fixture.cmake)

    add_executable(native_save_tests tests/native_save_test.cpp src/native_save.cpp src/atomic_file.cpp)
    target_include_directories(native_save_tests PRIVATE src)
    target_compile_features(native_save_tests PRIVATE cxx_std_20)
    set_target_properties(native_save_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME native_save_round_trip COMMAND native_save_tests)
    add_test(NAME native_save_application COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -DSURFACE_SEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/tests/seed_legacy_surface.py
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/native_save_fixture.cmake)
    add_test(NAME persistence_journey COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSTARMAP=${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
        -DGUIDE=${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/persistence_fixture.cmake)

    add_executable(legacy_save_tests tests/legacy_save_test.cpp src/legacy_save.cpp src/native_save.cpp src/atomic_file.cpp)
    target_include_directories(legacy_save_tests PRIVATE src)
    target_compile_features(legacy_save_tests PRIVATE cxx_std_20)
    set_target_properties(legacy_save_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME legacy_save_import COMMAND legacy_save_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin)

    add_executable(input_mapping_tests tests/input_mapping_test.cpp src/input.cpp src/controls_config.cpp src/gamepad.cpp)
    target_include_directories(input_mapping_tests PRIVATE src)
    target_compile_features(input_mapping_tests PRIVATE cxx_std_20)
    set_target_properties(input_mapping_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(input_mapping_tests PRIVATE raylib)
    add_test(NAME input_mapping COMMAND input_mapping_tests)

    add_executable(ship_interface_tests tests/ship_interface_test.cpp src/ship_interface.cpp)
    target_include_directories(ship_interface_tests PRIVATE src)
    target_compile_features(ship_interface_tests PRIVATE cxx_std_20)
    set_target_properties(ship_interface_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME ship_interface COMMAND ship_interface_tests)
    add_executable(panorama_tests tests/panorama_test.cpp src/panorama.cpp src/atomic_file.cpp)
    target_include_directories(panorama_tests PRIVATE src)
    target_compile_features(panorama_tests PRIVATE cxx_std_20)
    set_target_properties(panorama_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME panorama_regression COMMAND panorama_tests)
    add_executable(gallery_tests tests/gallery_test.cpp src/gallery.cpp src/gallery_platform.cpp)
    target_include_directories(gallery_tests PRIVATE src)
    target_compile_features(gallery_tests PRIVATE cxx_std_20)
    set_target_properties(gallery_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(gallery_tests PRIVATE raylib)
    if(WIN32)
        target_link_libraries(gallery_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME gallery COMMAND gallery_tests ${CMAKE_CURRENT_BINARY_DIR}/gallery-test)
    add_executable(plus_controls_tests tests/plus_controls_test.cpp src/plus_controls.cpp)
    target_include_directories(plus_controls_tests PRIVATE src)
    target_compile_features(plus_controls_tests PRIVATE cxx_std_20)
    set_target_properties(plus_controls_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME plus_controls COMMAND plus_controls_tests)
    add_executable(controls_tests tests/controls_test.cpp src/controls_config.cpp src/gamepad.cpp src/plus_presentation.cpp)
    target_include_directories(controls_tests PRIVATE src)
    target_compile_features(controls_tests PRIVATE cxx_std_20)
    set_target_properties(controls_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(controls_tests PRIVATE raylib)
    add_test(NAME controls COMMAND controls_tests)
    add_executable(gamepad_tests tests/gamepad_test.cpp src/gamepad.cpp src/controls_config.cpp src/input.cpp src/plus_presentation.cpp)
    target_include_directories(gamepad_tests PRIVATE src)
    target_compile_features(gamepad_tests PRIVATE cxx_std_20)
    set_target_properties(gamepad_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(gamepad_tests PRIVATE raylib)
    add_test(NAME gamepad COMMAND gamepad_tests)
    add_executable(plus_presentation_tests tests/plus_presentation_test.cpp src/plus_presentation.cpp)
    target_include_directories(plus_presentation_tests PRIVATE src)
    target_compile_features(plus_presentation_tests PRIVATE cxx_std_20)
    set_target_properties(plus_presentation_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME plus_presentation COMMAND plus_presentation_tests)
    add_executable(audio_tests tests/audio_test.cpp src/audio.cpp src/music.cpp src/gamepad.cpp src/controls_config.cpp)
    target_include_directories(audio_tests PRIVATE src)
    target_compile_features(audio_tests PRIVATE cxx_std_20)
    set_target_properties(audio_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(audio_tests PRIVATE raylib)
    if(WIN32)
        target_link_libraries(audio_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME audio_determinism COMMAND audio_tests)
    add_executable(display_tests tests/display_test.cpp src/display.cpp src/atmospheric_scattering.cpp src/stellar_coronal_flares.cpp src/simulation_clock.cpp src/upscale.cpp src/audio.cpp src/music.cpp src/gamepad.cpp src/controls_config.cpp src/runtime_paths.cpp)
    target_include_directories(display_tests PRIVATE src)
    target_compile_features(display_tests PRIVATE cxx_std_20)
    set_target_properties(display_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(display_tests PRIVATE raylib)
    if(WIN32)
        target_link_libraries(display_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME display COMMAND display_tests)

    add_executable(atmospheric_scattering_tests tests/atmospheric_scattering_test.cpp src/atmospheric_scattering.cpp)
    target_include_directories(atmospheric_scattering_tests PRIVATE src)
    target_compile_features(atmospheric_scattering_tests PRIVATE cxx_std_20)
    set_target_properties(atmospheric_scattering_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME atmospheric_scattering COMMAND atmospheric_scattering_tests)

    add_executable(stellar_coronal_flares_tests tests/stellar_coronal_flares_test.cpp src/stellar_coronal_flares.cpp)
    target_include_directories(stellar_coronal_flares_tests PRIVATE src)
    target_compile_features(stellar_coronal_flares_tests PRIVATE cxx_std_20)
    set_target_properties(stellar_coronal_flares_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME stellar_coronal_flares COMMAND stellar_coronal_flares_tests)
    add_executable(upscale_tests tests/upscale_test.cpp src/upscale.cpp)
    target_include_directories(upscale_tests PRIVATE src)
    target_compile_features(upscale_tests PRIVATE cxx_std_20)
    set_target_properties(upscale_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME upscale COMMAND upscale_tests)
    add_executable(movie_capture_tests tests/movie_capture_test.cpp src/movie_capture.cpp)
    target_include_directories(movie_capture_tests PRIVATE src)
    target_compile_features(movie_capture_tests PRIVATE cxx_std_20)
    set_target_properties(movie_capture_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME movie_capture COMMAND movie_capture_tests
        ${CMAKE_CURRENT_BINARY_DIR}/movie-capture-test)
    add_test(NAME movie_application COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/movie_fixture.cmake)

    add_executable(video_export_tests tests/video_export_test.cpp src/video_export.cpp src/gallery.cpp src/gallery_platform.cpp src/runtime_paths.cpp)
    target_include_directories(video_export_tests PRIVATE src)
    target_compile_features(video_export_tests PRIVATE cxx_std_20)
    set_target_properties(video_export_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(video_export_tests PRIVATE raylib)
    if(WIN32)
        target_link_libraries(video_export_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME video_export COMMAND video_export_tests
        ${CMAKE_CURRENT_BINARY_DIR}/video-export-test)

    add_executable(movie_player_tests tests/movie_player_test.cpp src/movie_player.cpp src/engine_state.cpp
        src/video_export.cpp src/gallery.cpp src/gallery_platform.cpp src/runtime_paths.cpp)
    target_include_directories(movie_player_tests PRIVATE src)
    target_compile_features(movie_player_tests PRIVATE cxx_std_20)
    set_target_properties(movie_player_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(movie_player_tests PRIVATE raylib)
    if(WIN32)
        target_link_libraries(movie_player_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME movie_player COMMAND movie_player_tests
        ${CMAKE_CURRENT_BINARY_DIR}/movie-player-test)

    add_test(NAME ship_interface_application COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ship_interface_fixture.cmake)

    add_executable(goesnet_protocol_tests tests/goesnet_protocol_test.cpp src/goesnet_protocol.cpp)
    target_include_directories(goesnet_protocol_tests PRIVATE src)
    target_compile_features(goesnet_protocol_tests PRIVATE cxx_std_20)
    set_target_properties(goesnet_protocol_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME goesnet_protocol COMMAND goesnet_protocol_tests)

    add_executable(goesnet_data_tests tests/goesnet_data_test.cpp src/goesnet_data.cpp src/atomic_file.cpp)
    target_include_directories(goesnet_data_tests PRIVATE src)
    target_compile_features(goesnet_data_tests PRIVATE cxx_std_20)
    set_target_properties(goesnet_data_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME goesnet_data COMMAND goesnet_data_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
        ${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
        ${CMAKE_CURRENT_BINARY_DIR}/goesnet-starmap-test.bin
        ${CMAKE_CURRENT_BINARY_DIR}/goesnet-guide-test.bin)

    add_executable(flight_log_tests tests/flight_log_test.cpp src/flight_log.cpp)
    target_include_directories(flight_log_tests PRIVATE src)
    target_compile_features(flight_log_tests PRIVATE cxx_std_20)
    set_target_properties(flight_log_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME flight_log COMMAND flight_log_tests)

    add_executable(bookmarks_tests tests/bookmarks_test.cpp src/bookmarks.cpp)
    target_include_directories(bookmarks_tests PRIVATE src)
    target_compile_features(bookmarks_tests PRIVATE cxx_std_20)
    set_target_properties(bookmarks_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME bookmarks COMMAND bookmarks_tests)

    add_executable(navigation_hud_tests tests/navigation_hud_test.cpp src/navigation_hud.cpp)
    target_include_directories(navigation_hud_tests PRIVATE src)
    target_compile_features(navigation_hud_tests PRIVATE cxx_std_20)
    set_target_properties(navigation_hud_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME navigation_hud COMMAND navigation_hud_tests)

    add_executable(goesnet_command_tests tests/goesnet_commands_test.cpp
        src/goesnet_commands.cpp src/goesnet_protocol.cpp src/goesnet_data.cpp
        src/starmap_exchange.cpp
        src/flight_log.cpp src/bookmarks.cpp src/gallery.cpp src/gallery_platform.cpp src/atomic_file.cpp
        src/video_export.cpp src/runtime_paths.cpp
        src/galaxy_sector.cpp src/star_properties.cpp src/system_properties.cpp
        src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(goesnet_command_tests)
    target_include_directories(goesnet_command_tests PRIVATE src)
    target_compile_features(goesnet_command_tests PRIVATE cxx_std_20)
    set_target_properties(goesnet_command_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(goesnet_command_tests PRIVATE raylib)
    if(WIN32)
        target_link_libraries(goesnet_command_tests PRIVATE shell32 ole32)
    endif()
    add_test(NAME goesnet_commands COMMAND goesnet_command_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
        ${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
        ${CMAKE_CURRENT_BINARY_DIR}/goesnet-command-guide.bin
        ${CMAKE_CURRENT_BINARY_DIR}/goesnet-guide-export.txt)

    add_executable(starmap_exchange_tests tests/starmap_exchange_test.cpp
        src/starmap_exchange.cpp src/goesnet_data.cpp src/atomic_file.cpp)
    target_include_directories(starmap_exchange_tests PRIVATE src)
    target_compile_features(starmap_exchange_tests PRIVATE cxx_std_20)
    set_target_properties(starmap_exchange_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME starmap_exchange COMMAND starmap_exchange_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
        ${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
        ${CMAKE_CURRENT_BINARY_DIR}/test-exchange-sm.bin
        ${CMAKE_CURRENT_BINARY_DIR}/test-exchange-gd.bin)
    add_test(NAME goesnet_application COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSTARMAP=${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
        -DGUIDE=${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/goesnet_fixture.cmake)

    add_executable(indexed_framebuffer_tests tests/indexed_framebuffer_test.cpp src/indexed_framebuffer.cpp)
    target_include_directories(indexed_framebuffer_tests PRIVATE src)
    target_compile_features(indexed_framebuffer_tests PRIVATE cxx_std_20)
    set_target_properties(indexed_framebuffer_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME indexed_framebuffer COMMAND indexed_framebuffer_tests)

    add_executable(renderer_fixture_tests tests/renderer_fixture_test.cpp)
    nivlr_require_strict_fp(renderer_fixture_tests)
    target_include_directories(renderer_fixture_tests PRIVATE src)
    target_compile_features(renderer_fixture_tests PRIVATE cxx_std_20)
    set_target_properties(renderer_fixture_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(renderer_fixture_tests PRIVATE glm::glm)
    add_test(NAME renderer_fixtures COMMAND renderer_fixture_tests)

    add_executable(travel_fixture_tests tests/travel_fixture_test.cpp
        src/travel.cpp src/system_properties.cpp src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(travel_fixture_tests)
    target_include_directories(travel_fixture_tests PRIVATE src)
    target_compile_features(travel_fixture_tests PRIVATE cxx_std_20)
    set_target_properties(travel_fixture_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME travel_fixture COMMAND travel_fixture_tests)

    add_executable(scripted_journey_tests tests/scripted_journey_test.cpp
        src/input.cpp src/input_recording.cpp src/atomic_file.cpp src/controls_config.cpp src/gamepad.cpp
        src/travel.cpp src/system_properties.cpp src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(scripted_journey_tests)
    target_include_directories(scripted_journey_tests PRIVATE src)
    target_compile_features(scripted_journey_tests PRIVATE cxx_std_20)
    set_target_properties(scripted_journey_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    target_link_libraries(scripted_journey_tests PRIVATE raylib glm::glm)
    add_test(NAME scripted_journey COMMAND scripted_journey_tests)

    add_executable(input_recording_tests tests/input_recording_test.cpp src/input_recording.cpp src/atomic_file.cpp)
    target_include_directories(input_recording_tests PRIVATE src)
    target_compile_features(input_recording_tests PRIVATE cxx_std_20)
    set_target_properties(input_recording_tests PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME input_recording COMMAND input_recording_tests
        ${CMAKE_CURRENT_BINARY_DIR}/input-recording-test.nirp)

    set(NIVLR_SURFACE_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_surfaces.tsv)
    set(NIVLR_ENVIRONMENT_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_environments.tsv)
    set(NIVLR_CONTENT_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_surface_content.tsv)
    set(NIVLR_PRESENTATION_BASELINE native)
    if(MSVC AND CMAKE_CONFIGURATION_TYPES STREQUAL "Release")
        set(NIVLR_SURFACE_FIXTURES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_surfaces_msvc_release.tsv)
        set(NIVLR_ENVIRONMENT_FIXTURES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_environments_msvc_release.tsv)
        set(NIVLR_CONTENT_FIXTURES
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_surface_content_msvc_release.tsv)
        set(NIVLR_PRESENTATION_BASELINE msvc-release)
    endif()

    add_test(NAME representative_surface_fixtures COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -DFIXTURES=${NIVLR_SURFACE_FIXTURES}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/surface_fixture.cmake)
    set_tests_properties(representative_surface_fixtures PROPERTIES LABELS cross_platform_review)

    add_test(NAME landing_return_fixture COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/landing_fixture.cmake)
    set_tests_properties(landing_return_fixture PROPERTIES TIMEOUT 30)

    add_test(NAME oakenshield_landing_fixture COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/oakenshield_fixture.cmake)
    set_tests_properties(oakenshield_landing_fixture PROPERTIES TIMEOUT 30)

    add_test(NAME surface_environment_fixtures COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -DFIXTURES=${NIVLR_ENVIRONMENT_FIXTURES}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/environment_fixture.cmake)
    set_tests_properties(surface_environment_fixtures PROPERTIES TIMEOUT 60)
    set_tests_properties(surface_environment_fixtures PROPERTIES LABELS cross_platform_review)

    add_test(NAME surface_content_fixtures COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -DFIXTURES=${NIVLR_CONTENT_FIXTURES}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/content_fixture.cmake)
    set_tests_properties(surface_content_fixtures PROPERTIES TIMEOUT 60)
    set_tests_properties(surface_content_fixtures PROPERTIES LABELS cross_platform_review)

    add_test(NAME orbit_surface_journey COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:nivlr>
        -DPYTHON=${Python3_EXECUTABLE}
        -DRESOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/res
        -DSEED_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/seed_felysia_system.py
        -DSEED=${CMAKE_CURRENT_SOURCE_DIR}/docs/porting/fixtures/F01C_MIRACLE/seed-current.bin
        -DBASELINE=${NIVLR_PRESENTATION_BASELINE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/orbit_surface_fixture.cmake)
    set_tests_properties(orbit_surface_journey PROPERTIES TIMEOUT 60)
    set_tests_properties(orbit_surface_journey PROPERTIES LABELS cross_platform_review)

    add_executable(star_fixture_runner tests/star_fixture_runner.cpp
        src/star_properties.cpp src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(star_fixture_runner)
    target_include_directories(star_fixture_runner PRIVATE src)
    target_compile_features(star_fixture_runner PRIVATE cxx_std_20)
    set_target_properties(star_fixture_runner PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME native_star_fixtures COMMAND star_fixture_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_stars.tsv)

    add_executable(galaxy_fixture_runner tests/galaxy_fixture_runner.cpp
        src/galaxy_sector.cpp src/star_properties.cpp src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(galaxy_fixture_runner)
    target_include_directories(galaxy_fixture_runner PRIVATE src)
    target_compile_features(galaxy_fixture_runner PRIVATE cxx_std_20)
    set_target_properties(galaxy_fixture_runner PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME native_galaxy_fixtures COMMAND galaxy_fixture_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_galaxy.tsv)

    add_executable(system_fixture_runner tests/system_fixture_runner.cpp
        src/system_properties.cpp src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(system_fixture_runner)
    target_include_directories(system_fixture_runner PRIVATE src)
    target_compile_features(system_fixture_runner PRIVATE cxx_std_20)
    set_target_properties(system_fixture_runner PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME native_system_fixtures COMMAND system_fixture_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_systems.tsv)

    add_executable(dos_star_reference_test tests/dos_star_reference_test.cpp
        src/galaxy_sector.cpp src/star_properties.cpp src/legacy_numeric.cpp src/brtl.cpp)
    nivlr_require_strict_fp(dos_star_reference_test)
    target_include_directories(dos_star_reference_test PRIVATE src)
    target_compile_features(dos_star_reference_test PRIVATE cxx_std_20)
    set_target_properties(dos_star_reference_test PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
    add_test(NAME dos_adelphe_reference COMMAND dos_star_reference_test adelphe)
    add_test(NAME dos_jehovaboh_reference COMMAND dos_star_reference_test jehovaboh)
    add_test(NAME dos_miracle_reference COMMAND dos_star_reference_test miracle)
    add_test(NAME dos_new_felysia_reference COMMAND dos_star_reference_test new-felysia)

    if(WIN32)
        add_test(NAME windows_graphical_startup COMMAND $<TARGET_FILE:nivlr> --graphical-smoke)
        set_tests_properties(windows_graphical_startup PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS requires_graphics
            TIMEOUT 30)
    endif()
endif()

add_custom_command(TARGET nivlr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:nivlr>/res
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:nivlr>/defaults
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/data
        -DDESTINATION_DIR=$<TARGET_FILE_DIR:nivlr>/defaults
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/InitializeRuntimeData.cmake
    COMMENT "Copying immutable resources and default catalogs"
)

# The preview archive is deliberately a self-contained, user-owned directory.
# Runtime files use relative paths today, so installing them beside the
# executable preserves the same layout exercised by development builds.
install(TARGETS nivlr RUNTIME DESTINATION . COMPONENT Runtime)
install(DIRECTORY res/ DESTINATION res COMPONENT Runtime)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/STARMAP.BIN
    ${CMAKE_CURRENT_SOURCE_DIR}/data/GUIDE.BIN
    DESTINATION defaults
    COMPONENT Runtime)
if(WIN32)
    set(NIVLR_PACKAGE_README ${CMAKE_CURRENT_SOURCE_DIR}/PACKAGE_README_WINDOWS.md)
else()
    set(NIVLR_PACKAGE_README ${CMAKE_CURRENT_SOURCE_DIR}/PACKAGE_README.md)
endif()
install(FILES
    ${NIVLR_PACKAGE_README}
    DESTINATION .
    RENAME README.md
    COMPONENT Runtime)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/COMMUNITY_TESTING.md
    ${CMAKE_CURRENT_SOURCE_DIR}/KNOWN_ISSUES.md
    ${CMAKE_CURRENT_SOURCE_DIR}/TROUBLESHOOTING.md
    DESTINATION .
    COMPONENT Runtime)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION .
    COMPONENT Runtime)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
    DESTINATION .
    RENAME WTOF-LICENSE.md
    COMPONENT Runtime)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTORS.md
    ${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    DESTINATION .
    COMPONENT Runtime)
install(FILES
    ${raylib_SOURCE_DIR}/LICENSE
    DESTINATION licenses
    RENAME raylib.txt
    COMPONENT Runtime)
install(FILES
    ${glm_SOURCE_DIR}/copying.txt
    DESTINATION licenses
    RENAME glm.txt
    COMPONENT Runtime)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32)
    if(WIN32)
        set(CPACK_GENERATOR "ZIP")
        set(CPACK_PACKAGE_FILE_NAME "Noctis-IV-OM-windows-x86_64")
    else()
        set(CPACK_GENERATOR "TGZ")
        set(CPACK_PACKAGE_FILE_NAME
            "Noctis-IV-OM-linux-${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    set(CPACK_PACKAGE_NAME "Noctis-IV-OM")
    set(CPACK_PACKAGE_VENDOR "Noctis IV OM contributors")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Noctis IV OM 3.0.0 Release")
    set(CPACK_PACKAGE_VERSION "3.0.0")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CPACK_PACKAGE_CHECKSUM "SHA256")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/")
    set(CPACK_COMPONENTS_ALL Runtime)
    set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")
    include(CPack)
endif()
