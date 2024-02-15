#include <string.h>
#include "paths.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "core/vstring.h"
#include "platform/platform.h"

typedef struct PathContext {
    // The root directory.
    char *root_directory;
    // The current working directory.
    char *current_directory;
} PathContext;

//Static pointer to the path context.
static PathContext *path_context = null;

char *path_normalize(char *path) {
    if (path == null) {
        return null;
    }
    
    // Allocate enough space for the normalized path, including potential leading slash
    char *normalized_path = string_allocate_empty(
            string_length(path) + 2); // +1 for null terminator, +1 for potential leading slash
    if (normalized_path == null) {
        // Memory allocation failed
        return null;
    }
    
    size_t j = 0; // Index for writing to normalized_path
    
    // Ensure it starts with a slash
    if (path[0] != '/') {
        normalized_path[j++] = '/';
    }
    
    // Replace backslashes with slashes and skip colons
    for (size_t i = 0; i < strlen(path); ++i) {
        if (path[i] == '\\') {
            normalized_path[j++] = '/';
        } else if (path[i] != ':') {
            normalized_path[j++] = path[i];
        }
    }
    
    normalized_path[j] = '\0'; // Null-terminate the modified path
    
    return normalized_path;
}

/**
 *Removes the root directory from the path. If the path is not relative to the root directory, then the path is returned as is.
 * Expects the path to be absolute when passed in as well as being within the root file tree.
 * @param path The path to normalize.
 * @return The normalized path.
 * @example path_relative("/root/asset.txt") -> "asset.txt"
 * @example path_relative("/root/asset/asset.txt") -> "asset/asset.txt"
 */
char *path_relative(char *path) {
    if (path == null) {
        return null;
    }
    
    char *input_path = path_normalize(path);
    char *root_path = path_normalize(path_root_directory());
    // if the strings are equal, we know it's the root and just return a slash
    if (strcmp(input_path, root_path) == 0) return string_duplicate("/");
    if (string_starts_with(input_path, root_path)) {
        char *relative_path = string_duplicate(input_path + string_length(root_path) + 1);
        kfree(input_path, string_length(input_path) + 1, MEMORY_TAG_STRING);
        kfree(root_path, string_length(root_path) + 1, MEMORY_TAG_STRING);
//        vdebug("relative path: %s", relative_path)
//
        return relative_path;
    }
    return input_path;
}

/**
 * Moves the current directory to the given path. If no root path is given, the current working directory will be used.
 * Root directory can only be set once. It will be set by the first call to this function.
 *
 * The provided path can relative or absolute. If the path is relative, it will be relative to the current working directory.
 *
 * ‼️This must be called at least once before any other path functions are called.‼️
 */

// Example modification for path_move to demonstrate the pattern:
void initialize_paths(char *path) {
    char *normalized = path_normalize(string_duplicate(path)); // Use string_duplicate for tracking
    if (path_context == null) {
        path_context = kallocate(sizeof(PathContext), MEMORY_TAG_STRING); // Keep as is; non-string allocation
        path_context->root_directory = null;
        path_context->current_directory = null;
    }
    
    if (path_context->root_directory == null) {
        path_context->root_directory = normalized; // Set both root and current to normalized
        path_context->current_directory = normalized;
    } else {
        if (path_context->current_directory != path_context->root_directory) {
            string_deallocate(path_context->current_directory); // Use string_deallocate to free the current directory
        }
        path_context->current_directory = normalized;
    }
}

void shutdown_paths() {
    if (path_context == null) {
        return;
    }
    kfree(path_context, sizeof(PathContext), MEMORY_TAG_STRING); // Keep as is; non-string allocation
    path_context = null;
}


