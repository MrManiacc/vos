#include "lua_asset.h"
#include "core/mem.h"
#include "core/logger.h"
#include <sys/stat.h>
#include <dirent.h>
#include "kernel/vfs/paths.h"
#include "core/str.h"
#include "asset_watcher.h"

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
    }
    closedir(handle);
    kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
}
// Unloads a directory from the disk. This will free the asset data.
void directory_unload(AssetHandle *asset) {
    DirectoryAssetData *data = (DirectoryAssetData *) asset->data;
    asset_watcher_shutdown(data->watcher);
    kfree(data->path, sizeof(AssetPath), MEMORY_TAG_VFS);
    kfree(data, sizeof(DirectoryAssetData), MEMORY_TAG_VFS);
    //TODO probably some memory leaks here
}

// The asset manager context.
AssetLoader *system_directory_asset_loader() {
    AssetLoader *loader = kallocate(sizeof(AssetLoader), MEMORY_TAG_ASSET);
    loader->is_supported = directory_is_supported;
    loader->load = directory_load;
    loader->unload = directory_unload;
    loader->name = "Directory";
    return loader;
}
