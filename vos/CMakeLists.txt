
# Glob all the source files
file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h")

# this is heuristically generated, and may not be correct
find_package(raylib CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(unofficial-tree-sitter CONFIG REQUIRED)
add_library(vos STATIC ${SOURCES})

# Set c standard target
set_property(TARGET vos PROPERTY C_STANDARD 17)
target_include_directories(vos PUBLIC src)
target_include_directories(vos PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(vos PRIVATE ${LUA_LIBRARIES} raylib unofficial::tree-sitter::tree-sitter)


