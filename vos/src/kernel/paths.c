#include <string.h>
#include "paths.h"
#include "core/logger.h"
#include "core/mem.h"
#include "core/str.h"

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

    // Free the passed path
    kfree(path, len + 1, MEMORY_TAG_VFS);

    char *lower = string_to_lower(normalized_path);
    kfree(normalized_path, j + 1, MEMORY_TAG_VFS);
    return lower;
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
        path_context->root_directory = normalized;
        path_context->current_directory = path_context->root_directory;
    } else {
        path_context->current_directory = normalized;
    }
}

/**
 * Gets the absolute path from a relative path.
 * @param path The relative path.
 * @return The absolute path.
 */
char *path_absolute(char *path) {
    if (path == null) {
        return null;
    }
    if (path[0] == '/') {
        return path_normalize(path);
    }
    char *absolute_path = kallocate(string_length(path_context->current_directory) + string_length(path) + 1,
                                    MEMORY_TAG_VFS);
    strcpy(absolute_path, path_context->current_directory);
    strcat(absolute_path, path);
    return path_normalize(absolute_path);
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