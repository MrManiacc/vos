include(CheckCCompilerFlag)
# Glob all the source files
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_library(vos STATIC ${SOURCES})
# Set c standard target
set_property(TARGET vos PROPERTY C_STANDARD 17)
target_include_directories(vos PUBLIC src)
target_link_libraries(vos PUBLIC ${LUA_LIBRARIES} nanovg::nanovg GLEW::GLEW glfw)

# Setup our defines for the target
target_compile_definitions(vos PUBLIC
        -DUSE_LINE_NUMBER
        -DUSE_DEBUG_LOG
        -DKEXPORT
)
check_c_compiler_flag(-Wint-to-pointer-cast HAS_INT_TO_POINTER_CAST )
if (HAS_INT_TO_POINTER_CAST)
    target_compile_options(vos PRIVATE -Wint-to-pointer-cast)
endif ()
