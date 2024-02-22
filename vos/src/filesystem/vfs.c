/**
 * Created by jraynor on 2/13/2024.
 */
#include <stdio.h>
#include "vfs.h"
#include "containers/dict.h"
#include "core/vlogger.h"
#include "lib/vmem.h"
#include "core/vstring.h"
#include "platform/platform.h"
#include "paths.h"
#include "containers/darray.h"

#define MAX_PATH 1024

// File System Structure
typedef struct FSContext {
    FsNode *root; // Root directory node
    Dict *users; // Users loaded in memory
    Dict *nodes;// The nodes that are currently loaded in memory. They can be directories or files.
} FSContext;

static FSContext *fs_context = null;

/**
 * Loads all the nodes in the file system into memory, recursively.
 */
void load_nodes();

/**
 * Shuts down all the nodes in the file system.
 */
void unload_nodes();

/**
 * @brief Loads a node from the file system into memory.
 *
 * This function loads a node from the file system into memory, regardless of whether it is a file or a directory.
 *
 * @param path The path of the node to load.
 * @return A pointer to the loaded node, or null if the node could not be loaded.
 */
FsNode *load_node(FsPath path);


/**
 * @brief Unloads a node from memory.
 *
 * This function unloads a node from the file system. If the node is a file, it will be
 * completely removed from memory and from the node hierarchy. If the node is a directory,
 * all its children nodes will be recursively unloaded. After unloading the node, it will
 * be removed from the file system dictionary.
 *
 * @param node The node to unload.
 * @return \c true if the node was successfully unloaded, \c false otherwise.
 *
 * @pre The file system must be initialized before calling this function.
 * @pre The node must be a valid reference to a node in memory.
 *
 * @warning This function does not free the memory occupied by the file or directory data,
 * it only removes the node from memory and the node hierarchy. The memory occupied by the
 * node and file/directory data must be freed separately using \c kfree().
 */
b8 unload_node(FsNode *node);

/**
 * Initializes the file system. This function must be called before any other file system functions.
 * @param root  The root path of the file system.
 * @return
 */
b8 vfs_initialize(FsPath root) {
    if (fs_context) {
        vwarn("vfs_initialize - File system already initialized.")
        return false;
    }
    //Setup the root path to be normalized to the platform
    initialize_paths(path_normalize(root));
    fs_context = kallocate(sizeof(FSContext), MEMORY_TAG_RESOURCE);
    fs_context->users = dict_new();
    fs_context->nodes = dict_new();
    load_nodes();
    //Collect the total nodes and tell the user how many nodes were loaded.
    u32 total_nodes = dict_size(fs_context->nodes);
    vinfo("vfs_initialize - Loaded %d nodes into memory.", total_nodes);
    return true;
}

void vfs_shutdown() {
    if (!fs_context) {
        vwarn("vfs_shutdown - File system not initialized.")
        return;
    }
    
    //free/shutdown all the nodes in the system
    unload_nodes();
    //root should have been freed by the shutdown_nodes function, at this point it should be null
    dict_delete(fs_context->users);
    dict_delete(fs_context->nodes);
    kfree(fs_context, sizeof(FSContext), MEMORY_TAG_RESOURCE);
    fs_context = null;
    shutdown_paths();
}


// Loads a file from the file system into memory.
FsNode *load_file(FsPath path) {
    if (!fs_context) {
        vwarn("load_file - File system not initialized.");
        return null;
    }
    FsNode *node = kallocate(sizeof(FsNode), MEMORY_TAG_RESOURCE);
    //Make sure to sanitize the path, the path will be relative to the root. i.e:
    // D:/root/asset.txt -> asset.txt
    // D:/root/asset/asset.txt -> asset/asset.txt
    node->path = path_relative(path);
    node->type = NODE_FILE;
    //Load the file into memory. Internally we use the real absolute path system file path,
    // but we store the relative path in the node and use that for all operations i.e. lookups, etc.
    if (platform_file_exists(path)) {
        node->data.file.size = platform_file_size(path);
        node->data.file.data = platform_read_file(path);
        dict_set(fs_context->nodes, node->path, node);
        vdebug("load_file - Loaded file at path: %s", node->path)
        return node;
    }
    vwarn("load_file - Failed to load file at path: %s", node->path);
    kfree(node, sizeof(FsNode), MEMORY_TAG_RESOURCE);
    return null;
}

