#include <sys/stat.h>
#include "vfs.h"
#include "core/mem.h"
#include "containers/dict.h"
#include "core/logger.h"
#include "vfs_watch.h"
#include "core/timer.h"
#include "core/event.h"

static Fs *fs = NULL;
static dict *loaded_nodes = NULL;

b8 fs_on_file_event(u16 code, void *sender, void *listener_inst, event_context data);

b8 fs_initialize(Path root) {
    if (fs != NULL) {
        verror("VFS was already initialized!");
        return false;
    }
    //TODO internally create the file tree structure recursively from the root at startup and then just update it here
    fs = kallocate(sizeof(Fs), MEMORY_TAG_VFS);
    fs->running = true;
    loaded_nodes = dict_create_default();
    event_register(EVENT_FILE_CREATED, 0, fs_on_file_event);
    event_register(EVENT_FILE_MODIFIED, 0, fs_on_file_event);
    event_register(EVENT_FILE_DELETED, 0, fs_on_file_event);
    watcher_initialize(root, &fs->running);
    return true;
}

b8 fs_exists(Path path) {
    return false;
}

void fs_shutdown() {
    event_unregister(EVENT_FILE_CREATED, 0, fs_on_file_event);
    event_unregister(EVENT_FILE_MODIFIED, 0, fs_on_file_event);
    event_unregister(EVENT_FILE_DELETED, 0, fs_on_file_event);
    fs->running = false;
    watcher_shutdown();
    dict_destroy(loaded_nodes);
    kfree(fs, sizeof(Fs), MEMORY_TAG_VFS);
}

b8 fs_on_file_event(u16 code, void *sender, void *listener_inst, event_context data) {
    char *path = (char *) data.data.c;

    switch (code) {
        case EVENT_FILE_CREATED: {
            vdebug("File was created: %s\n\tloading from path...", path)
            fs_load_node_from(path);
            break;
        }
        case EVENT_FILE_MODIFIED:vdebug("File modified: %s", path)

            break;
        case EVENT_FILE_DELETED: {
            if (dict_lookup(loaded_nodes, path) != NULL)
                dict_remove(loaded_nodes, path);
            vdebug("File deleted: %s", path)
            break;
        }
    }
    return false;
}

/**
 * Creates a new node from the given path. If the path is a directory, the node will be a directory
 * node. If the path is a file, the node will be a file node. If the path is invalid, the node will
 * be NULL.
 * @param path The path to create a node from.
 * @return
 */
Node *fs_load_node_from(Path path) {

    //Use fopen and fstat to get information about the file
    struct stat file_stat;
    if (stat(path, &file_stat) != 0) {
        verror("Failed to get file stat for %s", path);
        return NULL;
    }

    //Create a new node
    Node *node = kallocate(sizeof(Node), MEMORY_TAG_VFS);
    node->path = path;
    node->parent = NULL;
    //TODO: load the node data

    //Add the node to the loaded nodes dictionary. We only update this if the file modified event callback is fired.
    dict_insert(loaded_nodes, path, node);
    vdebug("Loaded node from path: %s", dict_to_string(loaded_nodes))
    return node;
}

/**
 * Gets the node at the given path. If the path is invalid, the node will be NULL. This assumes that
 * the requested node has been loaded into memory using fs_load_node_from.
 * @param path The path to get the node from.
 * @return The node at the given path.
 */
Node *fs_get_node(Path path) {
    Node *node = dict_lookup(loaded_nodes, path);
    if (node != NULL)
        return node;
    return fs_load_node_from(path);
}