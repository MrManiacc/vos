cmake_minimum_required(VERSION 3.25)
project(vlos C)
# this is heuristically generated, and may not be correct
find_package(Lua REQUIRED)
#find_package(unofficial-tree-sitter CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(nanovg CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
# Only run these if on mac
#if (APPLE)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
#endif (APPLE)

add_subdirectory(vos)
add_subdirectory(app)
add_subdirectory(drivers)
#add_subdirectory(muil)