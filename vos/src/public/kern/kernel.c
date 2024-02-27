/**
 * Created by jraynor on 2/24/2024.
 */
#include "kern/kernel.h"
#include <assert.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>

#include "containers/darray.h"
#include "containers/dict.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "core/vstring.h"
#include "lauxlib.h"
#include "lualib.h"
#include "platform/platform.h"
#include "kern/kernel_ext.h"

// Initializes the kernel.
VAPI Kernel *kernel_create(const char *root_path) {
    Kernel *kernel = kernel_get();
    if (kernel->initialized) {
        vwarn("Attempted to reinitialize the kernel. Please only call kernel_create once per application execution.")
        return kernel;
    }
    strings_initialize();
    kernel->root_path = root_path;
    kernel->process_count = 0;
    kernel->event_state = kallocate(sizeof(EventState), MEMORY_TAG_KERNEL);
    kernel->initialized = true;
    kernel->namespaces = dict_new();
    return kernel;
}

VAPI Kernel *kernel_get() {
    // The kernel is a singleton, so we can have a global instance of the kernel.
    static Kernel _kernel = {0};
    return &_kernel;
}

VAPI b8 kernel_destroy(const Kernel *kern) {
    const Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        vwarn("Attempted to destroy an uninitialized kernel. Please only call kernel_destroy once per application execution.")
        return false;
    }
    // We need to terminiate all processes before we can destroy the kernel.
    // kfree(kernel->event_state, sizeof(KernelEventState), MEMORY_TAG_KERNEL);
    // dict_delete(kernel->namespaces);
    strings_shutdown();
    return true;
}


VAPI Namespace *kernel_namespace(const Kernel *kernel, const char *name) {
    if (dict_contains(kernel->namespaces, name)) {
        return dict_get(kernel->namespaces, name);
    }
    Namespace *ns = kallocate(sizeof(Namespace), MEMORY_TAG_KERNEL);
    ns->name = name;
    ns->functions = dict_new();
    ns->kernel = kernel;
    dict_set(kernel->namespaces, name, ns);
    return ns;
}

VAPI b8 kernel_namespace_define(const Namespace *namespace, Function *function) {
    if (dict_contains(namespace->functions, function->signature.name)) {
        verror("Function %s already exists in namespace %s", function->signature.name, namespace->name);
        return false;
    }
    dict_set(namespace->functions, function->signature.name, function);
    return true;
}

VAPI b8 kernel_namespace_define_query(const Namespace *namespace, Process *process, const char *query) {
    Function *func = kernel_process_function_query(process, query);
    if (func == null) {
        verror("Failed to define function from query %s", query);
        return false;
    }
    return kernel_namespace_define(namespace, func);
}


VAPI Function *kernel_process_function_lookup(Process *process, const FunctionSignature signature) {
    Function *function = kallocate(sizeof(Function), MEMORY_TAG_KERNEL);
    function->base = process;
    function->signature = signature;
    switch (process->type) {
        case PROCESS_TYPE_DRIVER:
            const DriverProcess *dprocess = (DriverProcess *) process;
            const DynLib *handle = dprocess->handle;
            const DynLibFunction *func = platform_dynamic_library_load_function(signature.name, handle);
            if (func == null) {
                verror("Failed to load function %s from driver %s", signature.name, process->path);
                kfree(function, sizeof(Function), MEMORY_TAG_KERNEL);
                return null;
            }
            function->type = CALLABLE_TYPE_DRIVER;
            function->context.pfn = func->pfn;
            break;
        case PROCESS_TYPE_LUA:
            LuaProcess *lprocess = (LuaProcess *) process;
            lua_State *L = lprocess->lua_state;
        // Attempt to get the function by its name from the global namespace
            lua_getglobal(L, signature.name);
            function->context.lua.ref = luaL_ref(lprocess->lua_state, LUA_REGISTRYINDEX);

        // // Check if the value at the top of the stack is a function
        //     if (lua_isfunction(L, -1) == false) {
        //         verror("Failed to find function %s in Lua script", signature.name);
        //         lua_pop(L, 1); // Remove the non-function value from the stack
        //         return false;
        //     }
        // //checks the arg count
        //     if (lua_gettop(L) != signature.arg_count) {
        //         verror("Function %s in Lua script has the wrong number of arguments, expected %i, got %i", signature.name, signature.arg_count, lua_gettop(L));
        //         lua_pop(L, 1); // Remove the non-function value from the stack
        //         return false;
        //     }
            function->type = CALLABLE_TYPE_LUA;
            function->context.lua.lua_state = lprocess->lua_state;

            break;
        case PROCESS_TYPE_GRAVITY:
            break;
    }
    return function;
}


// Auxiliary function to map string to FunctionType


// Constructs a function signature from the query string
//Example query: "render(f32;pointer;f32);f32" would equate to "f32 render(f32, void*, f32)"
VAPI Function *kernel_process_function_query(Process *process, const char *query) {
    const FunctionSignature signature = kernel_process_create_signature(query);
    return kernel_process_function_lookup(process, signature);
}