// Loads a directory from the file system into memory.
FsNode *load_directory(FsPath path) {
    if (!fs_context) {
        vwarn("load_directory - File system not initialized.");
        return null;
    }
    FsNode *dir_node = kallocate(sizeof(FsNode), MEMORY_TAG_RESOURCE);
    dir_node->path = path_relative(path);
    dir_node->type = NODE_DIRECTORY;
    dir_node->data.directory.children = null; // Initialize to null
    dir_node->data.directory.child_count = 0; // Initialize count to 0
    
    dict_set(fs_context->nodes, dir_node->path, dir_node);
    vdebug("load_directory - Loaded directory at path: %s", dir_node->path);
    
    // Loads all the files and directories in the directory into memory.
    VFilePathList *child_files = platform_collect_files_direct(path);
    if (child_files == null) {
        vwarn("load_directory - No files found in directory at path: %s", dir_node->path);
        return null;
    }
    
    // Allocate memory for children pointers based on count
    dir_node->data.directory.children = kallocate(child_files->count * sizeof(FsNode *), MEMORY_TAG_RESOURCE);
    for (int i = 0; i < child_files->count; i++) {
        char *file_path = child_files->paths[i];
        FsNode *child_node = load_node(string_duplicate(file_path));
        if (child_node == null) {
            vwarn("load_directory - Failed to load child node at path: %s", file_path);
            continue;
        }
        child_node->parent = dir_node; // Set the parent of the node to the current directory.
        dir_node->data.directory.children[dir_node->data.directory.child_count++] = child_node; // Add the child to the directory.
    }
    file_path_list_free(child_files);
    return dir_node;
}

//Loads a node from the file system into memory, doesn't care if it's a file or directory.
FsNode *load_node(FsPath path) {
    if (!fs_context) {
        vwarn("load_node - File system not initialized.");
        return null;
    }
    char *system_path = platform_path(path);
    FsNode *node = null;
    if (platform_file_exists(system_path)) {
        if (platform_is_directory(system_path))
            node = load_directory(system_path);
        else node = load_file(system_path);
    }
    if (node == null) {
        vwarn("load_node - Failed to load node at path: %s", path)
        return null;
    }
    kfree(system_path, string_length(system_path) + 1, MEMORY_TAG_STRING);
    return node;
}

// Unloads a file from memory
b8 unload_file(FsNode *node) {
    if (!fs_context) {
        vwarn("unload_file - File system not initialized.");
        return false;
    }
    if (node == null) {
        vwarn("unload_file - fs_node not found.");
        return false;
    }
    FsPath path = node->path;
    if (node->type != NODE_FILE) {
        vwarn("unload_file - fs_node at path is not a file: %s", path)
        return false;
    }
    
    vdebug("unload_file - Unloaded file at path: %s", path);
    kfree(node->data.file.data, node->data.file.size, MEMORY_TAG_RESOURCE);
    kfree(node, sizeof(FsNode), MEMORY_TAG_RESOURCE);
    
    dict_remove(fs_context->nodes, path);
    return true;
}

// Unloads a directory from memory.
b8 unload_directory(FsNode *node) {
    if (!fs_context) {
        vwarn("unload_directory - File system not initialized.");
        return false;
    }
    if (node == null) {
        vwarn("unload_directory - fs_node not found.");
        return false;
    }
    
    FsPath path = node->path;
    if (node->type != NODE_DIRECTORY) {
        vwarn("unload_directory - fs_node at path is not a directory: %s", path);
        return false;
    }
    
    // Unload all children nodes
    for (u32 i = 0; i < node->data.directory.child_count; i++) {
        unload_node(node->data.directory.children[i]);
    }
    // Free the children array
    kfree(node->data.directory.children, node->data.directory.child_count * sizeof(FsNode *), MEMORY_TAG_RESOURCE);
    node->data.directory.children = null; // Ensure pointer is NULL after free
    node->data.directory.child_count = 0; // Reset count to 0
    
    vdebug("unload_directory - Unloaded folder at path: %s", path);
    dict_remove(fs_context->nodes, path);
    kfree(node, sizeof(FsNode), MEMORY_TAG_RESOURCE);
    return true;
}

b8 unload_node(FsNode *node) {
    if (node == null) {
        vwarn("unload_node - fs_node not found.");
        return false;
    }
    if (node->type == NODE_FILE) {
        return unload_file(node);
    } else if (node->type == NODE_DIRECTORY) {
        return unload_directory(node);
    }
    return false;
}

