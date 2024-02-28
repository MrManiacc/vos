/**
 * Created by jwraynor on 2/28/2024.
 */
#include "kern/vfs.h"

#include <stdlib.h>
#include <string.h>

#include "containers/darray.h"
#include "core/vlogger.h"
#include "core/vstring.h"
#include "platform/platform.h"

Vfs *vfs_init(const char *name, const char *root) {
    Vfs *vfs = malloc(sizeof(Vfs));
    vfs->name = name;
    vfs->path = root;
    vfs->root = null;
    vfs->nodes = dict_new();
    vfs->root = vfs_load(vfs, root);
    vfs->root->parent = null;

    return vfs;
}

void vfs_discover(VfsHandle *handle) {
    dict_set(handle->vfs->nodes, handle->path, handle);
    // This will walk the file system and discover all the nodes but will not load them into memory.
    //If we're a directory, we need to discover all the children.
    //If we're a file, we do nothing right now.
    // If we're a symlink, we need to discover the target.
    if (handle->type == VFS_DIRECTORY) {
        FilePathList *files = platform_collect_files_direct(handle->path);
        if (files) {
            handle->node.directory.children = dict_new();
            for (int i = 0; i < files->count; ++i) {
                VfsHandle *childHandle = platform_allocate(sizeof(VfsHandle), false);
                childHandle->name = platform_file_name(files->paths[i]);
                childHandle->path = platform_file_append(handle->path, childHandle->name);
                childHandle->status = platform_file_exists(childHandle->path) ? VFS_NODE_FOUND : VFS_NODE_NOT_FOUND;
                childHandle->vfs = handle->vfs;
                childHandle->parent = handle;
                if (childHandle->status == VFS_NODE_FOUND) {
                    if (platform_is_directory(childHandle->path)) {
                        childHandle->type = VFS_DIRECTORY;
                    } else if (platform_is_file(childHandle->path)) {
                        childHandle->type = VFS_FILE;
                    } else if (platfrom_is_symbolic_link(childHandle->path)) {
                        childHandle->type = VFS_SYMLINK;
                    }
                }
                vfs_discover(childHandle); // Recursively discover
                dict_set(handle->node.directory.children, childHandle->name, childHandle);
            }
            platform_file_path_list_free(files);
        }
    } else if (handle->type == VFS_SYMLINK) {
        char *targetPath = platform_resolve_symlink(handle->path);
        if (targetPath) {
            handle->node.symlink.target_path = string_duplicate(targetPath);
            handle->node.symlink.target = vfs_load(handle->vfs, handle->node.symlink.target_path);
            handle->node.symlink.target->parent = handle;
            handle->status = handle->node.symlink.target->status;
            free(targetPath);
        }
    }
    // If it's a file, there's nothing to do here.
}

VfsHandle *vfs_load(const Vfs *fs, const char *abs_path) {
    //The path is expected to be relative to the root of the file system, in a standardized unix format.
    // I.e. if the root is /mnt/fs, and you want to open /mnt/fs/home/user/file.txt, you would pass /home/user/file.txt
    // Returns an empty handle with the VFS_NODE_NOT_FOUND status if the file is not found.
    // This will read the file from disk and load it into memory, updating the status to VFS_NODE_LOADED.
    VfsHandle *handle = platform_allocate(sizeof(VfsHandle), false);
    handle->vfs = fs;
    // the name of the handle after the last / in the path.
    handle->name = platform_file_name(abs_path);
    if (fs->root == null) {
        handle->path = string_duplicate(fs->path);
    } else {
        handle->path = platform_file_append(fs->path, handle->name);
    }

    // the absolute path of the file/directory.
    // The type of the file/directory.
    handle->status = platform_file_exists(handle->path) ? VFS_NODE_FOUND : VFS_NODE_NOT_FOUND;
    // Check the type of the file/directory.
    if (handle->status == VFS_NODE_FOUND) {
        if (platform_is_directory(handle->path)) {
            handle->type = VFS_DIRECTORY;
        } else if (platform_is_file(handle->path)) {
            handle->type = VFS_FILE;
        } else if (platfrom_is_symbolic_link(handle->path)) {
            handle->type = VFS_SYMLINK;
        }
    }
    vfs_discover(handle);
    return handle;
}

