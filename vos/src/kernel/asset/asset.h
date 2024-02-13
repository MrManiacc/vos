/**
 * Created by jraynor on 8/29/2023.
 */
#pragma once

#include "defines.h"

// The asset path is the path to the asset on the disk.
typedef struct AssetPath {
    // The path of the asset.
    const char *path;
    // The extension of the asset.
    const char *extension;
    
} AssetPath;

// The asset data is the raw data that is loaded from the disk.
typedef struct AssetData {
    // The size of the data.
    u32 size;
    // The data.
    void *data;
} AssetData;

typedef enum AssetState {
    // The asset is not loaded.
    ASSET_STATE_UNLOADED,
    // The asset is loaded.
    ASSET_STATE_LOADED,
    // The asset is being loaded.
    ASSET_STATE_LOADING,
    // The asset is being unloaded.
    ASSET_STATE_UNLOADING,
} AssetState;

typedef struct AssetHandle {
    // The data of the asset. Can be null if the asset is not loaded. Will be reloaded if the asset is dirty.
    AssetData *data;
    // The path of the asset. Can be used as a key for the asset map.
    AssetPath *path;
    // The state of the asset.
    AssetState state;
} AssetHandle;

// The asset loader is used to load and unload assets.
typedef struct AssetLoader {
    // The function pointer to load the asset.
    b8 (*is_supported)(AssetPath *path);
    
    // The function pointer to load the asset.
    AssetData *(*load)(AssetPath *path);
    
    // The function pointer to unload the asset. Should only delete the data and update the state.
    // Can also do any cleanup needed for the asset.
    void (*unload)(AssetHandle *asset);
    
    // The name of the asset loader. Used for debugging.
    const char *name;
} AssetLoader;

/**
 * Initializes the asset manager. This will allocate the asset manager context and initialize the asset map.
 * @return TRUE if the asset manager was successfully initialized, else FALSE.
 */
b8 asset_manager_initialize(char *root_path);

/**
 * Syncs the asset manager. This will simply load the root directory as an asset which will load all the assets in the
 * directory. This will also load all the subdirectories.
 * @return TRUE if the asset manager was successfully synced, else FALSE.
 */
b8 asset_manager_sync();

/**
 * Registers a new asset loader. This will add the loader to the list of loaders that will be used to load assets.
 * @param loader
 * @return
 */
b8 asset_loader_register(AssetLoader *loader);

/**
 * Loads an asset from the disk. This will use the asset loaders to load the asset.
 * @param path The path to the asset.
 * @return The asset if it was successfully loaded, else NULL.
 */
AssetHandle *asset_load(AssetPath path);

/**
 * Reloads an asset from the disk. This will use the asset loaders to reload the asset.
 * @param path The path to the asset.
 * @return The asset if the asset was successfully reloaded, else NULL.
 */
AssetHandle *asset_reload(AssetPath path);

/**
 * Gets an asset from the asset map. This will return null if the asset is not found.
 * @param path The path to the asset.
 * @return The asset if it was found, else NULL.
 */
AssetHandle *asset_get(AssetPath path);

/**
 * Unloads an asset. This will use the asset loaders to unload the asset.
 * @param path The path to the asset.
 * @return TRUE if the asset was successfully unloaded, else FALSE.
 */
b8 asset_unload(AssetHandle *handle);

/**
 * Destroys the asset manager. This will deallocate the asset manager context and destroy the asset map.
 * @return TRUE if the asset manager was successfully destroyed, else FALSE.
 */
b8 asset_manager_shutdown();