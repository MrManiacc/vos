#include "core/vevent.h"

#include "vmem.h"
#include "containers/darray.h"
#include "vlogger.h"


typedef struct registered_event {
    void *listener;
    PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
    registered_event *events;
} event_code_entry;

// This should be more than enough codes...
#define MAX_MESSAGE_CODES 16384

// State structure.
struct EventState {
    // Lookup table for event codes.
    event_code_entry registered[MAX_MESSAGE_CODES];
} EventState;

/**
 * Event system internal state.
 */

VAPI b8 event_initialize(Kernel *kernel) {
    kernel->event_state = kallocate(sizeof(EventState), MEMORY_TAG_KERNEL);
    return true;
}

VAPI void event_shutdown(Kernel *kernel) {
    // Free the events arrays. And objects pointed to should be destroyed on their own.
    for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
        if (kernel->event_state->registered[i].events != 0) {
            darray_destroy(kernel->event_state->registered[i].events);
            kernel->event_state->registered[i].events = 0;
        }
    }
    kfree(kernel->event_state, sizeof(EventState), MEMORY_TAG_KERNEL);
}

VAPI b8 event_register(Kernel *kernel, u16 code, void *listener, PFN_on_event on_event) {
    if (kernel->event_state->registered[code].events == 0) {
        kernel->event_state->registered[code].events = darray_create(registered_event);
    }
    
    u64 registered_count = darray_length(kernel->event_state->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        if (kernel->event_state->registered[code].events[i].listener == listener) {
            // TODO: warn
            return false;
        }
    }
    
    
    // If at this point, no duplicate was found. Proceed with registration.
    registered_event event;
    event.listener = listener;
    event.callback = on_event;
    darray_push(registered_event, kernel->event_state->registered[code].events, event);
    vinfo("Registered event in kernel at address %p, %p", kernel, kernel->event_state);
    return true;
}


VAPI b8 event_unregister(Kernel *kernel, u16 code, void *listener, PFN_on_event on_event) {
    
    // On nothing is registered for the code, boot out.
    if (kernel->event_state->registered[code].events == 0) {
        // TODO: warn
        return false;
    }
    
    u64 registered_count = darray_length(kernel->event_state->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = kernel->event_state->registered[code].events[i];
        if (e.listener == listener && e.callback == on_event) {
            // Found one, remove it
            registered_event popped_event;
            darray_pop_at(kernel->event_state->registered[code].events, i, &popped_event);
            return true;
        }
    }
    
    // Not found.
    return false;
}

VAPI b8 event_fire(Kernel *kernel, u16 code, void *sender, event_context context) {
    
    // If nothing is registered for the code, boot out.
    if (kernel->event_state->registered[code].events == 0) {
        return false;
    }
    
    u64 registered_count = darray_length(kernel->event_state->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = kernel->event_state->registered[code].events[i];
        if (e.callback(kernel, code, sender, e.listener, context)) {
            // Message has been handled, do not send to other listeners.
            return true;
        }
    }
    
    // Not found.
    return false;
}


