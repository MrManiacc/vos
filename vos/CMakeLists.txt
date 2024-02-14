set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
# Glob all the source files
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")


add_library(vos STATIC ${SOURCES}
        src/filesystem/paths.h)
# Set c standard target
set_property(TARGET vos PROPERTY C_STANDARD 17)
target_include_directories(vos PUBLIC src)
target_link_libraries(vos ${LUA_LIBRARIES} raylib unofficial::tree-sitter::tree-sitter Threads::Threads)

# Setup our defines for the target
target_compile_definitions(vos PUBLIC
        -DUSE_LINE_NUMBER
        -DUSE_DEBUG_LOG
        -DKEXPORT
)
