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

# SDL3. Uses an installed SDL3 (e.g. from vcpkg) when one is found, otherwise
# downloads and builds SDL3 from source as a static library.
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.20/SDL3-3.4.20.tar.gz
        URL_HASH SHA256=f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(SDL3)
endif()

add_subdirectory(game)
add_subdirectory(inicrunch)

# The C# tools (map editor M, AniMax and the data build tools) are built by
# HostileTakeoverTools.sln into bin. Turn this on to build them along with
# the game; it needs a Visual Studio generator (for MSBuild).
option(HT_BUILD_TOOLS "Build the C# tools (HostileTakeoverTools.sln) into bin" OFF)
if(HT_BUILD_TOOLS)
    if(NOT CMAKE_VS_MSBUILD_COMMAND)
        message(FATAL_ERROR "HT_BUILD_TOOLS needs a Visual Studio generator")
    endif()
    add_custom_target(tools ALL
        COMMAND "${CMAKE_VS_MSBUILD_COMMAND}" HostileTakeoverTools.sln
            -restore -m -nologo -v:minimal -p:Configuration=$<CONFIG>
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Building the C# tools into bin"
        VERBATIM)
endif()
