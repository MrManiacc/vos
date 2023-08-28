#include <sys/stat.h>

#include "core/mem.h"
#include "containers/dict.h"
#include "core/logger.h"
#include "vfs_watch.h"
#include "core/timer.h"
#include "core/event.h"
#include "containers/darray.h"

#include <stdio.h>
#include <dirent.h>
#include "vfs.h"
#include "core/str.h"

static Fs *fs = NULL;
static dict *loaded_nodes = NULL;
//Loaders by extension
static dict *asset_loaders = NULL;

b8 fs_on_file_event(u16 code, void *sender, void *listener_inst, event_context data);
void fs_node_destroy(Node *node);

typedef struct Fs {
  //The root directory of the file system.
  Node *root;
  //The current working directory of the file system.
  Path cwd;
  //The current user of the file system.
  User *user;
  //The current group of the file systems.
  Group *group;
  //Whether the file system is running.
  b8 running;
} Fs;

b8 fs_initialize(Path root) {
    if (fs != NULL) {
        verror("VFS was already initialized!");
        return false;
    }
    fs = kallocate(sizeof(Fs), MEMORY_TAG_VFS);
    kzero_memory(fs, sizeof(Fs));
    //Load the root node, this will recursively load all children
    loaded_nodes = dict_create_default();
    fs->root = fs_sync_node(root, NODE_CREATED);
    fs->running = true;
    event_register(EVENT_FILE_CREATED, 0, fs_on_file_event);
    event_register(EVENT_FILE_MODIFIED, 0, fs_on_file_event);
    event_register(EVENT_FILE_DELETED, 0, fs_on_file_event);
    watcher_initialize(root, &fs->running);
    vwarn(fs_to_string());
    char *test = fs_node_to_string(fs->root);
    if (test == NULL) {
        verror("Could not convert node to string");
        return false;
    }
    vinfo("Initialized VFS with root: %s", test);
    return true;
}

void fs_register_asset_loader(asset_loader *loader) {
    if (asset_loaders == NULL) {
        asset_loaders = dict_create_default();
        //TODO: add default asset loaders here
    }
    if (dict_lookup(asset_loaders, loader->extension) != NULL) {
        vwarn("Asset loader for extension: %s already exists!", loader->extension);
        return;
    }
    dict_insert(asset_loaders, loader->extension, loader);
}

void fs_shutdown() {
    event_unregister(EVENT_FILE_CREATED, 0, fs_on_file_event);
    event_unregister(EVENT_FILE_MODIFIED, 0, fs_on_file_event);
    event_unregister(EVENT_FILE_DELETED, 0, fs_on_file_event);
    fs->running = false;
    watcher_shutdown();
    fs_node_destroy(fs->root);
    dict_destroy(loaded_nodes);
    kfree(fs, sizeof(Fs), MEMORY_TAG_VFS);
}

b8 fs_on_file_event(u16 code, void *sender, void *listener_inst, event_context data) {
    char *path = (char *) data.data.c;
    switch (code) {
        case EVENT_FILE_CREATED: {
            vdebug("File was created: %s\n\tloading from path...", path)
            Node *node = fs_sync_node(path, NODE_CREATED);
            char *parent_path = string_format("%s", path);
            char *last_slash = strrchr(parent_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
            }
            Node *parent = fs_sync_node(parent_path, NODE_MODIFIED);
            if (parent == NULL) {
                verror("Could not sync parent node: %s", parent_path);
                return false;
            }
            if (node->parent == NULL) {
                vdebug("Setting parent of node: %s to: %s", node->path, parent->path)
                node->parent = parent;
            }
            break;
        }
        case EVENT_FILE_MODIFIED: {
            Node *synced = fs_sync_node(path, NODE_MODIFIED);
            if (synced == NULL) {
                verror("Could not sync node: %s", path);
                return false;
            }
            char *parent_path = string_format("%s", path);
            char *last_slash = strrchr(parent_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
            }
            Node *parent = fs_sync_node(parent_path, NODE_MODIFIED);
            if (parent == NULL) {
                verror("Could not sync parent node: %s", parent_path);
                return false;
            }
            if (synced->parent == NULL) {
                vdebug("Setting parent of node: %s to: %s", synced->path, parent->path)
                synced->parent = parent;
            }

            break;
        }

        case EVENT_FILE_DELETED: {
            vdebug("File deleted: %s", path)
            fs_sync_node(path, NODE_DELETED);
            break;
        }
    }
//    vinfo(fs_node_to_string(fs->root))

    return true;
}

