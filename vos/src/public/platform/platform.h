/**
 * @file platform.h
 * @author Travis Vroman (travis@kohiengine.com)
 * @brief This file contains the platform layer, or at least the interface to it.
 * Each platform should provide its own implementation of this in a .c file, and
 * should be compiled exclusively of the rest.
 * @version 1.0
 * @date 2022-01-10
 *
 * @copyright Kohi Game Engine is Copyright (c) Travis Vroman 2021-2022
 *
 */

#pragma once

#include "defines.h"
#include "kernel/kernel.h"

typedef struct platform_system_config {
    /** @brief application_name The name of the application. */
    const char *application_name;
    /** @brief x The initial x position of the main window. */
    i32 x;
    /** @brief y The initial y position of the main window.*/
    i32 y;
    /** @brief width The initial width of the main window. */
    i32 width;
    /** @brief height The initial height of the main window. */
    i32 height;
} platform_system_config;

typedef struct DynLibFunction {
    const char *name;
    void *pfn;
} DynLibFunction;

typedef struct DynLib {
    const char *name;
    const char *filename;
    u64 internal_data_size;
    void *internal_data;
    u32 watch_id;
    struct Dict *functions;
} DynLib;

typedef enum platform_error_code {
    PLATFORM_ERROR_SUCCESS = 0,
    PLATFORM_ERROR_UNKNOWN = 1,
    PLATFORM_ERROR_FILE_NOT_FOUND = 2,
    PLATFORM_ERROR_FILE_LOCKED = 3,
    PLATFORM_ERROR_FILE_EXISTS = 4
} platform_error_code;

typedef struct VFilePathList {
    char **paths; // Dynamic array of strings
    int count; // Number of paths
} FilePathList;

/**
 * @brief Initializes the platform layer.
 */
b8 platform_initialize();

/**
 * @brief Shuts down the platform layer.
 *
 * @param plat_state A pointer to the platform layer state.
 */
void platform_shutdown();


/**
 * @brief Get the temporary directory path for the current platform.
 *
 * This function returns the path to the temporary directory for the current platform.
 *
 * @return A pointer to a null-terminated string that represents the temporary directory path.
 */
char *platform_get_temp_directory();
/**
 * @brief Performs any platform-specific message pumping that is required
 * for windowing, etc.
 *
 * @return True on success; otherwise false.
 */
b8 platform_pump_messages(struct Kernel *kernel);

/**
 * @brief Performs platform-specific memory allocation of the given size.
 *
 * @param size The size of the allocation in bytes.
 * @param aligned Indicates if the allocation should be aligned.
 * @return A pointer to a block of allocated memory.
 */
void *platform_allocate(u64 size, b8 aligned);

/**
 * @brief Performs platform-specific memory reallocation of the given block of memory.
 *
 * @param block The block of memory to be reallocated.
 * @param size The new size of the allocation in bytes.
 * @param aligned Indicates if the allocation should be aligned.
 * @return A pointer to the reallocated block of memory.
 */
void *platform_reallocate(void *block, u64 size, b8 aligned);

/**
 * @brief Frees the given block of memory.
 *
 * @param block The block to be freed.
 * @param aligned Indicates if the block of memory is aligned.
 */
void platform_free(void *block, b8 aligned);

/**
 * @brief Performs platform-specific zeroing out of the given block of memory.
 *
 * @param block The block to be zeroed out.
 * @param size The size of data to zero out.
 * @return A pointer to the zeroed out block of memory.
 */
void *platform_zero_memory(void *block, u64 size);

/**
 * @brief Copies the bytes of memory in source to dest, of the given size.
 *
 * @param dest The destination memory block.
 * @param source The source memory block.
 * @param size The size of data to be copied.
 * @return A pointer to the destination block of memory.
 */
void *platform_copy_memory(void *dest, const void *source, u64 size);

/**
 * @brief Sets the bytes of memory to the given value.
 *
 * @param dest The destination block of memory.
 * @param value The value to be set.
 * @param size The size of data to set.
 * @return A pointer to the set block of memory.
 */
void *platform_set_memory(void *dest, i32 value, u64 size);

/**
 * @brief Performs platform-specific printing to the console of the given
 * message and colour code (if supported).
 *
 * @param message The message to be printed.
 * @param colour The colour to print the text in (if supported).
 */
void platform_console_write(const char *message, u8 colour);

