/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

#include <pthread.h>
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

typedef u64 Handle;

typedef struct Node {
  //The path of the node relative to the root.
  Path path;
  // The Parent of the node.
  struct Node *parent;
  // A union of the data that can be stored in the node, can be a directory or a file.
  union {
    // The data of the node if it is a directory.
    struct {
      // The children of the directory.
      struct Node *children[NODE_CAPACITY];
      // The number of children in the directory.
      u32 child_count;
    } directory;
    // The data of the node if it is a file.
    struct {
      // The size of the file in bytes.
      u64 size;
      // The raw data of the file.
      const char *data;
    } file;
  } data;
} Node;

typedef struct Fs {
  //The root directory of the file system.
  Path root;
  //The current working directory of the file system.
  Path cwd;
  //The current user of the file system.
  User *user;
  //The current group of the file systems.
  Group *group;
  //Whether the file system is running.
  b8 running;
} Fs;

b8 fs_initialize(Path root);

/**
 * Creates a new node from the given path. If the path is a directory, the node will be a directory
 * node. If the path is a file, the node will be a file node. If the path is invalid, the node will
 * be NULL.
 * @param path The path to create a node from.
 * @return
 */
Node *fs_load_node_from(Path path);

/**
 * Gets the node at the given path. If the path is invalid, the node will be NULL. This assumes that
 * the requested node has been loaded into memory using fs_load_node_from.
 * @param path The path to get the node from.
 * @return The node at the given path.
 */
Node *fs_get_node(Path path);

void fs_shutdown();