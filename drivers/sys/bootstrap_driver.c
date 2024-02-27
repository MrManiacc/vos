/**
 * Created by jraynor on 2/25/2024.
 */
// ReSharper disable CppPointerConversionDropsQualifiers
#include <stdlib.h>
#include <string.h>

#include "containers/darray.h"
#include "core/vlogger.h"
#include "kern/kernel.h"
#include "core/vgui.h"
#include "platform/platform.h"

// Creates the driver entry point
$driver_entry("boot")


static void **renderers = null;

b8 driver_create(const Namespace *ns) {
    renderers = darray_create(Function*);
    kernel_namespace_define_query(ns, process, "tell_secret(string)string");
    kernel_namespace_define_query(ns, process, "register_render(string)bool");
    kernel_namespace_define_query(ns, process, "color(u32;u32;u32;u32)pointer");
    kernel_namespace_define_query(ns, process, "rect(pointer;f32;f32;f32;f32;pointer)void");
    // kernel_namespace_call(ns, "render", 0.0, null);
    kernel_event_listen(kernel, EVENT_KERNEL_RENDER, kernel_process_function_query(process, "render(pointer)bool"));
    return true;
}


VAPI b8 register_render(const char *query) {
    const Function *function = kernel_namespace_function_lookup(kernel, query);
    if (function == null) {
        verror("Failed to find function: %s", query);
        return false;
    }
    darray_push(Function*, renderers, function);
    vinfo("Registering render function, %s", query);
    return true;
}

VAPI b8 render(const EventData *data) {
    NVGcontext *ctx = (NVGcontext *) ((uintptr_t) data->f64[1]);
    for (u64 i = 0; i < darray_length(renderers); ++i) {
        const Function *allocation = *(Function **) darray_get(renderers, i);
        kernel_process_function_call(allocation, data->f64[0], ctx);
    }

    return false; // We didn't handle the event
}


VAPI char *tell_secret(const char *name) {
    vinfo("Telling secret to %s", name);
    return "The secret is 42";
}


b8 driver_destroy(const Namespace *ns) {
    vinfo("Destroying process");
    return true;
}
