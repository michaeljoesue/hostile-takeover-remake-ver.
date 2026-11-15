cmake_minimum_required(VERSION 3.21)

project(HostileTakeover LANGUAGES C CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "Hostile Takeover builds for Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Put the game, its data files and the tools in one folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${config} CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_BINARY_DIR}/bin/${config})
endforeach()

# SDL2. Uses an installed SDL2 (e.g. from vcpkg) when one is found, otherwise
# downloads and builds SDL2 from source as a static library.
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz
        URL_HASH SHA256=24b574f71c87a763f50704bbb630cbe38298d544a1f890f099a4696b1d6beba4
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(SDL2)
endif()

add_subdirectory(game)
add_subdirectory(inicrunch)
