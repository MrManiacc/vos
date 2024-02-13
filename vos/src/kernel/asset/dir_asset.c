#include "lua_asset.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include <sys/stat.h>
#include <dirent.h>
#include "kernel/vfs/paths.h"
#include "core/vstring.h"
#include "asset_watcher.h"
#include "core/vlogger.h"

b8 directory_is_supported(AssetPath *path) {
    // check if the directory is a file or a directory.
    struct stat s;
    char *sys_path = path_to_platform((char *) path->path);
    if (stat(sys_path, &s) == 0) {
        if (s.st_mode & S_IFDIR) {
            return true;
        }
    }
    kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
    
    return false;
}

typedef struct DirectoryAssetData {
    AssetPath *path;
    AssetWatcher *watcher;
} DirectoryAssetData;

// Loads a directory from the disk. The asset data will be a darray of the files in the directory.
// This WILL load subdirectories recursively.
AssetData *directory_load(AssetPath *path) {
    vdebug("Loading directory: %s", path->path);
    //get the directory path
    char *sys_path = path_to_platform((char *) path->path);
    DirectoryAssetData *data = kallocate(sizeof(DirectoryAssetData), MEMORY_TAG_VFS);
    data->path = path;
    b8 *running = kallocate(sizeof(b8), MEMORY_TAG_VFS);
    *running = true;
    data->watcher = asset_watcher_initialize(string_duplicate(sys_path), running);
    AssetData *asset_data = kallocate(sizeof(AssetData), MEMORY_TAG_VFS);
    asset_data->data = data;
    asset_data->size = sizeof(DirectoryAssetData);
    
    //recursive load
    //list all the files in the directory
    //for each file, check if it is a directory or a file
    //if it is a directory, load it recursively
    //if it is a file, load it
    DIR *handle = opendir(sys_path);
    if (handle == NULL) {
        verror("Could not open directory: %s", path);
        kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
        return NULL;
    }
    struct dirent *entry;
    while ((entry = readdir(handle)) != NULL) {
        //make sure we don't load the current directory or the parent directory
        if (strings_equal(entry->d_name, ".") || strings_equal(entry->d_name, "..")) {
            continue;
        }
        AssetPath child = {};
        char *child_path = string_format("%s/%s", sys_path, entry->d_name);
        child.path = path_to_platform(child_path);
        kfree(child_path, string_length(child_path), MEMORY_TAG_VFS);
        asset_load(child);
        //TODO: add the child to the asset map
    }
    closedir(handle);
    kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
    
    return asset_data;
}

void directory_unload(AssetHandle *asset) {
    AssetPath *path = asset->path;
    vdebug("Unloading directory: %s", path->path);
//    if (!asset || !asset->data) return; // Safety check
//
//    DirectoryAssetData *data = (DirectoryAssetData *) asset->data->data;
////
////    // Ensure the watcher is properly shutdown and freed.
//    if (data->watcher) {
//        asset_watcher_shutdown(data->watcher); // Properly handle the watcher's cleanup.
//        kfree(data->watcher, sizeof(AssetWatcher), MEMORY_TAG_VFS);
//        data->watcher = NULL; // Avoid dangling pointer.
//    }
//
//    // Log the path being unloaded, if available.
//    if (data->path && data->path->path) {
//        vdebug("Unloading directory asset at path %s", data->path->path);
//    }
//
//    // Free the DirectoryAssetData itself.
    kfree(asset->data->data, sizeof(DirectoryAssetData), MEMORY_TAG_VFS);
    // Delete the handle
//    kfree(asset->data->data, asset->data->size, MEMORY_TAG_VFS);
    kfree(asset->data, sizeof(AssetData), MEMORY_TAG_VFS);
    asset->data = NULL;
    asset->state = ASSET_STATE_UNLOADED;
}

// The asset manager context.
AssetLoader *system_directory_asset_loader() {
    AssetLoader *loader = kallocate(sizeof(AssetLoader), MEMORY_TAG_VFS);
    loader->is_supported = directory_is_supported;
    loader->load = directory_load;
    loader->unload = directory_unload;
    loader->name = "Directory";
    return loader;
}
