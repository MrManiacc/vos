/**
 * Created by jraynor on 2/12/2024.
 */
#include "resource/loaders.h"
#include "platform/platform.h"
#include "core/vstring.h"


static resource_loader *loader = null;

static b8 is_png(char *ext) {
    return strings_equal(ext, "png");
}


//the loader for the image
static void *load_image(const char *path, u64 *size) {
    return null;
}

static void unload_image(resource *data) {
    platform_free(data->data, false);
    data->data = null;
}

/**
 * @brief The folder loader is a special loader that will load a binary file as a resource.
 */
resource_loader *image_loader() {
    if (loader == null) {
        loader = platform_allocate(sizeof(resource_loader), false);
        loader->name = "Image Loader";
        loader->loader_id = RESOURCE_TYPE_IMAGE;
        loader->is_for = is_png;
        loader->load = load_image;
        loader->unload = unload_image;
    }
    return loader;
}
