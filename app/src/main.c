#include <assert.h>
#include <kern/kernel.h>

#include "containers/darray.h"
#include "core/vgui.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "filesystem/paths.h"
#include "kern/vfs.h"
#include "platform/platform.h"

static Function *driver_render = null;
static Function *script_render = null;

// b8 render_listener() {
//     if (event->code != EVENT_KERNEL_RENDER) {
//         return false;
//     }
//     // vdebug("delta time: %f", event->data->f64[0]);
//     // Call the driver render function                     /*A nasty hack to convert the second double into a pointer of a nvgcontext*/
//     // kernel_process_function_call(driver_render, data.f64[0], (NVGcontext *) ((uintptr_t) data.f64[1]));
//     return false;
// }

// Test the kernel
int main(int argc, char **argv) {
    const Vfs *vfs = vfs_init("root", path_locate_root());
    vfs_read(vfs->root); //This forces all discovered nodes to be loaded into memory.
    Kernel *kernel = kernel_create(path_locate_root());
    vinfo("Kernel created: %p", kernel);
    WindowContext window; // Allocate the window on the stack
    if (window_initialize(&window, "Kernel Test", 800, 600) == false) {
        verror("Failed to initialize window");
        return 1;
    }
    void *luas = vfs_collect(vfs, ".lua");
    if (luas != null) {
        darray_for_each(VfsHandle, luas, handle) {
            vinfo("Found lua file: %s", handle->path);
        }
    }

    // Process *test_driver = kernel_process_load(kernel, "D:\\vos\\build\\debug\\drivers\\sys.dll");
    // kernel_process_run(kernel, test_driver);

    // Process *test_script = kernel_process_load(kernel, "D:\\vos\\app\\assets\\test_script.lua");
    // kernel_process_run(kernel, test_script);

    //
    // driver_render = kernel_process_function_lookup(test_driver, (FunctionSignature){
    //     .name = "render",
    //     .arg_count = 2,
    //     .return_type = FUNCTION_TYPE_VOID,
    //     .args[0] = FUNCTION_TYPE_F64,
    //     .args[1] = FUNCTION_TYPE_POINTER,
    // });
    //
    // script_render = kernel_process_function_lookup(test_script, (FunctionSignature){
    //     .name = "render",
    //     .arg_count = 1,
    //     .return_type = FUNCTION_TYPE_VOID,
    //     .args[0] = FUNCTION_TYPE_F64,
    // });
    //
    // const FunctionResult boot_list = kernel_process_function_call(kernel_process_function_lookup(test_driver, (FunctionSignature){
    //     .name = "boot_sys_exectuables",
    //     .arg_count = 0,
    //     .return_type = FUNCTION_TYPE_POINTER,
    // }));


    // assert(boot_list.type == FUNCTION_TYPE_POINTER);

    // const FilePathList *list = boot_list.data.pointer;
    // for (u32 i = 0; i < list->count; i++) {
    // vinfo("Found boot script: %s", list->paths[i]);
    // }
    // kernel_event_listen(null, EVENT_KERNEL_RENDER, render_listener);


    // kernel_event_trigger(kernel, &(KernProcEvent){
    //     .code = EVENT_KERNEL_INIT,
    //     .data = &(EventData){
    //         .pointers = {
    //             .one = test_driver
    //         },
    //     },
    //     .sender.kernel = kernel,
    // });
    EventData data;
    data.f32[0] = platform_get_absolute_time();
    // make the second double actually be a pointer to the nanovg context. We only really need to do this once.
    data.f64[1] = (double) ((uintptr_t) window.vg); // Store in your double array
    f64 now = platform_get_absolute_time();
    while (!window_should_close(&window)) {
        window_begin_frame(&window);
        const f64 time = platform_get_absolute_time();
        // Create the event delta time and trigger the render event
        data.f64[0] = (time - now);
        now = time;
        kernel_event_trigger(kernel, EVENT_KERNEL_RENDER, &data);
        //render here
        // kernel_event_trigger(kernel, &event);
        window_end_frame(&window);
    }
    window_shutdown(&window);
    kernel_destroy(kernel);
    return 0;
}
