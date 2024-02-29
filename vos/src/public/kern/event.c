/**
 * Created by jraynor on 2/27/2024.
 */
#include "kernel_internal.h"
#include "core/vlogger.h"
// VAPI b8 kernel_event_listen_function(const Kernel *kern, const u16 code, const Function *function) {
//     const Kernel *kernel = kern ? kern : kernel_get();
//     if (!kernel->initialized) {
//         verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
//         return false;
//     }
//     if (kernel->event_state->registered[code].events == 0) {
//         kernel->event_state->registered[code].events = darray_create(KernelRegisterEvent);
//     }
//     // If at this point, no duplicate was found. Proceed with registration.
//     KernelRegisterEvent event;
//     event.code = code;
//     event.is_raw = false;
//     event.callback.function = function;
//     darray_push(KernelRegisterEvent, kernel->event_state->registered[code].events, event);
//     return true;
// }
VAPI b8 kernel_event_listen(const Kernel *kernel, u8 code, const Function *function) {
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    if (kernel->event_state->registered[code].register_listeners == 0) {
        kernel->event_state->registered[code].register_listeners = darray_create(EventListener);
    }
    // If at this point, no duplicate was found. Proceed with registration.
    EventListener event;
    event.code = code;
    event.function = function;
    darray_push(EventListener, kernel->event_state->registered[code].register_listeners, event);
    return true;
}

VAPI b8 kernel_event_trigger(const Kernel *kernel, u8 code, EventData *data) {
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    // If nothing is registered for the code, boot out.
    if (kernel->event_state->registered[code].register_listeners == 0) {
        return false;
    }
    const u64 registered_count = darray_length(kernel->event_state->registered[code].register_listeners);
    for (u64 i = 0; i < registered_count; ++i) {
        const EventListener *event = darray_get(kernel->event_state->registered[code].register_listeners, i);
        const FunctionResult result = kernel_process_function_call(event->function, data);
        if (result.type != FUNCTION_TYPE_BOOL) {
            verror("Error occurred while processing event: %s", result.data.string);
            continue;
        }
        if (result.data.boolean) return true; // If the event was handled, return true.
    }
    return false;
}

VAPI b8 kernel_event_unlisten(const Kernel *kernel, u8 code, const Function *function) {
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    if (kernel->event_state->registered[code].register_listeners == 0) {
        return false;
    }
    // If at this point, no duplicate was found. Proceed with registration.
    u64 index = darray_find(kernel->event_state->registered[code].register_listeners, function);
    if (index == -1) {
        return false;
    }
    darray_pop_at(kernel->event_state->registered[code].register_listeners, index, null);
    return true;
}
