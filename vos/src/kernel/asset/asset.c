#include "asset.h"
#include "containers/Map.h"
#include "core/mem.h"
#include "core/logger.h"
#include "containers/darray.h"
#include "kernel/vfs/paths.h"
#include "lua_asset.h"
#include "dir_asset.h"
#include "core/str.h"

#define MAX_ASSET_LOADERS 142

typedef struct AssetContext {
    // The asset map.
    Map *asset_map;
    // The asset loaders.
    AssetLoader *asset_loaders[MAX_ASSET_LOADERS];
    //the number of asset loaders.
    u32 asset_loader_count;
} AssetContext;

static AssetContext *asset_context;

/**
 * Initializes the asset manager. This will allocate the asset manager context and initialize the asset map.
 * @return TRUE if the asset manager was successfully initialized, else FALSE.
 */
b8 asset_manager_initialize(char *root_path) {
    if (asset_context != null) {
        verror("Asset manager already initialized.");
        return false;
    }
    path_move(root_path);
    asset_context = kallocate(sizeof(AssetContext), MEMORY_TAG_ASSET);
    asset_context->asset_map = dict_create_default();
    asset_loader_register(system_directory_asset_loader());
    asset_loader_register(system_lua_file_asset_loader());
    vdebug("Asset manager initialized.");
    return true;
}

/**
 * Syncs the asset manager. This will simply load the root directory as an asset which will load all the assets in the
 * directory. This will also load all the subdirectories.
 * @return TRUE if the asset manager was successfully synced, else FALSE.
 */
b8 asset_manager_sync() {
    AssetPath root_path = {};
    root_path.path = path_root_directory();
    root_path.extension = "";
    vdebug("Syncing asset manager with root path %s", root_path.path)
    return null != asset_load(root_path);
}

/**
 * Registers an asset loader with the asset manager.
 * @param loader The asset loader to register.
 * @return TRUE if the asset loader was successfully registered, else FALSE.
 */
b8 asset_loader_register(AssetLoader *loader) {
    if (asset_context->asset_loader_count >= MAX_ASSET_LOADERS) {
        verror("Asset loader count exceeded maximum asset loader count.");
        return false;
    }
    asset_context->asset_loaders[asset_context->asset_loader_count++] = loader;
    vdebug("%s asset loader registered.", loader->name);
    return true;
}

/**
 * Loads an asset from the disk. This will use the asset loaders to load the asset.
 * @param path The path to the asset.
 * @return The asset if the asset was successfully loaded, else NULL.
 */
AssetHandle *asset_load(AssetPath path) {
    // Check if the asset is supported.
    for (u32 i = 0; i < asset_context->asset_loader_count; ++i) {
        AssetLoader *loader = asset_context->asset_loaders[i];
        if (loader->is_supported(&path)) {
            AssetHandle *asset = kallocate(sizeof(AssetHandle), MEMORY_TAG_ASSET);
            asset->path = kallocate(sizeof(AssetPath), MEMORY_TAG_ASSET);
            kcopy_memory(asset->path, &path, sizeof(AssetPath));
            asset->state = ASSET_STATE_LOADING;
            asset->data = loader->load(&path);
            if (asset->data == null) {
                vwarn("Asset [%s] failed to load at path %s", loader->name, path.path)
                return null;
            }
            asset->state = ASSET_STATE_LOADED;
            char *sys_path = path_to_platform((char *) path.path);
            dict_set(asset_context->asset_map, sys_path, asset);
            vdebug("Asset [%s] loaded at path %s", loader->name, sys_path)
            kfree(sys_path, string_length(sys_path), MEMORY_TAG_VFS);
            return asset;
        }
    }
    vwarn("Asset [%] not supported", path_file_name((char *) path.path));
    return null;
}

/**
 * Reloads an asset from the disk. This will use the asset loaders to reload the asset.
 * @param path The path to the asset.
 * @return The asset if the asset was successfully reloaded, else NULL.
 */
AssetHandle *asset_reload(AssetPath path) {
    // Check if the asset is already loaded.
    AssetHandle *asset = dict_get(asset_context->asset_map, path.path);
    if (asset == null) {
        return null;
    }
    // Check if the asset is supported.
    for (u32 i = 0; i < asset_context->asset_loader_count; ++i) {
        AssetLoader *loader = asset_context->asset_loaders[i];
        if (loader->is_supported(&path)) {
            asset->state = ASSET_STATE_UNLOADING;
            loader->unload(asset);
            asset->state = ASSET_STATE_LOADING;
            asset->data = loader->load(&path);
            
            if (asset->data == null) {
                vwarn("Asset [%s] failed to load at path %s", loader->name, path.path)
                return null;
            }
            asset->state = ASSET_STATE_LOADED;
            vdebug("Asset [%s] reloaded at path %s", loader->name, path.path)
            return asset;
        }
    }
    vwarn("Asset [%] not supported", path_file_name((char *) path.path));
    return null;
}

/**
 * Unloads an asset. This will use the asset loaders to unload the asset.
 * @param path The path to the asset.
 * @return TRUE if the asset was successfully unloaded, else FALSE.
 */
b8 asset_unload(AssetPath path) {
    // Check if the asset is already loaded.
    AssetHandle *asset = dict_get(asset_context->asset_map, path.path);
    if (asset == null) {
        return false;
    }
    // Unload the asset using the appropriate loader.
    for (u32 i = 0; i < asset_context->asset_loader_count; i++) {
        AssetLoader *loader = asset_context->asset_loaders[i];
        if (loader->is_supported(&path)) {
            vdebug("Asset [%s] unloaded at path %s", path_file_name((char *) path.path), path.path);
            asset->state = ASSET_STATE_UNLOADING;
            if (loader->unload) {
                loader->unload(asset); // Assuming unload function takes care of asset data.
            }
            
            // Remove the asset from the asset map
            dict_remove(asset_context->asset_map, path.path);
            kfree(asset, sizeof(AssetHandle), MEMORY_TAG_ASSET);
            return true;
        }
    }
    vwarn("Asset [%s] not supported", path_file_name((char *) path.path));
    return false;
}

/**
 * Destroys the asset manager. This will deallocate the asset manager context and destroy the asset map.
 * @return TRUE if the asset manager was successfully destroyed, else FALSE.
 */
b8 asset_manager_shutdown() {
    if (asset_context == null) {
        verror("Asset manager already destroyed.");
        return false;
    }
    // Iterate over the asset map and unload all the assets.
    idict it = dict_iterator(asset_context->asset_map);
    while (dict_next(&it)) {
        AssetPath path;
        path.path = it.entry->key;
        if (!asset_unload(path)) {
            vwarn("Failed to unload asset at path %s", path.path);
        }
    }
    dict_destroy(asset_context->asset_map);
    for (u32 i = 0; i < asset_context->asset_loader_count; ++i) {
        kfree(asset_context->asset_loaders[i], sizeof(AssetLoader), MEMORY_TAG_ASSET);
    }
    
    
    kfree(asset_context, sizeof(AssetContext), MEMORY_TAG_ASSET);
    vdebug("Asset manager destroyed.");
    return true;
}

/**
 * Gets an asset from the asset map. This will return null if the asset is not found.
 * @param path The path to the asset.
 * @return The asset if it was found, else NULL.
 */
AssetHandle *asset_get(AssetPath path) {
    return dict_get(asset_context->asset_map, path.path);
}