/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

//#include <pthread.h>
#include "defines.h"
#include "kernel/kresult.h"

typedef const char *Path;
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

typedef struct Fs Fs;

typedef enum NodeAction {
  NODE_CREATED,
  NODE_DELETED,
  NODE_MODIFIED,
  NODE_RENAMED
} NodeAction;

typedef enum NodeType {
  NODE_FILE,
  NODE_DIRECTORY,
  NODE_SYMLINK
} NodeType;
typedef struct Node Node;

b8 fs_initialize(Path root);

/**
 * This function will return the node at the given path. If the path is invalid, the node will be NULL.
 * It will attempt to load the node from the file system if it is not already loaded. The node will be cached
 * in memory for future use. We also watch the file system for changes to the node and update the node
 * accordingly to keep it in sync with the file system. This method is thread safe and guarantees that
 * a node will only be loaded once.
 * @param path The path to get the node from. Path can be absolute or relative to the current working directory, this is
 * determined by the first character of the path. If the path starts with a '/', it is absolute. Otherwise,
 * it is relative to the current working directory.
 *
 * @param action The action that was performed on the node. This is used to determine how to update the node.
 *
 * Windows paths will be converted to Unix paths, with /c/ being the root of the C drive.
 * @return The node at the given path or NULL if the path is invalid.
 */
Node *fs_sync_node(Path path, NodeAction action);

/**
 * This function will return the node at the given path. If the path is invalid, the node will be NULL.
 * It uses the cached nodes in memory and does not attempt to load the node from the file system.
 * @param path The path to get the node from.
 * @return The node at the given path or NULL if the path is invalid.
 */
Node *fs_node_exists(Path path);

char *fs_to_string();

char *fs_node_to_string(Node *node);

void fs_shutdown();