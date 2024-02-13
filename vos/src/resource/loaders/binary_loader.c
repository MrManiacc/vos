/**
 * Created by jraynor on 2/12/2024.
 */
#include "resource/loaders.h"

#include "platform/platform.h"
#include "core/vstring.h"


static resource_loader *loader = null;

static b8 is_supported_binary(char *ext) {
    return strings_equal(ext, "bin"); //TODO: also check magic number for binary files
}


static void *load_binary(const char *path, u64 *size) {
    return null; //TODO: implement binary loading based on the type of binary file
}

static void unload_binary(resource *data) {
    platform_free(data->data, false);
    data->data = null;
}

/**
 * @brief The folder loader is a special loader that will load a binary file as a resource.
 */
resource_loader *binary_loader() {
    if (loader == null) {
        loader = platform_allocate(sizeof(resource_loader), false);
        loader->name = "Binary Loader";
        loader->loader_id = RESOURCE_TYPE_IMAGE;
        loader->is_for = is_supported_binary;
        loader->load = load_binary;
        loader->unload = unload_binary;
    }
    return loader;
}
