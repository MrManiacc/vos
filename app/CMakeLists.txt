

# Glob all the source files
file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h")

add_executable(app ${SOURCES})
set_property(TARGET app PROPERTY C_STANDARD 17)
target_link_libraries(app vos)