char *path_absolute(char *path) {
    if (path == NULL || path_context == NULL || path_context->current_directory == NULL) {
        verror("Path, path context, or current directory is null.");
        return NULL;
    }
    if (path[0] == '/') {
        return path_normalize(path); // Assuming path_normalize handles NULL correctly
    }
    size_t total_length =
            strlen(path_context->current_directory) + strlen(path) + 1; // +1 for the separator or terminator
    char *absolute_path = kallocate(total_length, MEMORY_TAG_STRING);
    vdebug("current directory: %s", path_context->current_directory)
    vdebug("path: %s", path)
    strcpy(absolute_path, path_context->current_directory);
    strcat(absolute_path, "/"); // Ensure there's a separator
    strcat(absolute_path, path);
    char *result = path_normalize(absolute_path);
    //free the absolute path
    return result;
}


/**
 * Gets the file name from a path.
 * @param path The path to get the file name from.
 * @return The file name from the path.
 */

char *path_file_name(char *path) {
    if (path == null) {
        return null;
    }
    char *absolute_path = path_absolute(path);
    char *file_name = string_split_at(absolute_path, "/", string_split_count(absolute_path, "/") - 1);
    file_name = string_split_at(file_name, ".", 0);
    kfree(absolute_path, string_length(absolute_path) + 1, MEMORY_TAG_STRING);
    return file_name;
}

/**
 * Gets the current working directory.
 * @return The current working directory.
 */
char *path_root_directory() {
    if (path_context == null) {
        verror("Path context is null");
        return null;
    }
    return path_context->root_directory;
}

/**
 * Gets the current directory.
 * @return The current directory.
 */
char *path_current_directory() {
    if (path_context == null) {
        verror("Path context is null");
        return null;
    }
    return path_context->current_directory;
}

/**
 * Gets the file extension from a path.
 * @param path The path to get the file extension from.
 * @return The file extension from the path.
 */
char *path_file_extension(char *path) {
    if (path == null) {
        return null;
    }
    char *absolute_path = path_absolute(path);
    char *file_extension = string_split_at(absolute_path, ".", string_split_count(absolute_path, ".") - 1);
    kfree(absolute_path, string_length(absolute_path) + 1, MEMORY_TAG_STRING);
    return file_extension;
}


/**
 * Gets the platform specific path from a path.
 * @param path The path to get the platform specific path from.
 * @return The platform specific path from the path. This is the reverse of path_normalize.
 */
char *path_to_platform(char *path) {
    
    if (path == NULL) {
        return NULL; // Added NULL check
    }
    u32 len = strlen(path);
    char *platform_path = string_allocate_sized(path, len + 2);
    
    strcpy(platform_path, path);  // Copy the original path
    
    // Transform '/' to '\\'
    for (u32 i = 0; i < len; ++i) {
        if (platform_path[i] == '/') {
            platform_path[i] = '\\';
        }
    }
    
    // Handle drive letter
    if (len > 1 && platform_path[0] == '\\' && platform_path[1] != '\\') {
        platform_path[0] = platform_path[1];
        platform_path[1] = ':';
    }
    
    return platform_path;
}

// Used for recursive calls to locate the directory that contains the init script.
// Assumes the passed path is a directory, so we want to check for the init script in the directory using the platform specific path.
const char *locate_boot_folder(char *search_path) {
    //Check if the file exists
    VFilePathList *paths = platform_collect_files_recursive(search_path);
    //Check if the file exists
    if (paths == null) {
        verror("Failed to collect files from directory %s", search_path);
        return null;
    }
    for (u32 i = 0; i < paths->count; i++) {
        char *path = paths->paths[i];
        if (string_ends_with(path, "boot.lua")) {
            // return the PARENT directory of the init script
            char *parent = platform_parent_directory(path);
//            kfree(paths, sizeof(VFilePathList), MEMORY_TAG_STRING);
            return parent;
        }
    }
    return null;
}


char *path_locate_root() {
    const char *root_path = locate_boot_folder(platform_get_current_working_directory());
    if (root_path == null) root_path = locate_boot_folder(platform_get_current_home_directory());
    if (root_path == null) {
        verror("Failed to locate root directory");
        return null;
    }
    return strdup(root_path);
}
