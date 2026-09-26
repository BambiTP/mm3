cmake_minimum_required(VERSION 3.16)
project(mm3 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MM3_BUILD_FRONTEND "Build the SDL2 frontend (desktop or web)" ON)
option(MM3_FETCH_SDL2 "Download and statically build SDL2 instead of using a system copy" OFF)
option(MM3_BUILD_TESTS "Build tests" ON)
option(MM3_BUILD_TOOLS "Build developer tools (jump calibration)" OFF)

# ---------------------------------------------------------------------------
# Core: the deterministic simulation. Pure C99, no floats, no OS calls.
# ---------------------------------------------------------------------------
add_library(mm3_core STATIC
    core/src/rng.c
    core/src/physics_profiles.c
    core/src/collide.c
    core/src/player_common.c
    core/src/engine_retro.c
    core/src/engine_island.c
    core/src/engine_modern.c
    core/src/objects.c
    core/src/sim.c
)
target_include_directories(mm3_core PUBLIC core/include)

if(MSVC)
    target_compile_options(mm3_core PRIVATE /W4 /WX)
else()
    # -ffp-contract=off / no fast-math are belt-and-braces: core has no floats.
    target_compile_options(mm3_core PRIVATE -Wall -Wextra -Wpedantic -Werror
                                            -fno-fast-math -ffp-contract=off)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(MM3_BUILD_TESTS)
    enable_testing()
    add_executable(test_determinism tests/test_determinism.c)
    target_link_libraries(test_determinism PRIVATE mm3_core)
    add_test(NAME determinism COMMAND test_determinism)
    add_executable(test_moves tests/test_moves.c)
    target_link_libraries(test_moves PRIVATE mm3_core)
    add_test(NAME moves COMMAND test_moves)
endif()

if(MM3_BUILD_TOOLS)
    add_executable(calibrate_jump tools/calibrate_jump.c)
    target_link_libraries(calibrate_jump PRIVATE mm3_core)
endif()

# ---------------------------------------------------------------------------
# SDL2 frontend
# ---------------------------------------------------------------------------
if(MM3_BUILD_FRONTEND)
    add_executable(mm3 platform/sdl2/main.c platform/sdl2/art.c)
    target_link_libraries(mm3 PRIVATE mm3_core)
    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
        target_link_libraries(mm3 PRIVATE m)
    endif()

    if(EMSCRIPTEN)
        set_target_properties(mm3 PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
        target_compile_options(mm3 PRIVATE -sUSE_SDL=2)
        target_link_options(mm3 PRIVATE -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1
            "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html")
        set_property(TARGET mm3 APPEND PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/web/shell.html)
    else()
        if(MM3_FETCH_SDL2)
            include(FetchContent)
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
            set(SDL_TEST OFF CACHE BOOL "" FORCE)
            FetchContent_Declare(SDL2
                URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.8/SDL2-2.30.8.tar.gz)
            FetchContent_MakeAvailable(SDL2)
            target_link_libraries(mm3 PRIVATE SDL2::SDL2main SDL2::SDL2-static)
        else()
            find_package(SDL2 REQUIRED)
            if(TARGET SDL2::SDL2main)
                target_link_libraries(mm3 PRIVATE SDL2::SDL2main)
            endif()
            target_link_libraries(mm3 PRIVATE SDL2::SDL2)
        endif()
        if(WIN32)
            set_target_properties(mm3 PROPERTIES WIN32_EXECUTABLE ON)
        endif()
    endif()
endif()