VAPI FunctionResult kernel_process_function_call_internal(const Function *function, va_list args) {
    if (function->base->type == PROCESS_TYPE_DRIVER && function->context.pfn) {
        ffi_cif cif;
        ffi_type **arg_types = malloc(sizeof(ffi_type *) * function->signature.arg_count);
        void **arg_values = malloc(sizeof(void *) * function->signature.arg_count);
        void *ret_value = malloc(sizeof(void *));


        // Prepare argument types and values
        for (int i = 0; i < function->signature.arg_count; ++i) {
            arg_types[i] = kernel_function_type_to_ffi_type(function->signature.args[i]);
            arg_values[i] = kernel_allocate_arg_value(function->signature.args[i], &args);
        }

        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, function->signature.arg_count, kernel_function_type_to_ffi_type(function->signature.return_type), arg_types) == FFI_OK) {
            ffi_call(&cif, FFI_FN(function->context.pfn), ret_value, arg_values);
        } else {
            verror("Failed to prepare FFI call for function %s", function->signature.name);
        }

        FunctionResult func_result;
        kernel_function_result_from_ffi_return(ret_value, function->signature.return_type, &func_result);
        free(ret_value);
        // Clean up
        for (int i = 0; i < function->signature.arg_count; ++i) {
            free(arg_values[i]);
        }
        free(arg_types);
        free(arg_values);

        return func_result; // Assuming success
    }
    if (function->base->type == PROCESS_TYPE_LUA) {
        lua_State *L = function->context.lua.lua_state;
        const int base = lua_gettop(L); // Remember the stack's state to clean up later if needed
        lua_rawgeti(L, LUA_REGISTRYINDEX, function->context.lua.ref); // Push the function onto the stack
        // Assuming the rest of the arguments are handled similarly to how you've been handling them
        kernel_push_lua_args(L, function->signature, args);
        // Call the Lua function with arg_count arguments, expect 1 result (change as needed)
        if (lua_pcall(L, function->signature.arg_count, 1, 0) != LUA_OK) {
            FunctionResult func_result = {0};
            func_result.type = FUNCTION_TYPE_ERROR;
            func_result.data.error = lua_tostring(L, -1);
            lua_settop(L, base); // Restore stack state
            return func_result;
        }

        FunctionResult result = {0};
        // Interpret the return value from Lua stack
        if (function->signature.return_type == FUNCTION_TYPE_I32) {
            if (lua_isnumber(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_I32;
                result.data.i32 = (i32) lua_tointeger(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_F32) {
            if (lua_isnumber(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_F32;
                result.data.f32 = (f32) lua_tonumber(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_F64) {
            if (lua_isnumber(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_F64;
                result.data.f64 = (f64) lua_tonumber(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_U32) {
            if (lua_isnumber(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_U32;
                result.data.u32 = (u32) lua_tointeger(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_U64) {
            if (lua_isnumber(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_U64;
                result.data.u64 = (u64) lua_tointeger(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_BOOL) {
            if (lua_isboolean(L, -1)) {
                result.type = FUNCTION_TYPE_BOOL;
                result.data.boolean = lua_toboolean(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_I64) {
            if (lua_isnumber(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_I64;
                result.data.i64 = (i64) lua_tointeger(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_STRING) {
            if (lua_isstring(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_STRING;
                result.data.string = lua_tostring(L, -1);
            }
        }
        if (function->signature.return_type == FUNCTION_TYPE_POINTER) {
            if (lua_islightuserdata(L, -1)) {
                // Ensure the return value is of expected type
                result.type = FUNCTION_TYPE_POINTER;
                result.data.pointer = lua_touserdata(L, -1);
            }
        }
        lua_settop(L, base); // Clean the stack

        return result;
    }
    return (FunctionResult){0};
}

VAPI FunctionResult kernel_namespace_call(const Namespace *namespace, char *function, ...) {
    if (!dict_contains(namespace->functions, function)) {
        verror("Function %s does not exist in namespace %s", function, namespace->name);
        return (FunctionResult){.type = FUNCTION_TYPE_ERROR};
    }
    va_list args;
    va_start(args, function);
    const Function *func = dict_get(namespace->functions, function);
    const FunctionResult result = kernel_process_function_call_internal(func, args);
    va_end(args);
    return result;
}

VAPI FunctionResult kernel_call(const Kernel *kernel, const char *qualified_name, ...) {
    //Get the namespace by taking the first part of the qualified name
    const char *ns_name = strtok(qualified_name, ".");
    const char *func_name = strtok(NULL, ".");
    va_list args;
    va_start(args, qualified_name);
    const Namespace *ns = kernel_namespace(kernel, ns_name);
    const Function *func = dict_get(ns->functions, func_name);
    if (func == null) {
        verror("Function %s does not exist in namespace %s", func_name, ns_name);
        return (FunctionResult){.type = FUNCTION_TYPE_ERROR};
    }
    const FunctionResult result = kernel_process_function_call_internal(func, args);
    va_end(args);
    return result;
}

VAPI FunctionResult kernel_process_function_call(const Function *function, ...) {
    va_list args;
    va_start(args, function);
    const FunctionResult result = kernel_process_function_call_internal(function, args);
    va_end(args);
    return result;
}

//Process names are not unique, so we need to search for the process by name.
// There can be multiple processes with the same name so we allow for a query to be passed in.
// This will allow us to search for a process by name, returning the first process that matches the query.
// If no query is passed in, we will return the first process with the name.
// You can provide a query for the process name and . followed by a number to indicate which instance of the process you want.
VAPI Process *kernel_process_find(const Kernel *kern, const char *query) {
    const Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return null;
    }
    if (query == null) return null;
    const u32 query_len = strlen(query);
    for (u32 i = 0; i < kernel->process_count; ++i) {
        Process *process = kernel->processes[i];
        if (strncmp(process->name, query, query_len) == 0) {
            return process;
        }
    }
    return null;
}

/**
 * @brief Registers a listener function for a specific kernel event code.
 *
 * The listener function will be invoked when the specified kernel event occurs.
 *
 * @param kernel A pointer to the Kernel object.
 * @param code The code of the kernel event to register a listener for.
 * @param on_event The listener function to be invoked when the event occurs.
 * @return Returns true if the listener was successfully registered, false otherwise.
 */
// VAPI b8 kernel_event_listen(const Kernel *kern, const u16 code, const KernProcEventPFN on_event) {
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
//     event.is_raw = true;
//     event.callback.raw = on_event;
//     darray_push(KernelRegisterEvent, kernel->event_state->registered[code].events, event);
//     vinfo("Registered event in kernel at address %p, %p", kernel, kernel->event_state);
//     return true;
// }
//
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
//
// VAPI b8 kernel_event_unlisten_function(const u16 code, const Function *function) {
//     const Kernel *kernel = kernel_get();
//     if (!kernel->initialized) {
//         verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
//         return false;
//     }
//     // On nothing is registered for the code, boot out.
//     if (kernel->event_state->registered[code].events == 0) {
//         return false;
//     }
//     const u64 registered_count = darray_length(kernel->event_state->registered[code].events);
//     for (u64 i = 0; i < registered_count; ++i) {
//         KernelRegisterEvent e = kernel->event_state->registered[code].events[i];
//         if (e.callback.function == function) {
//             // Found one, remove it
//             KernelRegisterEvent popped_event;
//             darray_pop_at(kernel->event_state->registered[code].events, i, &popped_event);
//             return true;
//         }
//     }
//     // Not found.
//     return false;
// }
//
// VAPI b8 kernel_event_trigger(const Kernel *kern, const KernProcEvent *event) {
//     const Kernel *kernel = kern ? kern : kernel_get();
//     if (!kernel->initialized) {
//         verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
//         return false;
//     }
//     // If nothing is registered for the code, boot out.
//     if (kernel->event_state->registered[event->code].events == 0) {
//         return false;
//     }
//     const u64 registered_count = darray_length(kernel->event_state->registered[event->code].events);
//     for (u64 i = 0; i < registered_count; ++i) {
//         const KernelRegisterEvent e = kernel->event_state->registered[event->code].events[i];
//         if (e.is_raw && e.callback.raw(event)) {
//             return true; // If the event was consumed, return true.
//         }
//         if (e.callback.function) {
//             // Call the function
//             const FunctionResult result = kernel_process_function_call(e.callback.function, event);
//             if (result.type == FUNCTION_TYPE_BOOL && result.data.boolean) {
//                 return true;
//             } //Cary on to the next event because this one didn't "consume" the event.
//         }
//     }
//     return false;
// }
//
// VAPI KernProcEvent kernel_event_create(const Kernel *kernel, const KernelEventCode code, const EventData *data) {
//     return (KernProcEvent) {
//         .
//         code = code,
//         .
//         data = data,
//         .
//         sender.kernel = kernel,
//         .
//         sender.process = null
//     };
// }
//
// VAPI KernProcEvent kernel_process_event_create(const Process *process, const KernelEventCode event, const EventData *data) {
//     return (KernProcEvent) {
//         .
//         code = event,
//         .
//         data = data,
//         .
//         sender.kernel = null,
//         .
//         sender.process = process
//     };
// }
//
// VAPI b8 kernel_event_unlisten(const u16 code, const KernProcEventPFN on_event) {
//     const Kernel *kernel = kernel_get();
//     if (!kernel->initialized) {
//         verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
//         return false;
//     }
//     // On nothing is registered for the code, boot out.
//     if (kernel->event_state->registered[code].events == 0) {
//         return false;
//     }
//     const u64 registered_count = darray_length(kernel->event_state->registered[code].events);
//     for (u64 i = 0; i < registered_count; ++i) {
//         KernelRegisterEvent e = kernel->event_state->registered[code].events[i];
//         if (e.callback.raw == on_event) {
//             // Found one, remove it
//             KernelRegisterEvent popped_event;
//             darray_pop_at(kernel->event_state->registered[code].events, i, &popped_event);
//             return true;
//         }
//     }
//     // Not found.
//     return false;
// }