VfsHandle *vfs_get(const Vfs *fs, const char *rel_path) {
    // If the user passes "/", return the root.
    if (strcmp(rel_path, "/") == 0) {
        return fs->root;
    }

    // Start looking downward from the root.
    VfsHandle *current = fs->root;
    char *path = string_duplicate(rel_path); // Duplicate the path to avoid modifying the input.
    char *token = strtok(path, "/"); // Tokenize the path.

    while (token != NULL && current != NULL) {
        // Ensure we're looking into a directory before attempting to find children.
        if (current->type != VFS_DIRECTORY) {
            free(path);
            return NULL; // Not a directory, cannot contain children.
        }

        // Attempt to find the child in the current directory's children.
        VfsHandle *child = dict_get(current->node.directory.children, token);
        if (child == NULL) {
            // Child not found, the specified path does not exist.
            free(path);
            return NULL;
        }

        // Move to the next component of the path.
        current = child;
        token = strtok(NULL, "/");
    }

    return current; // Return the found handle or NULL if not found.
}

void **vfs_collect(const Vfs *fs, const char *query) {
    void **array = darray_create(VfsHandle*);
    // We iterate through all the nodes in the file system and return the first one that matches the query.
    dict_for_each(fs->nodes, VfsHandle, {
        if (strstr(value->path, query)) {
        darray_push(VfsHandle, array, *value);
        }
        })
    if (darray_length(array) == 0) {
        darray_destroy(array);
        vwarn("No nodes found matching query: %s", query);
        return null;
    }
    return array;
}

b8 vfs_write(VfsHandle *handle, void *data, const u64 size) {
    if (handle->type == VFS_DIRECTORY) {
        vwarn("Cannot write to a directory.");
        return false;
    }
    if (handle->type == VFS_SYMLINK) {
        // We need to write the target.
        vinfo("Writing symlink target: %s", handle->node.symlink.target_path);
        return vfs_write(handle->node.symlink.target, data, size);
    }
    //we just update the data and size.
    handle->node.file.data = data;
    handle->node.file.size = size;
    handle->status = VFS_NODE_NEEDS_WRITE;
    return true;
}


b8 vfs_read(VfsHandle *handle) {
    // Here we open a handle and read the file from disk. we then update the status to VFS_NODE_LOADED.
    if (handle->type == VFS_FILE) {
        // Open the file and read the contents.
        char *read = platform_read_file(handle->path);
        if (read) {
            handle->node.file.data = read;
            handle->node.file.size = strlen(read);
            handle->status = VFS_NODE_LOADED;
            return true;
        }
        handle->status = VFS_NODE_LOADED;
        return true;
    }
    if (handle->type == VFS_DIRECTORY) {
        // We need to read all the children.
        dict_for_each(handle->node.directory.children, VfsHandle, {
            vfs_read(value);
            })
        handle->status = VFS_NODE_LOADED;
        return true;
    }
    if (handle->type == VFS_SYMLINK) {
        // We need to read the target.
        vfs_read(handle->node.symlink.target);
        handle->status = VFS_NODE_LOADED;
        return true;
    }
    return false;
}

b8 vfs_mkdir(VfsHandle *handle, const char *name) {
    if (handle->type == VFS_FILE) {
        vwarn("Cannot create a directory in a file.");
        return false;
    }
    if (handle->type == VFS_SYMLINK) {
        // We need to create the directory in the target.
        return vfs_mkdir(handle->node.symlink.target, name);
    }
    if (dict_contains(handle->node.directory.children, name)) {
        vwarn("Directory %s, already exists in %s", name, handle->path);
        return false;
    }
    // We need to create the directory, but we don't need to write it to disk yet.
    // We just need to update the status to VFS_NODE_NEEDS_WRITE.
    const char *path = platform_file_append(handle->path, name);
    VfsHandle *child = platform_allocate(sizeof(VfsHandle), false);
    child->name = string_duplicate(name);
    child->type = VFS_DIRECTORY;
    child->path = path;
    child->status = VFS_NODE_FOUND;
    child->node.directory.children = dict_new();
    dict_set(handle->node.directory.children, name, child);
    handle->status = VFS_NODE_NEEDS_WRITE; // We need to write the directory to disk.
    return true;
}

b8 vfs_mkfile(VfsHandle *handle, const char *name) {
    if (handle->type == VFS_FILE) {
        vwarn("Cannot create a file in a file.");
        return false;
    }
    if (handle->type == VFS_SYMLINK) {
        // We need to create the file in the target.
        return vfs_mkfile(handle->node.symlink.target, name);
    }
    if (dict_contains(handle->node.directory.children, name)) {
        vwarn("File %s, already exists in %s", name, handle->path);
        return false;
    }
    // We need to create the file, but we don't need to write it to disk yet.
    // We just need to update the status to VFS_NODE_NEEDS_WRITE.
    const char *path = platform_file_append(handle->path, name);
    VfsHandle *child = platform_allocate(sizeof(VfsHandle), false);
    child->name = string_duplicate(name);
    child->type = VFS_FILE;
    child->path = path;
    child->status = VFS_NODE_FOUND;
    child->node.file.data = null;
    child->node.file.size = 0;
    dict_set(handle->node.directory.children, name, child);
    handle->status = VFS_NODE_NEEDS_WRITE; // We need to write the file to disk.
    return true;
}