/**
 * @brief Performs platform-specific printing to the error console of the given
 * message and colour code (if supported).
 *
 * @param message The message to be printed.
 * @param colour The colour to print the text in (if supported).
 */
void platform_console_write_error(const char *message, u8 colour);

/**
 * @brief Gets the absolute time since the application started.
 *
 * @return The absolute time since the application started.
 */
f64 platform_get_absolute_time(void);

/**
 * @brief Sleep on the thread for the provided milliseconds. This blocks the main thread.
 * Should only be used for giving time back to the OS for unused update power.
 * Therefore it is not exported. Times are approximate.
 *
 * @param ms The number of milliseconds to sleep for.
 */
VAPI void platform_sleep(u64 ms);

/**
 * @brief Obtains the number of logical processor cores.
 *
 * @return The number of logical processor cores.
 */
i32 platform_get_processor_count(void);

/**
 * @brief Obtains the required memory amount for platform-specific handle data,
 * and optionally obtains a copy of that data. Call twice, once with memory=0
 * to obtain size, then a second time where memory = allocated block.
 *
 * @param out_size A pointer to hold the memory requirement.
 * @param memory Allocated block of memory.
 */
VAPI void platform_get_handle_info(u64 *out_size, void *memory);

/**
 * @brief Returns the device pixel ratio of the main window.
 */
VAPI f32 platform_device_pixel_ratio(void);

/**
 * @brief Loads a dynamic library.
 *
 * @param name The name of the library file, *excluding* the extension. Required.
 * @param out_library A pointer to hold the loaded library. Required.
 * @return True on success; otherwise false.
 */
VAPI b8 platform_dynamic_library_load(const char *name, DynLib *out_library);

/**
 * @brief Unloads the given dynamic library.
 *
 * @param library A pointer to the loaded library. Required.
 * @return True on success; otherwise false.
 */
VAPI b8 platform_dynamic_library_unload(DynLib *library);

/**
 * @brief Loads an exported function of the given name from the provided loaded library.
 *
 * @param name The function name to be loaded.
 * @param library A pointer to the library to load the function from.
 * @return True on success; otherwise false.
 */
VAPI DynLibFunction *platform_dynamic_library_load_function(const char *name, const DynLib *library);

VAPI const char *platform_file_name(const char *path);
VAPI const char *platform_resolve_symlink(const char *path);
VAPI const char *platform_file_append(const char *base, const char *relative);
VAPI b8 platform_write_file(const char *path, const void *data, u64 size);
VAPI b8 platform_create_directory(const char *path);
VAPI b8 platform_delete_file(const char *path);
VAPI b8 platform_create_symlink(const char *path, const char *target);

/**
 * @brief Returns the file extension for the current platform.
 */
VAPI const char *platform_dynamic_library_extension(void);

/**
 * @brief Returns a file prefix for libraries for the current platform.
 */
VAPI const char *platform_dynamic_library_prefix(void);

/**
 * @brief Copies file at source to destination, optionally overwriting.
 *
 * @param source The source file path.
 * @param dest The destination file path.
 * @param overwrite_if_exists Indicates if the file should be overwritten if it exists.
 * @return An error code indicating success or failure.
 */
VAPI platform_error_code platform_copy_file(const char *source, const char *dest, b8 overwrite_if_exists);

/**
 * @brief Watch a file at the given path.
 *
 * @param file_path The file path. Required.
 * @return The watch identifier, starting from 1, or 0 if the watch failed.
 */
VAPI u32 *platform_watch_file(const char *file_path);

/**
 * @brief Stops watching the file with the given watch identifier.
 *
 * @param watch_id The watch identifier
 * @return True on success; otherwise false.
 */
VAPI b8 platform_unwatch_file(u32 watch_id);


/**
* @brief Deletes the file at the given path.s
* @param path
* @return
*/
VAPI b8 platform_file_exists(const char *path);

VAPI b8 platfrom_is_symbolic_link(const char *path);

/**
* @brief Checks if a given path is a directory.
*
* This function checks whether the specified path represents a directory or not.
*
* @param path The path to be checked.
* @return Returns a bool value indicating if the path is a directory or not.
*         - true: If the path is a directory.
*         - false: If the path is not a directory.
*
* @note The function returns false if the path does not exist.
*/
VAPI b8 platform_is_directory(const char *path);

