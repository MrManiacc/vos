cmake_minimum_required(VERSION 3.25)

project(vlos C)
find_package(Lua REQUIRED)
find_package(GLEW REQUIRED)
find_package(nanovg CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(LIBFFI REQUIRED IMPORTED_TARGET libffi)
include(.cmake/PLib.cmake)


create_library(vos STATIC vos
        LINK_LIBRARIES
            ${LUA_LIBRARIES}
            nanovg::nanovg
            GLEW::GLEW glfw
            PkgConfig::LIBFFI
        COMPILE_DEFINITIONS
            USE_LINE_NUMBER
            USE_DEBUG_LOG
            KEXPORT
)


add_subdirectory(app)
add_subdirectory(drivers)
#add_subdirectory(muil)