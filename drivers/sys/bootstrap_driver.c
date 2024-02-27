/**
 * Created by jraynor on 2/25/2024.
 */
#include <stdlib.h>
#include <string.h>

#include "core/vlogger.h"
#include "kern/kernel.h"
#include "core/vgui.h"

// Creates the driver entry point
$driver_entry("boot")


b8 driver_create(const Namespace *ns) {
    kernel_namespace_define_query(ns, process, "render(f64;pointer)void");
    kernel_namespace_define_query(ns, process, "tell_secret(string)string");
    // kernel_namespace_call(ns, "render", 0.0, null);

    kernel_call(kernel, "boot.render", 0.0, null);
    kernel_call(kernel, "test.say_hello", "James");
    return true;
}


b8 driver_destroy(const Namespace *ns) {
    vinfo("Destroying process");
    return true;
}


VAPI void render(f64 delta, const NVGcontext *ctx) {
    vinfo("Rendering, delta: %f", delta);
}


VAPI char *tell_secret(const char *name) {
    vinfo("Telling secret to %s", name);
    return "The secret is 42";
}
