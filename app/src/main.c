#include <assert.h>
#include <kern/kernel.h>

#include "containers/darray.h"
#include "core/vgui.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "filesystem/paths.h"
#include "platform/platform.h"

static Function *driver_render = null;
static Function *script_render = null;

b8 render_listener(Kernel *kernel, u16 code, const EventData data) {
    if (code != EVENT_KERNEL_RENDER) {
        return false;
    }
    kernel_process_function_call(driver_render, data.f64[0], (NVGcontext *) ((uintptr_t) data.f64[1]));
    kernel_process_function_call(script_render, data.f64[0]);
    return true;
}

// Test the kernel
int main(int argc, char **argv) {
    Kernel *kernel = kernel_create(path_locate_root());
    vinfo("Kernel created: %p", kernel);
    WindowContext window; // Allocate the window on the stack
    if (window_initialize(&window, "Kernel Test", 800, 600) == false) {
        verror("Failed to initialize window");
        return 1;
    }
    EventData data;
    data.f32[0] = platform_get_absolute_time();
    kernel_event_listen(EVENT_KERNEL_RENDER, render_listener);

    Process *test_driver = kernel_process_new(kernel, "C:\\Users\\jwraynor\\Documents\\vos\\build\\debug\\drivers\\sys.dll");
    kernel_driver_initialize(test_driver);
    Process *test_script = kernel_process_new(kernel, "C:\\Users\\jwraynor\\Documents\\vos\\app\\assets\\test_script.lua");


    driver_render = kernel_process_function_lookup(test_driver, (FunctionSignature){
        .name = "render",
        .arg_count = 2,
        .return_type = FUNCTION_TYPE_VOID,
        .args[0] = FUNCTION_TYPE_F64,
        .args[1] = FUNCTION_TYPE_POINTER,
    });

    script_render = kernel_process_function_lookup(test_script, (FunctionSignature){
        .name = "render",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_VOID,
        .args[0] = FUNCTION_TYPE_F64,
    });

    const FunctionResult boot_list = kernel_process_function_call(kernel_process_function_lookup(test_driver, (FunctionSignature){
        .name = "boot_sys_exectuables",
        .arg_count = 0,
        .return_type = FUNCTION_TYPE_POINTER,
    }));

    const FilePathList *list = boot_list.data.pointer;
    for (u32 i = 0; i < list->count; i++) {
        vinfo("Found boot script: %s", list->paths[i]);
    }


    f64 now = platform_get_absolute_time();
    while (!window_should_close(&window)) {
        window_begin_frame(&window);
        const f64 time = platform_get_absolute_time();
        data.f64[0] = (time - now);
        // make the second double actually be a pointer to the nanovg context
        data.f64[1] = (double) ((uintptr_t) window.vg); // Store in your double array
        now = time;
        //render here
        kernel_event_trigger(EVENT_KERNEL_RENDER, data);
        window_end_frame(&window);
    }
    window_shutdown(&window);
    kernel_destroy(kernel);
    return 0;
}
