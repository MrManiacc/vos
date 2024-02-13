#include "lua_asset.h"
#include "core/vmem.h"
#include "kernel/vfs/paths.h"
#include "core/vlogger.h"
#include "core/vstring.h"
#include <sys/stat.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

//#define SAVE_COMPILED_LUA 1

b8 lua_file_is_supported(AssetPath *path) {
    // check if the directory is a file or a directory.
    struct stat s;
    if (stat(path->path, &s) == 0) {
        if (s.st_mode & S_IFREG) {
            // check if the file is a lua file by checking the extension.
            if (string_ends_with(path->path, ".lua") != 0) {
                return true;
            }
        }
    }
    return false;
}


AssetData *lua_file_load(AssetPath *path) {
    char *sys_path = path_to_platform((char *) path->path);
//
//    if (is_binary_lua_file(sys_path)) {
    FILE *file = fopen(sys_path, "rb");
    if (file == NULL) {
        verror("Could not open file: %s", sys_path);
        kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size == 0) {
        fclose(file);
        verror("File is empty: %s", sys_path);
        kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
        return NULL;
    }
    char *data = kallocate(size, MEMORY_TAG_VFS);
    
    fread(data, size, 1, file);
    fclose(file);
    
    AssetData *asset_data = kallocate(sizeof(AssetData), MEMORY_TAG_VFS);
    asset_data->data = data;
    asset_data->size = size;
    
    kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
    return asset_data;
}

// Unloads a lua file from the disk. This will free the asset data.
// Unloads a lua file from the disk. This will free the asset data.
void lua_file_unload(AssetHandle *asset) {
    vdebug("Unloading lua file: %s", asset->path->path)
    if (asset->data == NULL) {
        return;
    }
    // Free the memory associated with the asset
    kfree(asset->data->data, asset->data->size, MEMORY_TAG_VFS);
    kfree(asset->data, sizeof(AssetData), MEMORY_TAG_VFS);
    asset->data = NULL;
    asset->state = ASSET_STATE_UNLOADED;
}

// This is a special loader that will load a lua file and execute it.
AssetLoader *system_lua_file_asset_loader() {
    AssetLoader *loader = kallocate(sizeof(AssetLoader), MEMORY_TAG_ASSET);
    loader->is_supported = lua_file_is_supported;
    loader->load = lua_file_load;
    loader->unload = lua_file_unload;
    loader->name = "Lua File";
    return loader;
}

