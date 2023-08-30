#include "lua_asset.h"
#include "core/mem.h"
#include <sys/stat.h>

b8 lua_file_is_supported(AssetPath* path) {
    // check if the directory is a file or a directory.
    struct stat s;
    if (stat(path->path, &s) == 0) {
        if (s.st_mode & S_IFREG) {
            // check if the file is a lua file by checking the extension.
            if (strcmp(path->extension, "lua") != 0) {
                return true;
            }
            return true;
        }
    }
    return false;
}

// Loads a lua file from the disk. The asset data will be a darray of the files in the directory.
// This should send events here about introspection onto the assetdata to transform it before it is loaded.
AssetData *lua_file_load(AssetPath* path) {

}

// Unloads a lua file from the disk. This will free the asset data.
void lua_file_unload(AssetHandle *asset) {

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