b8 fs_node_exists(Path path) {
    return fs_get_node(path) != NULL;
}

Node *load_directory_node(struct stat file_stat, Path path, NodeAction action) {
    // We need to create a new node and load it from the file system
    if (action == NODE_CREATED) {
        Node *node = kallocate(sizeof(Node), MEMORY_TAG_VFS);
        node->type = NODE_DIRECTORY;
        node->path = path;
        //if the root node is null, set it to this node
        if (fs->root == NULL) {
            fs->root = node;
            vinfo("Set root node to: %s", path)
        }
        //TODO: lookup parent node
        node->data.directory.child_count = 0;
        //initialize the children array to the DEFAULT_NODE_CAPACITY
        node->data.directory.children = kallocate(sizeof(Node *) * NODE_CAPACITY, MEMORY_TAG_VFS);
        DIR *handle = opendir(path);
        if (handle == NULL) {
            verror("Could not open directory: %s", path);
            return NULL;
        }
        struct dirent *entry;
        while ((entry = readdir(handle)) != NULL) {
            //make sure we don't load the current directory or the parent directory
            if (strings_equal(entry->d_name, ".") || strings_equal(entry->d_name, "..")) {
                continue;
            }
            char *child_path = string_format("%s/%s", path, entry->d_name);
            Node *child = fs_sync_node(child_path, NODE_CREATED);
            if (child == NULL) {
                verror("Could not load child node: %s", child_path);
                continue;
            }
            node->data.directory.children[node->data.directory.child_count++] = child;
            //set child parent
            child->parent = node;
        }
        closedir(handle);
        //We need to add the node to the loaded nodes dictionary
        dict_insert(loaded_nodes, path, node);
        return node;
    } else if (action == NODE_MODIFIED) {
        //TODO implement this
        Node *node = dict_lookup(loaded_nodes, path);
        if (node == NULL) {
            verror("Could not find node for path: %s", path);
            return NULL;
        }
        DIR *handle = opendir(path);
        if (handle == NULL) {
            verror("Could not open directory: %s", path);
            return NULL;
        }
        kfree(node->data.directory.children, sizeof(Node *) * NODE_CAPACITY, MEMORY_TAG_VFS);
        node->data.directory.children = kallocate(sizeof(Node *) * NODE_CAPACITY, MEMORY_TAG_VFS);

        node->data.directory.child_count = 0;
        struct dirent *entry;
        while ((entry = readdir(handle)) != NULL) {
            //make sure we don't load the current directory or the parent directory
            if (strings_equal(entry->d_name, ".") || strings_equal(entry->d_name, "..")) {
                continue;
            }
            char *child_path = string_format("%s/%s", path, entry->d_name);
            Node *child;
            if (dict_lookup(loaded_nodes, child_path) != NULL) {
                child = fs_sync_node(child_path, NODE_MODIFIED);
                //the node is already loaded, we don't need to do anything
            } else
                child = fs_sync_node(child_path, NODE_CREATED);

            if (child == NULL) {
                verror("Could not load child node: %s", child_path);
                continue;
            }
            node->data.directory.children[node->data.directory.child_count++] = child;
            //set child parent
            child->parent = node;
        }
        closedir(handle);
        //We need to add the node to the loaded nodes dictionary
        dict_insert(loaded_nodes, path, node);
        return node;
    }

    return NULL;
}

