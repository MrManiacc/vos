#include "lua_asset.h"
#include "core/mem.h"
#include "kernel/vfs/paths.h"
#include "core/logger.h"
#include "core/str.h"
#include <sys/stat.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

//#define SAVE_COMPILED_LUA 1

b8 lua_file_is_supported(AssetPath *path) {
    // check if the directory is a file or a directory.
    struct stat s;
    vdebug("Checking if %s is a file", path->path)
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

//A compile representation of a lua file.
struct LuaSource {
    // The compiled lua source code, if the read file is not a binary, we will compile it.
    char *compiled_program;
    // The size of the compiled program.
    AssetPath *path;
};

// Check to see if a given path is a binary lua file or a text lua file.
b8 is_binary_lua_file(char *path) {
    // Lua bytecode signature
    struct stat s;
    if (stat(path, &s) == 0) {
        if (s.st_mode & S_IFREG) {
            FILE *file = fopen(path, "rb");
            if (file) {
                char buffer[4];
                size_t read_size = fread(buffer, 1, 4, file);
                fclose(file);
                if (read_size == 4 && memcmp(buffer, LUA_SIGNATURE, 4) == 0) {
                    return true;  // It's a binary lua file
                }
            }
        }
    }
    return false; // It's a text lua file or an error occurred
}

// Loads a lua file from the disk. The asset data will be a darray of the files in the directory.
// This should send events here about introspection onto the asset data to transform it before it is loaded.
// Callback for lua_dump. Appends the dumped bytecode to a buffer.
static int dump_writer(lua_State *L, const void *p, size_t sz, void *ud) {
    luaL_Buffer *buff = (luaL_Buffer *) ud;
    luaL_addlstring(buff, (const char *) p, sz);
    return 0;
}

AssetData *lua_file_load(AssetPath *path) {
    char *sys_path = path_to_platform((char *) path->path);
    
    if (is_binary_lua_file(sys_path)) {
        FILE *file = fopen(sys_path, "rb");
        if (file == NULL) {
            verror("Could not open file: %s", sys_path);
            kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
            return NULL;
        }
        
        fseek(file, 0, SEEK_END);
        u64 size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char *data = kallocate(size, MEMORY_TAG_VFS);
        fread(data, size, 1, file);
        fclose(file);
        
        AssetData *asset_data = kallocate(sizeof(AssetData), MEMORY_TAG_VFS);
        asset_data->data = data;
        asset_data->size = size;
        
        return asset_data;
    } else {
        // For Lua source code
        lua_State *L = luaL_newstate();
        
        if (luaL_loadfile(L, sys_path) != 0) {
            verror("Failed to compile Lua source: %s", luaL_checkstring(L, -1));
            lua_close(L);
            kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
            return NULL;
        }
        
        luaL_Buffer bytecode_buff;
        luaL_buffinit(L, &bytecode_buff);
        lua_dump(L, dump_writer, &bytecode_buff, 0);
        
        // Extract the buffer to our asset data structure
        char *bytecode;
        size_t bytecode_size = luaL_bufflen(&bytecode_buff);
        bytecode = kallocate(bytecode_size, MEMORY_TAG_VFS);
        memcpy(bytecode, luaL_buffaddr(&bytecode_buff), bytecode_size);
        
        AssetData *asset_data = kallocate(sizeof(AssetData), MEMORY_TAG_VFS);
        asset_data->data = bytecode;
        asset_data->size = bytecode_size;
        
        lua_close(L);
        vinfo("compiled lua file: %s", path->path);
        return asset_data;
    }
}

// Unloads a lua file from the disk. This will free the asset data.
// Unloads a lua file from the disk. This will free the asset data.
void lua_file_unload(AssetHandle *asset) {
    // If we have the SAVE_COMPILED_LUA directive and the asset was a Lua source file
#ifdef SAVE_COMPILED_LUA
    char *path = (char*) asset->path->path;
    vdebug("Unloading lua file: %s", path)
    if (!is_binary_lua_file(path)) {

        // Construct the new path with the .lb extension
        size_t path_length = strlen(path);
        char *lb_path = kallocate(path_length + 3, MEMORY_TAG_VFS); // +3 for replacing ".lua" with ".lb"
        strncpy(lb_path, path, path_length - 4); // Copy all characters except ".lua"
        strcat(lb_path, ".lb");

        // Save the compiled bytecode to the new path
        FILE *file = fopen(lb_path, "wb");
        if (file) {
            fwrite(asset->data->data, 1, asset->data->size, file);
            fclose(file);
        } else {
            verror("Failed to write compiled bytecode to: %s", lb_path);
        }

        kfree(lb_path, path_length + 3, MEMORY_TAG_VFS);
    }
    kfree(path, string_length(path), MEMORY_TAG_VFS);
#endif
    
//    vdebug("Asset [%s] unloaded at path %s", path_file_name((char *) asset->path->path), asset->path->path);
    
    
//    vdebug("Unloaded lua file: %s", asset->path->path);
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

