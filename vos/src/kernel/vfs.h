/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

#include "defines.h"
#include "kresult.h"
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

typedef enum NodeType {
  NODE_TYPE_FILE,
  NODE_TYPE_DIRECTORY,
} NodeType;

typedef struct Node {
  // The type of the node.
  NodeType type;
  // The handle to the node.
  Handle handle;
  // The handle to the parent node, can be null.
  Handle parent_handle;
  // The path of the node, we can retrieve the name from this.
  const char *path;
  // The permissions of the node.
  u16 permissions;
  // The owner of the node.
  User *owner;
  // The groups that the node belongs to.
  Group *group;
  // The number of groups that the node belongs to. Maximum of 255.
  u8 group_count;
  // Metadata for the node.
  u64 creation_time;
  u64 modification_time;
  u64 access_time;
  u64 size;
  union {
    struct {
      u64 size;
      char *content;
    } file;
    struct {
      struct Node **children;
      u32 child_count;
      // Directory-specific metadata, if any.
    } directory;
    struct {
      char *target_path; // This will store the path that the symlink points to.
    } symlink;
  };
} Node;

typedef struct Vfs {
  // The root node of the file system. This is a tree structure that represents the entire file system.
  Node *root;
  // The number of nodes in the file system. Maximum of 1024.
  u16 user_count;
  // The number of groups in the file system. Maximum of 1024.
  u16 group_count;
  // The users in the file system.
  User *users;
  // The groups in the file system.
  Group *groups;
} Vfs;

typedef struct FileSystemContext {
  // The file system.
  Vfs *file_system;
  // The current working directory.
  Node *current_directory;
  // The current user.
  User *current_user;
} FileSystemContext;

/**
 * Initializes the virtual file system.
 * This function sets up the necessary structures and state for the VFS to operate.
 * It prepares the system to manage, store, and operate on nodes (files, directories, etc.)
 * @return A KernelResult indicating success or failure.
 */
KernelResult vfs_initialize();

/**
 * Cleans up and releases all resources associated with the VFS.
 * This function should be called when the VFS is no longer needed.
 * @param fs The file system to shut down.
 * @return A KernelResult indicating success or failure.
 */
KernelResult vfs_shutdown(Vfs *fs);

/**
 * Adds a new user to the VFS.
 * This function will create a unique representation for the user in the VFS and set up a home directory for them.
 * @param fs The file system where the user should be added.
 * @param name The unique name for the user.
 * @param password The raw password for the user; this will be hashed internally for security.
 * @param permissions The permissions bitmask for this user.
 * @return A KernelResult with status indicating success, failure, or specific issues.
 */
KernelResult vfs_add_user(Vfs *fs, const char *name, const char *password, u16 permissions);

/**
 * Remove a user from the file system. This will delete the user's home directory.
 * @param fs the file system to remove the user from
 * @param name the name of the user
 * @return KERNEL_SUCCESS if the user was successfully removed, else an error code.
 */
KernelResult vfs_remove_user(Vfs *fs, const char *name);

/**
 * Add a group to the file system. This will create a group with no users.
 * @param fs the file system to add the group to
 * @param name the name of the group
 * @param permissions the permissions of the group
 * @return KERNEL_VFS_GROUP_CREATED if the group was successfully created, else an error code.
 */
KernelResult vfs_add_group(Vfs *fs, const char *name, u16 permissions);
/**
 * Remove a group from the file system. This will remove the group from all users.
 * @param fs the file system to remove the group from
 * @param name the name of the group
 * @return KERNEL_SUCCESS if the group was successfully removed, else an error code.
 */
KernelResult vfs_remove_group(Vfs *fs, const char *name);

/**
 * Assign a user to a group in the file system.
 * @param fs the file system to assign the user to
 * @param user the user to assign
 * @param group the group to assign the user to
 * @return KERNEL_SUCCESS if the user was successfully assigned to the group, else an error code.
 */
KernelResult vfs_assign_user_to_group(Vfs *fs, User *user, Group *group);

/**
 * Remove a user from a group in the file system.
 * @param fs the file system to remove the user from
 * @param user the user to remove
 * @param group the group to remove the user from
 * @return KERNEL_SUCCESS if the user was successfully removed from the group, else an error code.
 */
KernelResult vfs_remove_user_from_group(Vfs *fs, User *user, Group *group);

/**
 * Create a new file system context.
 * @param fs the file system to create the context for
 * @return a pointer to the file system context
 */