Node *load_file_node(Path path, NodeAction action) {
    if (action == NODE_CREATED) {
        //TODO implement this
        FILE *handle = fopen(path, "r");
        if (handle == NULL) {
            verror("Could not open file: %s", path);
            return NULL;
        }

        Node *node = kallocate(sizeof(Node), MEMORY_TAG_VFS);
        node->type = NODE_FILE;
        node->path = path;
        node->parent = NULL;
        //store a total of 10MB of data
        char *data = kallocate(10 * 1024 * 1024, MEMORY_TAG_VFS);
        i32 index = 0;
        // Printing what is written in file
        // character by character using loop.
        char ch;

        while ((ch = fgetc(handle)) != EOF) {
            data[index++] = ch;
        }
        node->data.file.size = index;
        node->data.file.data = kallocate(index, MEMORY_TAG_VFS);
        kcopy_memory(node->data.file.data, data, index);
        //make sure it's null terminated
        node->data.file.data[index] = '\0';
        kfree(data, 10 * 1024 * 1024, MEMORY_TAG_VFS);
        dict_insert(loaded_nodes, path, node);
        fclose(handle);
        return node;
    } else if (action == NODE_MODIFIED) {
        //TODO implement this
        Node *node = dict_lookup(loaded_nodes, path);
        if (node == NULL) {
            verror("Could not find node for path: %s", path);
            return NULL;
        }
        FILE *handle = fopen(path, "r");
        if (handle == NULL) {
            verror("Could not open file: %s", path);
            return NULL;
        }
        //store a total of 10MB of data
        char *data = kallocate(10 * 1024 * 1024, MEMORY_TAG_VFS);
        i32 index = 0;
        // Printing what is written in file
        // character by character using loop.
        char ch;
        while ((ch = fgetc(handle)) != EOF) {
            data[index++] = ch;
        }
        node->data.file.size = index;
        node->data.file.data = kallocate(index, MEMORY_TAG_VFS);
        kcopy_memory(node->data.file.data, data, index);
        //make sure it's null terminated
        node->data.file.data[index] = '\0';
        kfree(data, 10 * 1024 * 1024, MEMORY_TAG_VFS);
        fclose(handle);
        return node;
    }
    return NULL;
}
char *fs_to_string() {
    return dict_to_string(loaded_nodes);
}

// Helper function for indentation
char *repeat_str(const char *str, int times) {
    if (times <= 0) return strdup("");

    size_t len = strlen(str);
    char *result = kallocate(len * times + 1, MEMORY_TAG_VFS);
    for (int i = 0; i < times; ++i) {
        strcpy(result + i * len, str);
    }
    return result;
}

char *fs_node_to_string_recursive(Node *node, int depth) {
    if (node == NULL) {
        return strdup(""); // Return an empty string for easy concatenation
    }

    char *result = NULL;
    char *indent = repeat_str("|  ", depth - 1); // Indentation for depth

    switch (node->type) {
        case NODE_DIRECTORY: {
            char *children_str = strdup(""); // Initialize to empty string

            for (u32 i = 0; i < node->data.directory.child_count; ++i) {
                Node *child = node->data.directory.children[i];
                if (child == NULL)
                    continue;

                char *child_str = fs_node_to_string_recursive(child, depth + 1);
                char *temp = children_str;
                children_str = string_format("%s%s", children_str, child_str);
                kfree(child_str, string_length(child_str) + 1, MEMORY_TAG_VFS);
                kfree(temp, string_length(temp) + 1, MEMORY_TAG_VFS);
            }

            result = string_format("%s|-- %s/\n%s", indent, node->path, children_str);
            kfree(children_str, string_length(children_str) + 1, MEMORY_TAG_STRING);
            break;
        }
        case NODE_FILE: {
            result = string_format("%s|-- %s\n", indent, node->path);
            break;
        }
        case NODE_SYMLINK: {
            // You can define the behavior for symlinks here if needed
            break;
        }
    }

    kfree(indent, string_length(indent) + 1, MEMORY_TAG_VFS);
    return result ? result : strdup(""); // Return result or an empty string if NULL
}

