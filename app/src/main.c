#include <assert.h>
#include <kern/kernel.h>

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
    return true;
}

// Test the kernel
int main(int argc, char **argv) {
    Kernel *kernel = kernel_create(path_locate_root());
    WindowContext window; // Allocate the window on the stack
    if (window_initialize(&window, "Kernel Test", 800, 600) == false) {
        verror("Failed to initialize window");
        return 1;
    }
    EventData data;
    data.f32[0] = platform_get_absolute_time();
    kernel_event_listen(EVENT_KERNEL_RENDER, render_listener);

    Process *test_driver = kernel_process_new("D:\\vos\\build\\debug\\drivers\\sys.dll");
    Process *test_script = kernel_process_new("D:\\vos\\build\\debug\\app\\assets\\test_script.lua");
    const Function *initializer = kernel_process_function_lookup(test_driver, (FunctionSignature){
        .name = "kernel_initialize",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_VOID,
        .args[0] = FUNCTION_TYPE_POINTER
    });
    if (initializer == null) {
        verror("Failed to find initializer function");
        return 1;
    }

    kernel_process_function_call(initializer, kernel);


    driver_render = kernel_process_function_lookup(test_driver, (FunctionSignature){
        .name = "render",
        .arg_count = 2,
        .return_type = FUNCTION_TYPE_VOID,
        .args[0] = FUNCTION_TYPE_F64,

    });

    script_render = kernel_process_function_lookup(test_script, (FunctionSignature){
        .name = "render",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_VOID,
        .args[0] = FUNCTION_TYPE_F64
    });

    const FunctionSignature fib = {
        .name = "fib",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_I32,
        .args[0] = FUNCTION_TYPE_I32
    };

    const FunctionSignature fib_from_script = {
        .name = "fib_from_script",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_I32,
        .args[0] = FUNCTION_TYPE_I32
    };

    const Function *driver_fib = kernel_process_function_lookup(test_driver, fib);
    if (driver_fib == null) {
        verror("Failed to find test driver function");
        return 1;
    }

    const Function *script_fib = kernel_process_function_lookup(test_script, fib);
    if (script_fib == null) {
        verror("Failed to find test script function");
        return 1;
    }

    const Function *driver_fib_from_script = kernel_process_function_lookup(test_driver, fib_from_script);
    if (driver_fib_from_script == null) {
        verror("Failed to find test driver function");
        return 1;
    }

    u64 start = platform_get_absolute_time();
    const FunctionResult driver_result = kernel_process_function_call(driver_fib, 11);
    vinfo("Fibonacci sequence from driver: %d", driver_result.data.i32);
    vinfo("Driver time: %.6f", platform_get_absolute_time() - start);

    start = platform_get_absolute_time();
    const FunctionResult script_result = kernel_process_function_call(script_fib, 11);
    vinfo("Fibonacci sequence from script: %d", script_result.data.i32);
    vinfo("Script time: %.6f", platform_get_absolute_time() - start);

    start = platform_get_absolute_time();
    const FunctionResult driver_script_result = kernel_process_function_call(driver_fib_from_script, 11);
    vinfo("Fibonacci sequence from script: %d", driver_script_result.data.i32);
    vinfo("Driver script time: %.6f", platform_get_absolute_time() - start);

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
    vinfo("Kernel created: %p", kernel);
    return 0;
}
