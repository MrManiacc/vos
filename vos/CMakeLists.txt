set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
# Glob all the source files
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

# Add AddressSanitizer and UndefinedBehaviorSanitizer flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")

add_library(vos STATIC ${SOURCES})
# Set c standard target
set_property(TARGET vos PROPERTY C_STANDARD 17)
target_include_directories(vos PUBLIC src)
target_link_libraries(vos ${LUA_LIBRARIES} raylib unofficial::tree-sitter::tree-sitter Threads::Threads)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(vos "-framework IOKit")
    target_link_libraries(vos "-framework Cocoa")
    target_link_libraries(vos "-framework OpenGL")
endif()
