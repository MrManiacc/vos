/**
 * Created by jraynor on 2/25/2024.
 */
#include <nanovg.h>
#include <string.h>

#include "containers/darray.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "core/vstring.h"
#include "filesystem/paths.h"
#include "kern/kernel.h"
#include "platform/platform.h"
// The kernel is a singleton, so we can define it here. It gets initialized by the kernel_create function.
static Kernel *_kernel;

VAPI void render(f64 delta, NVGcontext *vg) {
    //Draws and animated a circle
    nvgBeginPath(vg);
    nvgCircle(vg, 400, 300, 100);
    nvgFillColor(vg, nvgRGB(255, 0, 0));
    nvgFill(vg);
    nvgClosePath(vg);
}


VAPI FilePathList *boot_sys_exectuables() {
    const char *cwd = platform_get_current_working_directory();
    const FilePathList *list = platform_collect_files_recursive(cwd);
    //Filters the list to only include executable scripts
    FilePathList *filtered = kallocate(sizeof(FilePathList), MEMORY_TAG_DARRAY);
    filtered->paths = kallocate(sizeof(char *) * list->count, MEMORY_TAG_DARRAY);
    filtered->count = 0;
    for (u32 i = 0; i < list->count; i++) {
        const char *path = list->paths[i];
        if (string_ends_with(path, ".lua")) {
            filtered->paths[filtered->count++] = _strdup(path);
        }
    }
    return filtered;
}


kernel_define_driver();
