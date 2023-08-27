#include <string.h>
#include "vfs.h"
#include "defines.h"
#include "core/mem.h"
#include "core/str.h"
#include "containers/darray.h"

KernelResult vfs_initialize() {
    Vfs *vfs = kallocate(sizeof(Vfs), MEMORY_TAG_VFS);
    Node *root = kallocate(sizeof(Node), MEMORY_TAG_VFS);
    root->path = string_duplicate("/");
    root->type = NODE_TYPE_DIRECTORY;
    root->directory.child_count = 0;
    root->directory.children = darray_create(Node);
    vfs->root = root;
    vfs->users = darray_create(User);
    vfs->groups = darray_create(Group);
    KernelResult result = {KERNEL_SUCCESS, vfs};
    return result;
}

Node *vfs_resolve_path_to_node(Vfs *vfs, const char *path) {
    Node *current = vfs->root;
    char *token = strtok(path, "/");
    while (token) {
        b8 found = false;
        for (u32 i = 0; i < current->directory.child_count; i++) {
            if (strcmp(current->directory.children[i]->path, token) == 0) {
                current = current->directory.children[i];
                found = true;
                break;
            }
        }
        if (!found) return null;
        token = strtok(null, "/");
    }
    return current;
}

KernelResult vfs_add_user(Vfs *fs, const char *name, const char *password, u16 permissions) {
    if (fs->user_count >= 1024) {
        return (KernelResult) {KERNEL_ERROR, "The maximum number of users has been reached."};
    }
    User *user = &fs->users[fs->user_count];
    user->permissions = permissions;
    user->name = string_duplicate(name);
    //TODO: hash the password
    user->password_hash = string_duplicate(password);
    fs->user_count++;
    //Grow the user array
    return (KernelResult) {KERNEL_SUCCESS, user};
}

KernelResult vfs_add_node(Vfs *vfs, Node *node) {
    if (vfs->root == 0) {
        vfs->root = node;
        return (KernelResult) {KERNEL_SUCCESS, null};
    }

    char *parent_path = string_duplicate(node->path);
    // Remove the last token from the path.
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }
    Node *parent = vfs_resolve_path_to_node(vfs, parent_path);
    kfree(parent_path, string_length(parent_path) + 1, MEMORY_TAG_STRING);
    return (KernelResult) {KERNEL_SUCCESS, null};
}

b8 vfs_destroy_node(Node *node) {
    if (node->type == NODE_TYPE_DIRECTORY) {
        for (u32 i = 0; i < node->directory.child_count; ++i) {
            vfs_destroy_node(node->directory.children[i]);
        }
        darray_destroy(node->directory.children);
    }
    kfree(node->path, string_length(node->path) + 1, MEMORY_TAG_STRING);
    kfree(node, sizeof(Node), MEMORY_TAG_VFS);
    return true;
}

KernelResult vfs_shutdown(Vfs *fs) {
    darray_destroy(fs->users);
    darray_destroy(fs->groups);
    if (fs->root)
        vfs_destroy_node(fs->root);
    kfree(fs, sizeof(Vfs), MEMORY_TAG_VFS);
    return (KernelResult) {KERNEL_SUCCESS, null};
}

