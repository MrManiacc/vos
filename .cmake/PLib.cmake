

# Include the helper module for parsing arguments
include(CMakeParseArguments)


function(add_driver driver_name)
    # Specify the source directory and files
    set(driver_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${driver_name}")
    file(GLOB driver_sources "${driver_source_dir}/*.c" "${driver_source_dir}/*.cpp")

    # Create the shared library target
    add_library(${driver_name} SHARED ${driver_sources})

    # Link against the vos library
    target_link_libraries(${driver_name} PRIVATE vos)

    # Include directories, adjust as needed
    target_include_directories(${driver_name} PRIVATE ${driver_source_dir})

    # Set the property to use C17 standard (or any other standard as needed)
    set_property(TARGET ${driver_name} PROPERTY C_STANDARD 17)

    # Prepare the output directory paths
    set(output_dir "${CMAKE_BINARY_DIR}/app/assets/drivers/")

    # Ensure the output directory exists
    file(MAKE_DIRECTORY ${output_dir})

    # Move the shared library file, ensuring the file extension is preserved
    add_custom_command(TARGET ${driver_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename
            $<TARGET_FILE:${driver_name}>
            "${output_dir}/$<TARGET_FILE_NAME:${driver_name}>"
            COMMENT "Moving only the shared library for ${driver_name} to ${output_dir}"
    )

    # Move the .pdb file if it exists (for debug builds)
    # Ensure to check if PDB file exists before attempting to move it
    add_custom_command(TARGET ${driver_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename
            $<TARGET_PDB_FILE:${driver_name}>
            "${output_dir}/$<TARGET_PDB_FILE_NAME:${driver_name}>"
            COMMENT "Moving .pdb for ${driver_name} to ${output_dir}"
            CONDITION $<BOOL:$<TARGET_PROPERTY:${driver_name},PDB_FILE_NAME>>
    )

    # Move the .lib file
    add_custom_command(TARGET ${driver_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename
            $<TARGET_LINKER_FILE:${driver_name}>
            "${output_dir}/$<TARGET_LINKER_FILE_NAME:${driver_name}>"
            COMMENT "Moving .lib for ${driver_name} to ${output_dir}"
    )
endfunction(add_driver)


# Function to create a library with specified properties, link with given targets, and add compile definitions
function(create_library project_name library_type source_dir)
    # Parse additional arguments as lists of libraries to link against and compile definitions
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs LINK_LIBRARIES COMPILE_DEFINITIONS)
    cmake_parse_arguments(PARSE_ARGV 3 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # Assuming the source directory is relative to the current source directory
    set(project_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${source_dir}")

    # Glob source files
    file(GLOB_RECURSE SOURCES "${project_source_dir}/src/*.c")

    # Glob public headers and internal headers
    file(GLOB_RECURSE PUBLIC_HEADERS "${project_source_dir}/src/public/*.h")
    file(GLOB_RECURSE PRIVATE_HEADERS "${project_source_dir}/src/internal/*.h")

    # Create library (static or shared based on input)
    if (library_type STREQUAL "STATIC")
        add_library(${project_name} STATIC ${SOURCES} ${PUBLIC_HEADERS})
    elseif (library_type STREQUAL "SHARED")
        add_library(${project_name} SHARED ${SOURCES} ${PUBLIC_HEADERS})
    else ()
        message(FATAL_ERROR "Unsupported library type: ${library_type}. Use STATIC or SHARED.")
    endif ()

    # Set the property to use C17 standard
    set_property(TARGET ${project_name} PROPERTY C_STANDARD 17)

    # Include public header files for the library
    target_include_directories(${project_name} PUBLIC "${project_source_dir}/src/public")

    # Include internal header files for compilation but not for users of the library
    target_include_directories(${project_name} PRIVATE "${project_source_dir}/src/internal")

    # Link with specified libraries, if any
    if (ARG_LINK_LIBRARIES)
        target_link_libraries(${project_name} PUBLIC ${ARG_LINK_LIBRARIES})
    endif ()

    # Add compile definitions, if any
    if (ARG_COMPILE_DEFINITIONS)
        foreach (definition IN LISTS ARG_COMPILE_DEFINITIONS)
            target_compile_definitions(${project_name} PUBLIC ${definition})
        endforeach ()
    endif ()
endfunction(create_library)