char *fs_node_to_string(Node *node) {
    return fs_node_to_string_recursive(node, 0);
}



Node *fs_sync_node(Path path, NodeAction action) {
    if (action != NODE_DELETED) {
        struct stat file_stat;
        if (stat(path, &file_stat) < 0) {
            verror("Could not get file stats for: %s", path);
            return NULL;
        }
        //Check if file or directory

        if (S_ISDIR(file_stat.st_mode)) {
            return load_directory_node(file_stat, path, action);
        } else if (S_ISREG(file_stat.st_mode)) {

            Node *node = load_file_node(path, action);
            if (node == NULL) {
                verror("Could not load file node: %s", path);
                return NULL;
            }
            //Get the extension of the file
            char *extension = strrchr(path, '.');
            if (extension == NULL) {
                verror("Could not get extension for file: %s", path);
                return NULL;
            }
            //remove the '.' from the extension
            extension++;
            //get the asset loader for the extension
            asset_loader *loader = dict_lookup(asset_loaders, extension);
            if (loader == NULL) {
                verror("Could not find asset loader for extension: %s", extension);
                vdebug("Available asset loaders: %s", dict_to_string(asset_loaders));
                return NULL;
            }
            //load the asset
            Asset *asset = kallocate(sizeof(Asset), MEMORY_TAG_VFS);
            if (action == NODE_CREATED)
                loader->load(node, asset);
            else if (action == NODE_MODIFIED){
                loader->unload(node);
                loader->load(node, asset);
            }
            return node;
        } else if (S_ISFIFO(file_stat.st_mode)) {
            //TODO implement symbolic links
        }
    } else {
        //We're only deleting the node in memory.
        Node *node = dict_lookup(loaded_nodes, path);
        if (node == NULL) {
            verror("Could not find node for path: %s", path);
            return NULL;
        }
        //Get the extension of the file
        char *extension = strrchr(path, '.');
        if (extension == NULL) {
            verror("Could not get extension for file: %s", path);
            return NULL;
        }
        //remove the '.' from the extension
        extension++;

        //get the asset loader for the extension
        asset_loader *loader = dict_lookup(asset_loaders, extension);
        if (loader == NULL) {
            verror("Could not find asset loader for extension: %s", extension);
            vdebug("Available asset loaders: %s", dict_to_string(asset_loaders));
            return NULL;
        }
        //unload the asset
        loader->unload(node);
        Node *parent = node->parent;
        if (parent == NULL) {
            verror("Cannot delete root node: %s", path);
            return NULL;
        }
        //remove the node from the loaded nodes dictionary
        dict_remove(loaded_nodes, path);

        fs_sync_node(parent->path, NODE_MODIFIED);


//
//        //destroy the node
//        fs_node_destroy(node);
        return NULL;
    }
    return NULL;
}

Node *fs_get_node(Path path) {
    return dict_lookup(loaded_nodes, path);
}

void fs_node_destroy(Node *node) {
    if (node->type == NODE_DIRECTORY) {
        vwarn("Destroying node directory at path: %s", node->path);
        //TODO implement this
        node->data.directory.child_count = 0;
        //recursively destroy all children
        for (u32 i = 0; i < node->data.directory.child_count; i++) {
            Node _node;
            fs_node_destroy(&_node);
        }
        kfree(node->data.directory.children, sizeof(Node *) * NODE_CAPACITY, MEMORY_TAG_VFS);
    } else {
        //TODO implement this
        kfree(node->data.file.data, node->data.file.size, MEMORY_TAG_VFS);
    }
    kfree(node->path, string_length(node->path) + 1, MEMORY_TAG_VFS);
    //destroy the node
    kfree(node, sizeof(Node), MEMORY_TAG_VFS);
}