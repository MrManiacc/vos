cmake_minimum_required(VERSION 3.25)
project(vlos C)
# this is heuristically generated, and may not be correct
find_package(raylib CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(unofficial-tree-sitter CONFIG REQUIRED)
#-static-libgcc -static-libstdc++ to g++
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

add_subdirectory(vos)
add_subdirectory(app)