/**
 * Created by jraynor on 2/12/2024.
 */
#include "resource.h"
#include "core/vlogger.h"
#include "containers/dict.h"
#include "platform/platform.h"
#include "kernel/vfs/paths.h"

// Define a static struct for storing resource loaders
static struct {
    // An array of resource loaders
    resource_loader *loaders[RESOURCE_TYPE_MAX];
    // A dictionary for storing loaded resources
    dict *loaded_resources;
    // The mount point for the resources
    char *mount_point;
} resource_loaders;

// Initialize the resource manager and the resource loaders
void resource_init(char *mount_point) {
    //Initialize all of our resource loaders here.
    vinfo("Resource manager initialized.");
    resource_loaders.loaded_resources = dict_create_default();
    resource_loaders.mount_point = mount_point;
    //zero out the loaders
    platform_zero_memory(resource_loaders.loaders, sizeof(resource_loader *) * RESOURCE_TYPE_MAX);
}

b8 resource_register_loader(resource_loader *loader) {
    if (loader == null) {
        vwarn("Resource loader is null.");
        return false;
    }
    if (loader->loader_id >= RESOURCE_TYPE_MAX) {
        vwarn("Resource loader id %d is out of range.", loader->loader_id);
        return false;
    }
    resource_loaders.loaders[loader->loader_id] = loader;
    vdebug("Registered resource loader %s.", loader->name);
    return true;
}


resource *resource_get(const char *name) {
    return dict_get(resource_loaders.loaded_resources, name);
}

resource_loader *get_resource_loader(char *path) {
    //Get the file extension from the path
    char *extension = path_file_extension(path);
    //Check if the extension is null
    if (extension == null) {
        vwarn("Extension is null, assuming directory.");
        return resource_loaders.loaders[RESOURCE_TYPE_FOLDER];
    }
    //Get the loader for the resource
    for (u32 i = 0; i < RESOURCE_TYPE_MAX; i++) {
        resource_loader *loader = resource_loaders.loaders[i];
        if (loader == null) continue;
        if (loader->is_for(extension)) return loader;
    }
    // No loader found
    verror("No resource loader found for extension %s.", extension);
    return null;
}

resource *resource_load(char *path) {
    //Check if the path is null
    if (path == null) {
        vwarn("Path is null.");
        return null;
    }
    //Get the file name from the path
    char *name = path_file_name(path);
    //Check if the name is null
    if (name == null) {
        vwarn("Name is null.");
        return null;
    }
    //Check if the resource is already loaded
    resource *res = resource_get(name);
    if (res != null) {
        vwarn("Resource %s is already loaded.", name);
        return res;
    }
    resource_loader *loader = get_resource_loader(path);
    
    //Check if the loader is null
    if (loader == null) {
        vwarn("Resource loader is null.");
        return null;
    }
    
    //Load the resource
    u64 size;
    void *data = loader->load(path, &size);
    //Check if the data is null
    if (data == null) {
        vwarn("Resource data is null.");
        return null;
    }
    
    //Allocate memory for the resource
    res = platform_allocate(sizeof(resource), false);
    //Zero out the resource
    platform_zero_memory(res, sizeof(resource));
    //Set the resource data
    res->data = data;
    //Set the resource data size
    res->data_size = size;
    //Set the resource name
    res->name = name;
    //Set the resource full path
    res->full_path = path;
    //Set the resource loader id
    res->loader_id = loader->loader_id;
    //Add the resource to the loaded resources dictionary
    dict_set(resource_loaders.loaded_resources, name, res);
    return res;
}

b8 resource_unregister_loader(resource_loader *loader) {
    if (loader == null) {
        vwarn("Resource loader is null.");
        return false;
    }
    if (loader->loader_id >= RESOURCE_TYPE_MAX) {
        vwarn("Resource loader id %d is out of range.", loader->loader_id);
        return false;
    }
    resource_loaders.loaders[loader->loader_id] = null;
    vdebug("Unregistered resource loader %s.", loader->name);
    return true;
}

b8 resource_unload(resource *res) {
    //Check if the resource is null
    if (res == null) {
        vwarn("Resource is null.");
        return false;
    }
    //Check if the resource data is null
    if (res->data == null) {
        vwarn("Resource data is null.");
        return false;
    }
    //Get the loader for the resource
    resource_loader *loader = resource_loaders.loaders[res->loader_id];
    //Check if the loader is null
    if (loader == null) {
        vwarn("Resource loader is null.");
        return false;
    }
    //Call the unload function for the resource
    loader->unload(res);
    //Zero out the resource data
    platform_zero_memory(res->data, res->data_size);
    //Zero out the resource
    platform_zero_memory(res, sizeof(resource));
    return true;
}

//Unload all resources
void resource_unload_all() {
    idict iter = dict_iterator(resource_loaders.loaded_resources);
    while (dict_next(&iter)) {
        resource *res = iter.entry->object;
        resource_unload(res);
    }
}

void resource_destroy() {
    //Unload all resources
    resource_unload_all();
    //Destroy the dictionary
    dict_destroy(resource_loaders.loaded_resources);
    //Zero out the loaders
    platform_zero_memory(resource_loaders.loaders, sizeof(resource_loader *) * RESOURCE_TYPE_MAX);
    vinfo("Resource manager destroyed.");
}

