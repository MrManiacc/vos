cmake_minimum_required(VERSION 3.25)

# Function to add a driver, compile it, and copy its output
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

    # Copy only the shared library file, ensuring the file extension is preserved
    add_custom_command(TARGET ${driver_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${driver_name}>
            "${output_dir}/$<TARGET_FILE_NAME:${driver_name}>"
            COMMENT "Copying only the shared library for ${driver_name} to ${output_dir}"
    )
endfunction(add_driver)

# Should do add_subdirectory for the driver, then create a target to copy the assets to the build directory
add_driver(mui)

add_driver(sys)