/**
 * @brief Collect files directly within a given directory, non-recursively.
 *
 * This function collects files directly within a specified directory without searching subdirectories.
 * It creates and initializes a `VFilePathList` struct to store the paths of the collected files.
 *
 * @param path The path of the directory to collect files from.
 * @return A pointer to a `VFilePathList` struct that contains the collected file paths, or `NULL` if the allocation failed.
 *
 * @note The caller is responsible for freeing the returned `VFilePathList` using `v_file_path_list_free`.
 */
VAPI FilePathList *platform_collect_files_direct(const char *path);

/**
 * @brief Recursively collects files from a given path and returns a list of file paths.
 *
 * This function collects all files (and folders) within the specified path,
 * including files within subdirectories. The resulting list contains the full
 * paths of all collected files.
 *
 * @param path The base path from which to start collecting files.
 *
 * @return A pointer to a dynamically allocated VFilePathList structure
 *         containing the collected file paths. If an error occurs during
 *         memory allocation or path collection, NULL is returned.
 *
 * @note The caller is responsible for freeing the allocated VFilePathList
 *       structure using the file_path_list_free function.
 */
VAPI FilePathList *platform_collect_files_recursive(const char *path);


/**
* @brief Frees the FilePathList and its contents
*
* This function frees the memory allocated for a FilePathList structure and its contents.
* It releases the memory for each string in the paths array and then frees the array itself.
* Finally, it frees the FilePathList structure itself.
*
* @param list The FilePathList to be freed
*/
VAPI void platform_file_path_list_free(FilePathList *list);

/**
* @brief Checks if the given path represents a file.
*
* This function checks if the given path represents a file in the platform-specific
* file system. It returns true if the path represents a file, and false otherwise.
*
* @param[in] path The path to check if it represents a file.
* @return True if the path represents a file, false otherwise.
*/
VAPI b8 platform_is_file(const char *path);

/**
* @brief Get the size of a file.
*
* This function obtains the size in bytes of the file located at the given path.
*
* @param path The path to the file.
* @return The size of the file in bytes, or 0 if the file does not exist or an error occurred.
*/
VAPI u32 platform_file_size(const char *path);

/**
* @brief Reads the contents of a file and returns a pointer to the data.
*
* This function reads the contents of the file specified by the given path, and returns a pointer to the data.
* The caller is responsible for freeing the memory allocated for the data using a corresponding "free" function.
*
* @param path The path of the file to be read.
* @return A pointer to the data read from the file, or NULL if an error occurs.
*/
VAPI void *platform_read_file(const char *path);

/**
 * Converts the given path to a platform-specific path.
 * @param path The path to convert.
 * @return A platform-specific path.
 * @example platform_path("/C/root/asset.txt") -> "C:/root/asset.txt" (Windows)
 * @example platform_path("/C/root/asset/asset.txt") -> "C:/root/asset/asset.txt" (Windows)
 */
VAPI char *platform_path(const char *path);


/**
 * @brief Retrieves the current working directory of the platform.
 *
 * This function returns a null-terminated string containing the current working directory of the platform.
 *
 * @return A pointer to the current working directory string.
 *
 * @note The returned string is dynamically allocated and should be freed when no longer needed.
 * @note If the function fails to retrieve the current working directory, it returns a null pointer.
 */
char *platform_get_current_working_directory(void);


/**
 * @brief Gets the home directory of the current user.
 *
 * This function retrieves the home directory of the current user on Windows.
 *
 * @return A pointer to the home directory buffer. NULL if an error occurred.
 */
char *platform_get_current_home_directory(void);


/**
 * @brief Returns the parent directory of the given path.
 *
 * This function takes a path string as input and returns a new string representing the parent directory of the path.
 * If the given path is already the root directory or an empty string, NULL is returned.
 * The returned string must be freed by the caller.
 *
 * @param path The path string for which the parent directory is to be found.
 * @return A new string representing the parent directory of the given path, or NULL if it is the root directory or an empty string.
 *
 * @note The input path must be a valid null-terminated string.
 * @note The returned string must be freed by the caller using free() to avoid memory leaks.
 * @note The returned string may be the same as the input path if it does not contain a directory separator (i.e., it's a file name in the root directory).
 * @note This function does not modify the input path.
 */
char *platform_parent_directory(const char *path);


/**
 * @brief Check if a debugger is attached to the current process.
 *
 * This function checks if a debugger is attached to the current process.
 * It returns a boolean value indicating whether a debugger is attached or not.
 *
 * @return A boolean value indicating whether a debugger is attached (true) or not (false).
 */
b8 platform_is_debugger_attached(void);
