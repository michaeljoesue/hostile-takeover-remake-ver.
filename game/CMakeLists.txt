# Hostile Takeover for Windows

set(HTROOT ${PROJECT_SOURCE_DIR})

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS
    ${HTROOT}/game/*.cpp
    ${HTROOT}/game/sdl/*.cpp
    ${HTROOT}/base/*.cpp
    ${HTROOT}/mpshared/*.cpp)

# miscgraphics.cpp is #included by thunks.cpp
list(REMOVE_ITEM GAME_SOURCES ${HTROOT}/game/miscgraphics.cpp)

file(GLOB GAME_HEADERS CONFIGURE_DEPENDS
    ${HTROOT}/game/*.h
    ${HTROOT}/game/sdl/*.h
    ${HTROOT}/base/*.h
    ${HTROOT}/mpshared/*.h
    ${HTROOT}/inc/*.h)

add_executable(HostileTakeover WIN32
    ${GAME_SOURCES}
    ${GAME_HEADERS}
    ${HTROOT}/inc/rip.cpp
    ${HTROOT}/game/sdl/win/HostileTakeover.rc)

target_include_directories(HostileTakeover PRIVATE
    ${HTROOT}
    ${HTROOT}/game
    ${HTROOT}/game/sdl
    ${HTROOT}/inc)

target_compile_definitions(HostileTakeover PRIVATE
    SDL
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    $<$<CONFIG:Debug>:DEBUG DEV_BUILD DEBUG_LOGGING>)

if(MSVC)
    target_compile_options(HostileTakeover PRIVATE
        /W3 /MP /utf-8
        # Conversions that are part of the original code's style
        /wd4244 /wd4267 /wd4305 /wd4800 /wd4996)
    set_property(TARGET HostileTakeover PROPERTY
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:HostileTakeover>)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY
        VS_STARTUP_PROJECT HostileTakeover)
else()
    # MinGW / clang. The code relies on behaviors these flags preserve.
    target_compile_options(HostileTakeover PRIVATE
        -fno-delete-null-pointer-checks -fno-strict-aliasing -fwrapv
        -Wno-write-strings -Wno-multichar)
    target_link_options(HostileTakeover PRIVATE -static)
endif()

if(TARGET SDL2::SDL2main)
    target_link_libraries(HostileTakeover PRIVATE SDL2::SDL2main)
endif()
if(TARGET SDL2::SDL2-static)
    target_link_libraries(HostileTakeover PRIVATE SDL2::SDL2-static)
else()
    target_link_libraries(HostileTakeover PRIVATE SDL2::SDL2)
endif()

# Copy the game data (and SDL2.dll, when using a shared SDL2) next to the
# executable so it can be run from the build folder
add_custom_command(TARGET HostileTakeover POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${HTROOT}/game/htdata832.pdb
        ${HTROOT}/game/htsfx.pdb
        $<TARGET_RUNTIME_DLLS:HostileTakeover>
        $<TARGET_FILE_DIR:HostileTakeover>
    COMMAND_EXPAND_LISTS)

# cmake --install produces a folder with everything needed to play
install(TARGETS HostileTakeover RUNTIME DESTINATION .)
install(FILES ${HTROOT}/game/htdata832.pdb ${HTROOT}/game/htsfx.pdb
    DESTINATION .)
install(FILES $<TARGET_RUNTIME_DLLS:HostileTakeover> DESTINATION .)