KernelResult vfs_create_node(Vfs *fs, NodeType type, const char *path, u16 permissions);
/**
 * Add a node to the file system. This will import recursively all nodes in the directory.
 * This uses the path of the node to determine where to add it.
 *
 * @note This imposes assumptions about mingw-w64's unix-like path handling. or related libraries.
 *
 * @param fs the file system to add the node to
 * @param node the node to add
 * @return KERNEL_SUCCESS if the node was successfully added, else an error code.
 */
KernelResult vfs_add_node(Vfs *fs, Node *node);

/**
 * Delete a node from the file system.
 * @param fs the file system to delete the node from
 * @param path the path of the node to delete
 * @return KERNEL_SUCCESS if the node was successfully deleted, else an error code.
 */
KernelResult vfs_delete_node(Vfs *fs, const char *path);

/**
 * Change the permissions of a node.
 * @param fs the file system to change the permissions in
 * @param path the path of the node to change the permissions of
 * @param permissions the new permissions of the node
 * @return KERNEL_SUCCESS if the permissions were successfully changed, else an error code.
 */
KernelResult vfs_change_node_permissions(Vfs *fs, const char *path, u16 permissions);

/**
 * Move a node to a new path.
 * @param fs the file system to change the owner in
 * @param path the path of the node to change the owner of
 * @param user the new owner of the node
 * @return KERNEL_SUCCESS if the owner was successfully changed, else an error code.
 */
KernelResult vfs_move_node(Vfs *fs, const char *old_path, const char *new_path);

/**
 * Find a node by path.
 * @param fs the file system to change the owner in
 * @param path the path of the node to change the owner of
 * @param user the new owner of the node
 * @return KERNEL_SUCCESS if the owner was successfully changed, else an error code.
 */
Node *vfs_find_node_by_path(Vfs *fs, const char *path);

/**
 * Read from a file node
 * @param file_node the file node to read from
 * @param offset the offset to start reading from
 * @param length the number of bytes to read
 * @return KERNEL_VFS_NODE_READ if the node was successfully read, else an error code.
 */
KernelResult vfs_read_file(Node *file_node, u64 offset, u64 length);

/**
 * Write to a file node at a given offset
 * @param file_node the file node to write to
 * @param offset the offset to start writing to
 * @param content the content to write
 * @return KERNEL_SUCCESS if the node was successfully written to, else an error code.
 */
KernelResult vfs_write_file(Node *file_node, u64 offset, const char *content);

/**
 * Lists the contents of a directory node
 * @param directory_node the directory node to list
 * @return a pointer to an array of pointers to the nodes in the directory
 */
KernelResult vfs_list_directory(Node *directory_node);

/**
 * Create a symlink node in the file system
 * @param fs  the file system to create the symlink in
 * @param target_path the path that the symlink points to
 * @param symlink_path the path of the symlink
 * @return KERNEL_SYM_LINK_CREATED if the symlink was successfully created, else an error code.
 */
KernelResult vfs_create_symlink(Vfs *fs, const char *target_path, const char *symlink_path);

/**
 * Change the current working directory in the context
 * @param context  the file system context
 * @param path the path to change to
 * @return KERNEL_SUCCESS if the directory was successfully changed, else an error code.
 */
KernelResult vfs_change_directory(FileSystemContext *context, const char *path);

/**
 * Change the current user in the context
 * @param context  the file system context
 * @param user the user to change to
 * @return KERNEL_SUCCESS if the user was successfully changed, else an error code.
 */
KernelResult vfs_change_user(FileSystemContext *context, User *user);

/**
 * Checks if a user possesses the required permissions to access a specific node.
 * This is useful for operations like reading, writing, or executing actions on files and directories.
 * @param node The node (file or directory) to check.
 * @param user The user whose permissions need to be verified.
 * @param required_permissions The bitmask of permissions that are required for the desired action.
 * @return A KernelResult with status indicating if the user has the necessary permissions or not.
 */
KernelResult vfs_check_permissions(Node *node, User *user, u16 required_permissions);

/**
 * Serializes the entire state of the VFS to a binary file.
 * This is useful for backup, migration, or other operations where the state of the VFS needs to be persisted.
 * @param fs The file system to serialize.
 * @param filename The name of the file where the serialized data will be stored.
 * @return A KernelResult indicating success or the specifics of any failure.
 */
KernelResult vfs_serialize_to_file(Vfs *fs, const char *filename);

/**
 * Restores the state of the VFS from a binary file.
 * This function reads a previously serialized VFS state and initializes the VFS to that state.
 * @param filename The name of the file from which to read the serialized VFS state.
 * @return A KernelResult indicating success or detailing any issues encountered during deserialization.
 */
KernelResult vfs_deserialize_from_file(const char *filename);