void load_nodes() {
    //Loads all the nodes in the file system into memory, recursively.
    FsNode *root = load_node(path_root_directory());
    //iterate children to make sure they are loaded.
    // Iterate the children of the root node to make sure they are loaded.
    for (u32 i = 0; i < root->data.directory.child_count; i++) {
        FsNode *child = root->data.directory.children[i];
        vdebug("load_nodes - Loaded child node at path: %s\n%s", child->path, vfs_node_to_string(child));
    }
    fs_context->root = root;
    
}

void unload_nodes() {
    //we just need to unload the root node, the root node will recursively unload all the children.
    u8 count = dict_size(fs_context->nodes);
    if (fs_context->root) {
        unload_node(fs_context->root);
        fs_context->root = null;
    }
    
    vdebug("unload_nodes - Unloaded %d nodes from memory.", count);
}


char *node_tree_to_string(FsNode *node, int depth) {
    if (node == NULL) {
        return string_duplicate(""); // Return an empty string for easy concatenation
    }
    
    char *result = NULL;
    // Calculate indentation for depth, adjusting for the root node's representation
    char *indent = depth > 1 ? string_repeat("   ", depth - 1) : string_duplicate("");
    
    // Determine the prefix based on the node's type and its depth
    // For the root node, use a special case to skip the initial slash in its display
    char *prefix = (depth == 1) ? "@--/" : (node->type == NODE_DIRECTORY ? "@--" : "$--");
    
    // Adjust the displayPath formatting for the root node to directly include the prefix without an additional slash
    char *displayPath;
    if (depth == 1) {
        displayPath = string_duplicate(prefix); // For the root, displayPath is just the prefix
    } else {
        // For non-root nodes, append the node path, adding a slash for directories
        displayPath = string_format("%s%s%s", indent, prefix, node->path);
        if (node->type == NODE_DIRECTORY) {
            char *temp = displayPath;
            displayPath = string_format("%s/", displayPath); // Append slash for directories
            kfree(temp, string_length(temp) + 1, MEMORY_TAG_STRING);
        }
    }
    
    // Initialize formatted line with the display path and a newline
    char *formattedLine = string_format("%s\n", displayPath);
    
    // Handle children for directories
    if (node->type == NODE_DIRECTORY) {
        char *children_str = string_duplicate(""); // Initialize to an empty string
        for (u32 i = 0; i < node->data.directory.child_count; ++i) {
            FsNode *child = node->data.directory.children[i];
            if (child == NULL) continue;
            char *child_str = node_tree_to_string(child, depth + 1);
            char *temp = children_str;
            children_str = string_format("%s%s", children_str, child_str);
            kfree(child_str, string_length(child_str) + 1, MEMORY_TAG_STRING);
            kfree(temp, string_length(temp) + 1, MEMORY_TAG_STRING);
        }
        char *temp = formattedLine;
        formattedLine = string_format("%s%s", formattedLine, children_str); // Append children string
        kfree(temp, string_length(temp) + 1, MEMORY_TAG_STRING);
        kfree(children_str, string_length(children_str) + 1, MEMORY_TAG_STRING);
    }
    
    // Cleanup
    kfree(indent, string_length(indent) + 1, MEMORY_TAG_STRING);
    kfree(displayPath, string_length(displayPath) + 1, MEMORY_TAG_STRING);
    
    return formattedLine; // Return the formatted directory tree or an empty string if NULL
}

char *vfs_to_string() {
    if (fs_context == null) {
        vwarn("vfs_to_string - File system not initialized.");
        return null;
    }
    return vfs_node_to_string(fs_context->root);
}

char *vfs_node_to_string(FsNode *node) {
    if (node == null) {
        vwarn("vfs_node_to_string - fs_node not found.");
        return null;
    }
    return node_tree_to_string(node, 1);
}

b8 vfs_node_exists(FsPath path) {
    if (fs_context == null) {
        vwarn("vfs_node_exists - File system not initialized.");
        return false;
    }
    return dict_contains(fs_context->nodes, path);
}

FsNode *vfs_node_get(FsPath path) {
    if (fs_context == null) {
        vwarn("vfs_node_get - File system not initialized.");
        return null;
    }
    return dict_get(fs_context->nodes, path);
}

