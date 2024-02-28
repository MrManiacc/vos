/**
 * Created by jwraynor on 2/28/2024.
 */
#pragma once
#include "defines.h"
#include "containers/dict.h"


typedef enum VfsType {
    VFS_FILE,
    VFS_DIRECTORY,
    VFS_SYMLINK
} VfsType;


typedef enum VfsStatus {
    VFS_NODE_NOT_FOUND, //The node was not found.
    VFS_NODE_FOUND, //We found the node, but it's not loaded into memory.
    VFS_NODE_LOADED, //We've loaded the node from disk, and it's in memory. The FS handle should be closed at this point, we don't like files
    VFS_NODE_NEEDS_RELOAD, //The system file has been modified and needs to be reloaded.
    VFS_NODE_NEEDS_WRITE, //The system file has been modified and needs to be written to disk.
    VFS_NODE_NEEDS_DELETE, //The system file has been modified and needs to be written to disk.
} VfsStatus;

typedef union VfsNode {
    struct {
        u64 size; // The size of the file in bytes.
        void *data; // A pointer to the raw data of the file.
    } file;

    struct {
        Dict *children; // A dictionary of child nodes.
    } directory;

    struct {
        const char *target_path; // The target of the symlink.
        struct VfsHandle *target; // The target of the symlink.
    } symlink;
} VfsNode;

typedef struct VfsHandle {
    const char *name; // The relative name of the file/directory.
    const char *path; // The absolute path of the file/directory.
    VfsType type; // The type of the file/directory.
    VfsNode node; // The data of the file/directory.
    VfsStatus status; // The status of the root node.
    const struct Vfs *vfs;
    const struct VfsHandle *parent;
} VfsHandle;


typedef struct Vfs {
    const char *name; // The name of the file system.
    const char *path; // The root directory of the file system.
    VfsHandle *root; // The root node of the file system.
    Dict *nodes; // A dictionary of all the nodes in the file system.
} Vfs;

// This will walk the file system and discover all the nodes but will not load them into memory.
// We're basically building a tree of the file system before we load anything into memory.
// IMPORTANT: This function should only be called once during the initialization of the file system.
// the root is expected to be in the platform specific format.
// I.e. /mnt/fs or C:\mnt\fs
Vfs *vfs_init(const char *name, const char *root);

//The path is expected to be relative to the root of the file system, in a standardized unix format.
// I.e. if the root is /mnt/fs, and you want to open /mnt/fs/home/user/file.txt, you would pass /home/user/file.txt
// Returns an empty handle with the VFS_NODE_NOT_FOUND status if the file is not found.
// This will read the file from disk and load it into memory, updating the status to VFS_NODE_LOADED.
VfsHandle *vfs_load(const Vfs *fs, const char *abs_path);

// Finds a file using the relative path by walking the file system, starting at the root.
VfsHandle *vfs_get(const Vfs *fs, const char *rel_path);

/// Allows you to use regex to find a file in the file system.
/// Returns a darray of VfsHandle pointers.
Dict *vfs_collect(const Vfs *fs, const char *query) ;

// This will simply update the status of the node to VFS_NODE_NEEDS_WRITE and will not write the file to disk.
// The actual file data in memory will be updated but will not be written to disk at this time.
b8 vfs_write(VfsHandle *handle, void *data, u64 size);

// This will read the file data from disk and update the status to VFS_NODE_LOADED.
// If this is a directory, it will read the directory from disk and all of its children.
// If it's a file, it will read the file from disk.
b8 vfs_read(VfsHandle *handle);

// This will create a new directory in the file system and update the status to VFS_NODE_NEEDS_WRITE.
// The actual file data in memory will be updated but will not be written to disk at this time.
b8 vfs_mkdir(VfsHandle *handle, const char *name);

// This will create a new file in the file system and update the status to VFS_NODE_NEEDS_WRITE.
// The actual file data in memory will be updated but will not be written to disk at this time.
b8 vfs_mkfile(VfsHandle *handle, const char *name);

// This will remove the file from the file system and update the status to VFS_NODE_NEEDS_WRITE.
// The actual file data in memory will be updated but will not be written to disk at this time.
b8 vfs_rm(VfsHandle *handle, const char *name);

// This will write the file to disk and update the status to VFS_NODE_LOADED.
// If this is a directory, it will write the directory to disk and all of its children.
b8 vfs_commit(VfsHandle *handle);

//Builds a string representation of the file system.
char *vfs_to_string(VfsHandle *handle);

// This will free the memory of the file system and all of its children.
void vfs_free(Vfs *fs);
