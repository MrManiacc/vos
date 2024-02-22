project(muil C)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(muil ${SOURCES})

set_target_properties(muil PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        LINKER_LANGUAGE C
)

target_include_directories(muil PUBLIC src)
