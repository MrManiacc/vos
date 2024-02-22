

# Glob all the source files
file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h")

add_executable(app ${SOURCES})
set_property(TARGET app PROPERTY C_STANDARD 17)
target_link_libraries(app vos)

# Creat a target that will copy the assets folder to the build directory
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/app/assets
        COMMENT "Copying assets folder to build directory"
)

add_dependencies(app copy_assets)

