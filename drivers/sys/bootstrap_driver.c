/**
 * Created by jraynor on 2/25/2024.
 */
#include "core/vlogger.h"
#include "kern/kernel.h"

// Creates the driver entry point
$driver_entry$

VAPI b8 render(const KernProcEvent *event) {
    if (event->code != EVENT_KERNEL_RENDER) {
        return false;
    }
    vdebug("delta time: %f", event->data->f64[0]);
    return false;
}


b8 setup_process() {
    kernel_event_listen_function(kernel, EVENT_KERNEL_RENDER, kernel_process_function_lookup(process, (FunctionSignature){
        .name = "render",
        .arg_count = 1,
        .return_type = FUNCTION_TYPE_BOOL,
        .args[0] = FUNCTION_TYPE_POINTER,
    }));
    return true;
}


b8 destroy_process() {
    vinfo("Destroying process");
    return true;
}