b8 vfs_rm(VfsHandle *handle, const char *name) {
    if (handle->type == VFS_FILE) {
        vwarn("Cannot remove a file from a file.");
        return false;
    }
    if (handle->type == VFS_SYMLINK) {
        // We need to remove the file from the target.
        return vfs_rm(handle->node.symlink.target, name);
    }
    VfsHandle *child = dict_remove(handle->node.directory.children, name);
    if (child) {
        if (child->type == VFS_DIRECTORY) {
            // We need to remove all the children.
            dict_for_each(child->node.directory.children, VfsHandle, {
                vfs_rm(child, value->name);
                })
        }
        child->status = VFS_NODE_NEEDS_DELETE;
        return true;
    }
    return false;
}

b8 vfs_commit(VfsHandle *handle) {
    if (handle == NULL) {
        vwarn("vfs_commit: Handle is NULL.");
        return false;
    }

    switch (handle->status) {
        case VFS_NODE_NEEDS_WRITE: {
            // Depending on the node type, perform the appropriate disk operation.
            if (handle->type == VFS_FILE) {
                // Write file to disk with the data in `handle->node.file.data`.
                if (!platform_write_file(handle->path, handle->node.file.data, handle->node.file.size)) {
                    vwarn("Failed to write file: %s", handle->path);
                    return false;
                }
            } else if (handle->type == VFS_DIRECTORY) {
                // Create the directory on the disk if it doesn't exist.
                if (!platform_create_directory(handle->path)) {
                    vwarn("Failed to create directory: %s", handle->path);
                    return false;
                }
                // Recursively commit all children.
                dict_for_each(handle->node.directory.children, VfsHandle, {
                    if (!vfs_commit(value)) {
                    vwarn("Failed to commit child: %s", value->name);
                    // Continue trying to commit other children.
                    }
                    });
            } else if (handle->type == VFS_SYMLINK) {
                // Create symlink on the disk.
                if (!platform_create_symlink(handle->node.symlink.target_path, handle->path)) {
                    vwarn("Failed to create symlink: %s -> %s", handle->path, handle->node.symlink.target_path);
                    return false;
                }
            }
            handle->status = VFS_NODE_LOADED; // Update status to reflect changes are written.
            break;
        }
        case VFS_NODE_NEEDS_DELETE: {
            // Delete the file or directory from disk.
            if (!platform_delete_file(handle->path)) {
                vwarn("Failed to delete: %s", handle->path);
                return false;
            }
            handle->status = VFS_NODE_NOT_FOUND; // Update status to reflect deletion.
            dict_remove(handle->vfs->nodes, handle->path); // Remove the handle from the nodes dictionary.
            break;
        }
        default: {
            // For other statuses, no operation is required.
            vinfo("No write operation needed for: %s", handle->path);
            break;
        }
    }

    return true;
}


void vfs_append_handle(VfsHandle *handle, StringBuilder *sb, u32 indent) {
    const char *indentStr = string_repeat("\t", indent);
    sb_appendf(sb, "%sVfsHandle: %s\n", indentStr, handle->name);
    sb_appendf(sb, "%sPath: %s\n", indentStr, handle->path);
    sb_appendf(sb, "%sType: %s\n", indentStr, handle->type == VFS_FILE ? "File" : handle->type == VFS_DIRECTORY ? "Directory" : "Symlink");
    if (handle->type == VFS_FILE) {
        sb_appendf(sb, "%sSize: %lu\n", indentStr, handle->node.file.size);
    } else if (handle->type == VFS_DIRECTORY) {
        sb_appendf(sb, "%sChildren: %d\n", indentStr, dict_size(handle->node.directory.children));
        // Lets append the children.
        dict_for_each(handle->node.directory.children, VfsHandle, {
            vfs_append_handle(value, sb, indent + 1);
            })
    } else if (handle->type == VFS_SYMLINK) {
        sb_appendf(sb, "%sTarget: %s\n", indentStr, handle->node.symlink.target);
    }
}

char *vfs_to_string(VfsHandle *handle) {
    // This will return a string representation of the handle.
    // This is useful for debugging.
    StringBuilder *sb = sb_new();
    vfs_append_handle(handle, sb, 0);
    char *result = sb_build(sb);
    sb_free(sb);
    return result;
}
