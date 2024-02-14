/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

//#include <pthread.h>
#include "defines.h"
#include "kernel/kresult.h"
#include "containers/dict.h"

typedef char *Path;
#ifndef NODE_CAPACITY
#define NODE_CAPACITY 1024
#endif
#ifndef USER_CAPACITY
#define USER_CAPACITY 1024
#endif

/**
 * A serializable representation of a user
 */
typedef struct User {
    //A unique identifier for the user.
    const char *name;
    // Allows for safe authentication.
    const char *password_hash;
    // The home directory of the user.
    Path home_directory;
    // The permissions of the user.
    u16 permissions;
} User;

/**
 * A serializable representation of a group
 */
typedef struct Group {
    //A unique identifier for the group.
    const char *name;
    // The permissions of the group.
    u16 permissions;
} Group;

typedef enum fs_node_type {
    NODE_FILE,
    NODE_DIRECTORY,
    NODE_SYMLINK
} fs_node_type;


typedef struct fs_node {
    //The path of the node relative to the root.
    Path path;
    // The Parent of the node.
    struct fs_node *parent;
    // the type of the node.
    fs_node_type type;
    // A union of the data that can be stored in the node, can be a directory or a file.
    union {
        // The data of the node if it is a directory.
        struct {
            //A darray of the children for the directory.
            struct fs_node **children;
            // The number of children in the directory.
            u32 child_count;
        } directory;
        // The data of the node if it is a file.
        struct {
            // The size of the file in bytes.
            u64 size;
            // The raw data of the file.
            char *data;
        } file;
    } data;
} fs_node;


/**
 * This function will initialize the vfs file system.
 * @param root The root path of the vfs file system.
 * @return true if the vfs file system was initialized successfully, false otherwise.
 */
b8 vfs_initialize(Path root);


/**
 * This function will return the node at the given path. If the path is invalid, the node will be NULL.
 * It uses the cached nodes in memory and does not attempt to load the node from the file system.
 * @param path The path to get the node from.
 * @return The node at the given path or NULL if the path is invalid.
 */
b8 vfs_exists(Path path);

/**
 * This function will return the node at the given path. If the path is invalid, the node will be NULL.
 * @param path  The path to get the node from.
 * @return The node at the given path or NULL if the path is invalid.
 */
fs_node *vfs_get(Path path);

/**
 * This function will return the node at the given path. If the path is invalid, the node will be NULL.
 * @param node The node to get the path from.
 * @return The path of the node.
 */
char *vfs_node_to_string(fs_node *node);

/**
 * @breif this function will create a string representation of the vfs file system.
 */
char *vfs_to_string();

/**
 * @brief shuts down the vfs file system, cleaning up any resources.
 */
void vfs_shutdown();

