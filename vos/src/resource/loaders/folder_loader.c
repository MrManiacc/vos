/**
 * Created by jraynor on 2/12/2024.
 */
#include "resource/loaders.h"

#include "platform/platform.h"
#include "core/vstring.h"


static b8 is_supported_folder(char *ext) {
    return strings_equal(ext, "dir");
}


static void *load_folder(const char *path, u64 *size) {
    return null; //TODO: implement binary folder structure
}

static void unload_folder(resource *data) {
    platform_free(data->data, false);
    data->data = null;
}

/**
 * @brief The folder loader is a special loader that will load a folder file as a resource.
 */
resource_loader *folder_loader() {
    static resource_loader *loader = null;
    if (loader == null) {
        loader = platform_allocate(sizeof(resource_loader), false);
        loader->name = "Folder Loader";
        loader->loader_id = RESOURCE_TYPE_FOLDER;
        loader->is_for = is_supported_folder;
        loader->load = load_folder;
        loader->unload = unload_folder;
    }
    return loader;
}
