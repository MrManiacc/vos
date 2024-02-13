#include <string.h>
#include "paths.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "core/vstring.h"

typedef struct PathContext {
    // The root directory.
    char *root_directory;
    // The current working directory.
    char *current_directory;
} PathContext;

//Static pointer to the path context.
static PathContext *path_context = null;

char *path_normalize(char *path) {
    if (path == NULL) {
        return NULL;
    }
    u32 len = strlen(path);
    char *normalized_path = kallocate(len + 1, MEMORY_TAG_VFS);
    u32 j = 0;
    for (u32 i = 0; i < len; ++i) {
        if (path[i] == '\\') {
            normalized_path[j] = '/';
        } else if (path[i] == ':') {
            continue;  // Skip the colon
        } else {
            normalized_path[j] = path[i];  // Convert to lowercase
        }
        j++;
    }
    normalized_path[j] = '\0';  // Null-terminate the string
    
    // Make sure it starts with a slash
    if (normalized_path[0] != '/') {
        char *new_path = kallocate(j + 2, MEMORY_TAG_VFS);
        strcpy(new_path + 1, normalized_path);
        new_path[0] = '/';
        kfree(normalized_path, j + 1, MEMORY_TAG_VFS);
        normalized_path = new_path;
    }
    //free the passed path
//    kfree(path, len + 1, MEMORY_TAG_VFS);
    // Removed kfree(path, len + 1, MEMORY_TAG_VFS);
    return normalized_path;
}

/**
 *Removes the root directory from the path. If the path is not relative to the root directory, then the path is returned as is.
 * Expects the path to be absolute when passed in as well as being within the root file tree.
 * @param path The path to normalize.
 * @return The normalized path.
 */
char *path_relative(char *path) {
    if (path == null) {
        return null;
    }
    
    char *input_path = path_normalize(path);
    char *root_path = path_normalize(path_root_directory());
    if (string_starts_with(input_path, root_path)) {
        char *relative_path = string_duplicate(input_path + string_length(root_path) + 1);
        kfree(input_path, string_length(input_path) + 1, MEMORY_TAG_VFS);
        kfree(root_path, string_length(root_path) + 1, MEMORY_TAG_VFS);
        vdebug("relative path: %s", relative_path)
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
void path_move(char *path) {
    char *normalized = path_normalize(string_duplicate(path));
    vdebug("normalized path: %s", normalized)
    if (path_context == null) {
        //Initialize the path context.
        path_context = kallocate(sizeof(PathContext), MEMORY_TAG_VFS);
        path_context->root_directory = null;
        path_context->current_directory = null;
    }
    
    if (path_context->root_directory == null) {
        path_context->root_directory = normalized; // Set both root and current to normalized
        path_context->current_directory = normalized;
    } else {
        if (path_context->current_directory != path_context->root_directory) {
            kfree(path_context->current_directory, strlen(path_context->current_directory) + 1, MEMORY_TAG_VFS);
        }
        path_context->current_directory = normalized;
    }
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
    char *absolute_path = kallocate(total_length, MEMORY_TAG_VFS);
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
 * Gets the parent directory of a path.
 * @param path The path to get the parent directory from.
 * @return The parent directory of the path.
 */
char *path_parent_directory(char *path) {
    if (path == null) {
        return null;
    }
    char *absolute_path = path_absolute(path);
    char *parent_directory = string_split_at(absolute_path, "/", string_split_count(absolute_path, "/") - 1);
    kfree(absolute_path, string_length(absolute_path) + 1, MEMORY_TAG_VFS);
    return parent_directory;
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
    kfree(absolute_path, string_length(absolute_path) + 1, MEMORY_TAG_VFS);
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
    kfree(absolute_path, string_length(absolute_path) + 1, MEMORY_TAG_VFS);
    return file_extension;
}


/**
 * Gets the platform specific path from a path.
 * @param path The path to get the platform specific path from.
 * @return The platform specific path from the path. This is the reverse of path_normalize.
 */
char *path_to_platform(char *path) {
#ifndef KPLATFORM_WINDOWS
    return string_duplicate(path);  // Return a copy
#else
    if (path == NULL) {
        return NULL; // Added NULL check
    }
    u32 len = strlen(path);
    char *platform_path = kallocate(len + 2, MEMORY_TAG_VFS);  // Allocate new memory
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
#endif
}