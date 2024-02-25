#include <assert.h>
#include <kern/kernel.h>

#include "core/vgui.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "filesystem/paths.h"
#include "platform/platform.h"

b8 render_listener(Kernel *kernel, u16 code, const EventData data) {
    f32 time = (f32) data.f32[0];
    // vinfo("Render event received: %.6f", time);

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
    Process *test_script = kernel_process_new("D:\\vos\\build\\debug\\app\\assets\\test.lua");

    const FunctionSignature fib = {
        .name = "fib",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_I32,
        .args[0] = FUNCTION_TYPE_I32
    };

    const FunctionSignature prime = {
        .name = "prime",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_BOOL,
        .args[0] = FUNCTION_TYPE_I32
    };

    const Function *driver_fib = kernel_process_function_lookup(test_driver, fib);
    if (driver_fib == null) {
        verror("Failed to find test driver function");
        return 1;
    }

    const Function *driver_prime = kernel_process_function_lookup(test_driver, prime);
    if (driver_prime == null) {
        verror("Failed to find test driver function");
        return 1;
    }

    const Function *script_fib = kernel_process_function_lookup(test_script, fib);
    if (script_fib == null) {
        verror("Failed to find test script function");
        return 1;
    }
    const Function *script_prime = kernel_process_function_lookup(test_script, prime);
    if (script_prime == null) {
        verror("Failed to find test script function");
        return 1;
    }

    u64 start = platform_get_absolute_time();
    const FunctionResult driver_result = kernel_process_function_call(driver_fib, 45);
    vinfo("Fibonacci sequence from driver: %d", driver_result.data.i32);
    vinfo("Driver time: %.6f", platform_get_absolute_time() - start);

    start = platform_get_absolute_time();
    const FunctionResult script_result = kernel_process_function_call(script_fib, 45);
    vinfo("Fibonacci sequence from script: %d", script_result.data.i32);
    vinfo("Script time: %.6f", platform_get_absolute_time() - start);

    start = platform_get_absolute_time();
    const FunctionResult driver_prime_result = kernel_process_function_call(driver_prime, 1000000);
    vinfo("Prime number from driver: %s", driver_prime_result.data.boolean ? "true" : "false");
    vinfo("Driver time: %.6f", platform_get_absolute_time() - start);

    start = platform_get_absolute_time();
    const FunctionResult script_prime_result = kernel_process_function_call(script_prime, 1000000);
    vinfo("Prime number from script: %s", script_prime_result.data.boolean ? "true" : "false");
    vinfo("Script time: %.6f", platform_get_absolute_time() - start);


    while (!window_should_close(&window)) {
        data.f32[0] = platform_get_absolute_time() - data.f32[0];
        window_begin_frame(&window);
        //render here
        kernel_event_trigger(EVENT_KERNEL_RENDER, data);
        window_end_frame(&window);
    }
    window_shutdown(&window);
    vinfo("Kernel created: %p", kernel);
    return 0;
}
