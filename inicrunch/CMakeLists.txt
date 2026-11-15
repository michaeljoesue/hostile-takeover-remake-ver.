# inicrunch: compiles the game's .ini text files into the binary form stored
# in htdata832.pdb. Used by the data build (data/makefile).

add_executable(inicrunch main.cpp ini.cpp ini.h)
target_compile_definitions(inicrunch PRIVATE _CRT_SECURE_NO_WARNINGS)
if(NOT MSVC)
    target_link_options(inicrunch PRIVATE -static)
endif()
