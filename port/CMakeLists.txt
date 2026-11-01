cmake_minimum_required(VERSION 3.20)
project(alienarcade LANGUAGES C)

# SDL3 reimplementation. src/ is the matching decompilation and is not
# compiled here; tools/build.py never looks in this directory.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Freetype REQUIRED)
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    # Core X11 is enough for a window. These extras fail the configure when
    # the matching -devel packages are not installed.
    foreach(opt XCURSOR XDBE XINPUT XFIXES XSYNC XRANDR XSCRNSAVER XSHAPE XTEST)
        set(SDL_X11_${opt} OFF CACHE BOOL "" FORCE)
    endforeach()
    FetchContent_MakeAvailable(SDL3)
endif()

add_executable(alienarcade
    src/main.c src/pic.c src/select.c src/play.c src/scene.c src/sound.c src/text.c src/between.c src/scores.c src/intro.c src/round1.c src/round2.c src/round3.c src/round4.c src/round5.c src/round6.c)
target_link_libraries(alienarcade PRIVATE SDL3::SDL3 Freetype::Freetype)
target_compile_options(alienarcade PRIVATE -Wall -Wextra -Wpedantic)
