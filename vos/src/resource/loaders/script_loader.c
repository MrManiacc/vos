/**
 * Created by jraynor on 2/12/2024.
 */
#include "resource/loaders.h"

#include "platform/platform.h"
#include "core/vstring.h"




static b8 is_supported_script(char *ext) {
    return strings_equal(ext, "lua"); //TODO: check if valid lua code
}


static void *load_script(const char *path, u64 *size) {
    return null; //TODO: implement script loading based on the type of script file
}

static void unload_script(resource *data) {
    platform_free(data->data, false);
    data->data = null;
}

/**
 * @brief The folder loader is a special loader that will load a script file as a resource.
 */
resource_loader *script_loader() {
    static resource_loader *script_loader = null;
    if (script_loader == null) {
        script_loader = platform_allocate(sizeof(resource_loader), false);
        script_loader->name = "Script Loader";
        script_loader->loader_id = RESOURCE_TYPE_SCRIPT;
        script_loader->is_for = is_supported_script;
        script_loader->load = load_script;
        script_loader->unload = unload_script;
    }
    return script_loader;
}
