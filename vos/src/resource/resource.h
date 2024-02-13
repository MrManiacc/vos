/**
 * Created by jraynor on 2/12/2024.
 */
#pragma once

#include "defines.h"

/** @brief Pre-defined resource types. */
typedef enum resource_type {
    /** @brief Folder resource type. */
    RESOURCE_TYPE_FOLDER,
    /** @brief Script resource type. */
    RESOURCE_TYPE_SCRIPT,
    /** @brief Binary resource type. */
    RESOURCE_TYPE_BINARY,
    /** @brief Image resource type. */
    RESOURCE_TYPE_IMAGE,
    
    /** @brief The maximum number of resource types. */
    RESOURCE_TYPE_MAX
} resource_type;


/**
 * @brief A generic structure for a resource. All resource loaders
 * load data into these.
 */
typedef struct resource {
    /** @brief The identifier of the loader which handles this resource. */
    resource_type loader_id;
    /** @brief The name of the resource. */
    const char *name;
    /** @brief The full file path of the resource. */
    char *full_path;
    /** @brief The size of the resource data in bytes. */
    u64 data_size;
    /** @brief The resource data. */
    void *data;
} resource;


/**
 * @brief A structure for a resource loader. All resource loaders
 * must implement these functions.
 */
typedef struct resource_loader {
    /** @brief The unique identifier of the loader. */
    resource_type loader_id;
    
    /** @brief The name of the loader. */
    const char *name;
    
    /** @brief The function to check if the loader is for the given resource type. */
    b8 (*is_for)(char *type);
    
    /** @brief The function to load a resource. */
    void *(*load)(const char *path, u64 *size);
    
    /** @brief The function to unload a resource. */
    void (*unload)(resource *data);
} resource_loader;


/**
 * @brief Initializes the resource manager.
 */
VAPI void resource_init(char *mount_point);

/**
 * @brief Registers a resource loader with the resource manager.
 *
 * @param loader The loader to register.
 * @return True on success; otherwise false.
 */
VAPI b8 resource_register_loader(resource_loader *loader);

/**
 * @brief Unregisters a resource loader from the resource manager.
 *
 * @param loader The loader to unregister.
 * @return True on success; otherwise false.
 */
VAPI b8 resource_unregister_loader(resource_loader *loader);

/**
 * @brief Loads a resource from the given path.
 *
 * @param path The path to the resource.
 * @return The loaded resource unless this operation fails, then 0.
 */
VAPI resource *resource_load(char *path);

/**
 * @brief Unloads the given resource.
 *
 * @param resource The resource to unload.
 * @return True on success; otherwise false.
 */
VAPI b8 resource_unload(resource *res);


/**
 * @brief Gets the resource with the given name.
 *
 * @param name The name of the resource.
 * @return The resource with the given name unless it does not exist, then 0.
 */
VAPI resource *resource_get(const char *name);


/**
 * @brief Destroys the resource manager. Automatically unloads all resources that are still loaded.
 */
VAPI void resource_destroy();

