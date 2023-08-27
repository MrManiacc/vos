#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "vfs.h"
#include "defines.h"
#include "core/mem.h"
#include "core/str.h"
#include "containers/darray.h"
#include "pthread.h"
#include "core/logger.h"
#include "core/event.h"
#define _DIRENT_HAVE_D_TYPE
#include <dirent.h>
#include <stdio.h>
static Vfs *vfs;

static pthread_t watcher_thread;
static b8 should_run = true;

u64 vfs_last_modified_time(Node *node) {
    pthread_mutex_lock(&vfs->vfs_mutex); // Lock the mutex
//    get the most recent modified time of the node and all of its children

    char *path = string_duplicate(node->path);
    // Check the last modified time of the file.
    struct stat file_stat;
    stat(path, &file_stat);
//    vdebug("File: %s, Last modified: %d", path, file_stat.st_mtime);
//    update the node's modification time
    node->modification_time = file_stat.st_mtime;

    u64 most_recent = node->modification_time;

    if (node->type == NODE_TYPE_DIRECTORY) {
        for (u32 i = 0; i < node->directory.child_count; ++i) {
            u64 child_time = vfs_last_modified_time(node->directory.children[i]);
            if (child_time > most_recent) {
                most_recent = child_time;
            }
        }
    }else{
        most_recent = node->modification_time;
    }
    pthread_mutex_unlock(&vfs->vfs_mutex); // Unlock the mutex
    return most_recent;
}


// Called on a separate thread to watch for changes to the file system.
void *watch_directories(void *arg) {

    u64 first_modified = vfs_last_modified_time(vfs->root);
    while (should_run) {
        u64 last_modified = vfs_last_modified_time(vfs->root);
        if (last_modified > first_modified) {
            vdebug("File system changed");
            first_modified = last_modified;
        }
//        vdebug("Last modified: %d", last_modified);
        usleep(100000); // Sleep for 100ms before checking again.
    }
}




KernelResult vfs_initialize(Node *root_path) {
    vfs = kallocate(sizeof(Vfs), MEMORY_TAG_VFS);
    Node *root = kallocate(sizeof(Node), MEMORY_TAG_VFS);
    pthread_mutex_init(&vfs->vfs_mutex, NULL);
    root->path = string_duplicate(root_path);
    root->type = NODE_TYPE_DIRECTORY;
    root->directory.child_count = 0;
    root->directory.children = darray_create(Node);
    vfs->root = root;
    vfs->users = darray_create(User);
    vfs->groups = darray_create(Group);
    pthread_create(&watcher_thread, null, watch_directories, vfs);
    KernelResult result = {KERNEL_SUCCESS, vfs};
    return result;
}

Node * vfs_resolve_path_to_node(const char *path) {
    pthread_mutex_lock(&vfs->vfs_mutex); // Lock the mutex
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
        if (!found) {
            pthread_mutex_unlock(&vfs->vfs_mutex); // Unlock the mutex
            return null;
        }
        token = strtok(null, "/");
    }
    pthread_mutex_unlock(&vfs->vfs_mutex); // Unlock the mutex
    return current;
}

KernelResult vfs_add_user(const char *name, const char *password, u16 permissions) {
    pthread_mutex_lock(&vfs->vfs_mutex); // Lock the mutex
    if (vfs->user_count >= 1024) {
        return (KernelResult) {KERNEL_ERROR, "The maximum number of users has been reached."};
    }
    User *user = &vfs->users[vfs->user_count];
    user->permissions = permissions;
    user->name = string_duplicate(name);
    //TODO: hash the password
    user->password_hash = string_duplicate(password);
    vfs->user_count++;
    pthread_mutex_unlock(&vfs->vfs_mutex); // Lock the mutex

    //Grow the user array
    return (KernelResult) {KERNEL_SUCCESS, user};
}

KernelResult vfs_add_node(Node *node) {
    pthread_mutex_lock(&vfs->vfs_mutex); // Lock the mutex
    if (vfs->root == 0) {
        vfs->root = node;
        pthread_mutex_unlock(&vfs->vfs_mutex); // Lock the mutex

        return (KernelResult) {KERNEL_SUCCESS, null};
    }

    char *parent_path = string_duplicate(node->path);
    // Remove the last token from the path.
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }
    Node *parent = vfs_resolve_path_to_node(parent_path);
    kfree(parent_path, string_length(parent_path) + 1, MEMORY_TAG_STRING);
    pthread_mutex_unlock(&vfs->vfs_mutex); // Lock the mutex
    return (KernelResult) {KERNEL_SUCCESS, null};
}

b8 vfs_destroy_node(Node *node) {
    pthread_mutex_lock(&vfs->vfs_mutex); // Lock the mutex
    if (node->type == NODE_TYPE_DIRECTORY) {
        for (u32 i = 0; i < node->directory.child_count; ++i) {
            vfs_destroy_node(node->directory.children[i]);
        }
        darray_destroy(node->directory.children);
    }
    kfree(node->path, string_length(node->path) + 1, MEMORY_TAG_STRING);
    kfree(node, sizeof(Node), MEMORY_TAG_VFS);
    pthread_mutex_unlock(&vfs->vfs_mutex); // Lock the mutex
    return true;
}

KernelResult vfs_shutdown() {
    darray_destroy(vfs->users);
    darray_destroy(vfs->groups);
    if (vfs->root)
        vfs_destroy_node(vfs->root);
    should_run = false;
    pthread_join(watcher_thread, null);
    pthread_mutex_destroy(&vfs->vfs_mutex); // Destroy the mutex
    kfree(vfs, sizeof(Vfs), MEMORY_TAG_VFS);
    return (KernelResult) {KERNEL_SUCCESS, null};
}

