cmake_minimum_required(VERSION 3.25)
project(vlos C)
# Add AddressSanitizer and UndefinedBehaviorSanitizer flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")

# this is heuristically generated, and may not be correct
find_package(raylib CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(unofficial-tree-sitter CONFIG REQUIRED)
#check if we are in debug build or release
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    # Set the cmake runtime output directory to our build folder
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/debug)
#else ()
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/release)
#endif ()


add_subdirectory(vos)
add_subdirectory(app)