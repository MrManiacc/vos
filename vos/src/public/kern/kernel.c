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

#ifndef KERNEL_MAX_PROCESSES
#define KERNEL_MAX_PROCESSES 256
#endif

// Process State, used to determine if a process is running, paused, stopped, or dead.
typedef enum ProcessState {
    PROCESS_STATE_RUNNING, // In this state, the process is running normally. This is the default state of a process.
    PROCESS_STATE_PAUSED, // In this state, the process is paused. This is used to temporarily halt processing of a process.
    PROCESS_STATE_STOPPED, // In this state, the process is stopped. This is the initial state of a process.
    PROCESS_STATE_DESTROYED, // In this state, the process is dead. This is the final state of a process. Once a process is dead, it cannot be restarted.
    PROCESS_STATE_MAX_STATES
} ProcessState;

// Process Type, used to determine if a process is a kernel process or a user process.
typedef enum ProcessType {
    PROCESS_TYPE_DRIVER, // For drivers, these are shared libraries that are loaded into the kernel.
    PROCESS_TYPE_LUA, // For user processes, these are lua scripts that are executed by the kernel.
    PROCESS_TYPE_GRAVITY // For user processes, these are gravity lang scripts that are executed by the kernel.
} ProcessType;

// Process Exit Code, used to determine the exit code of a process.
typedef enum ProcessExitCode {
    PROCESS_EXIT_CODE_SUCCESS, // The process exited successfully.
    PROCESS_EXIT_CODE_KILLED, // The process was killed by the kernel.
    PROCESS_EXIT_CODE_RELOADED, // The process was reloaded by the kernel.
    PROCESS_EXIT_CODE_FAILURE // The process exited with an error.
} ProcessExitCode;

typedef struct KernelRegisterEvent {
    KernelEventCode code;
    KernelEventPFN callback;
} KernelRegisterEvent;

typedef struct KernelEventBag {
    KernelRegisterEvent *events;
} KernelEventBag;

// This should be more than enough codes...
#define MAX_MESSAGE_CODES 512

// State structure.
typedef struct KernelEventState {
    // Lookup table for event codes.
    KernelEventBag registered[MAX_MESSAGE_CODES];
} KernelEventState;


struct Process {
    // The type of the process. This is used to determine if a process is a kernel process or a user process.
    ProcessType type;
    // The state of the process. This is used to determine if a process is running, paused, stopped, or dead.
    ProcessState state;
    // The unique id of the process. This is used to identify the process.
    ProcessID pid;
    // The path of the process. This is used to identify the process.
    ProcessPath path;
    // The name of the process. Extracted from the paths filename.
    ProcessName name;
    // Stores the list of functions that have been looked up so they can be freed later.
    Dict *functions;
};

typedef struct DriverProcess {
    Process base; // The base process. This allows us to treat a driver process as a process.
    DynLib *handle; // The library handle
} DriverProcess;


typedef struct LuaProcess {
    Process base;
    lua_State *lua_state;
} LuaProcess;

typedef enum CallableType {
    CALLABLE_TYPE_DRIVER,
    CALLABLE_TYPE_LUA,
    CALLABLE_TYPE_GRAVITY, // Future expansion
} CallableType;

// A callable context is used to call a function in a process.
struct Function {
    Process *base; // To allow us to treat a process function as a process.
    CallableType type;
    FunctionSignature signature; // The function's signature

    union {
        void *pfn; // the raw function pointer for a driver function
        struct lua {
            lua_State *lua_state; // Lua context for a lua function
            int ref; // The reference to the function in the Lua registry
        } lua;

        //TODO gravity context
    } context;
};


// The kernel should be responsible for managing the state of the system.
// This includes the process tree, the event system, the file system, and the driver system.
//
struct Kernel {
    b8 initialized; // Whether the kernel has been initialized.
    ProcessPath root_path; // The root path of the file system.
    Process *processes[KERNEL_MAX_PROCESSES]; // The root process view is the root of the process tree. This is the first process that is executed.
    u32 process_count; // The number of processes in the system.
    KernelEventState *event_state; // The event state is used to track the state of the event system.
    Dict *process_groups; // A dictionary of process groups. They are indexed by their name.
};

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
    kernel->event_state = kallocate(sizeof(KernelEventState), MEMORY_TAG_KERNEL);
    kernel->initialized = true;
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

    strings_shutdown();
    return true;
}

Process *kernel_new_driver_process(Kernel *kern, const char *driver_path) {
    Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attempted to create a Lua process without initializing the kernel. Please call kernel_create before creating a Lua process.");
        return NULL;
    }
    Process *process = kallocate(sizeof(DriverProcess), MEMORY_TAG_KERNEL);
    if (!process) {
        verror("Failed to allocate memory for Driver process.");
        return NULL;
    }
    process->type = PROCESS_TYPE_DRIVER;
    process->path = driver_path;
    process->name = platform_file_name(driver_path);
    if (kernel->process_count >= KERNEL_MAX_PROCESSES) {
        verror("Too many processes");
        kfree(process, sizeof(DriverProcess), MEMORY_TAG_KERNEL);
        return NULL;
    }
    process->pid = kernel->process_count++;
    DriverProcess *dprocess = (DriverProcess *) process;
    DynLib *handle = kallocate(sizeof(DynLib), MEMORY_TAG_KERNEL);
    if (!platform_dynamic_library_load(driver_path, handle)) {
        verror("Failed to load driver %s", driver_path);
        kfree(handle, sizeof(DynLib), MEMORY_TAG_KERNEL);
        kfree(process, sizeof(DriverProcess), MEMORY_TAG_KERNEL);
        return null;
    }
    dprocess->handle = handle;
    // At this point, the Lua script is loaded but not executed.
    // You can now lookup functions within the script and call them later.
    kernel->processes[kernel->process_count - 1] = process; // Store the process in the kernel's processes array
    return process;
}


Process *kernel_new_lua_process(Kernel *kern, const char *script_path) {
    Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attempted to create a Lua process without initializing the kernel. Please call kernel_create before creating a Lua process.");
        return NULL;
    }

    LuaProcess *lua_process = kallocate(sizeof(LuaProcess), MEMORY_TAG_KERNEL);
    if (!lua_process) {
        verror("Failed to allocate memory for Lua process.");
        return NULL;
    }

    Process *process = (Process *) lua_process;
    process->type = PROCESS_TYPE_LUA;
    if (kernel->process_count >= KERNEL_MAX_PROCESSES) {
        verror("Too many processes");
        kfree(lua_process, sizeof(LuaProcess), MEMORY_TAG_KERNEL);
        return NULL;
    }
    process->pid = kernel->process_count++;
    process->path = script_path;
    process->state = PROCESS_STATE_STOPPED;
    process->name = platform_file_name(script_path);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    // Load and execute the Lua script

    const char *source = platform_read_file(script_path);
    const u32 size = platform_file_size(script_path);
    if (luaL_loadbuffer(L, source, size, script_path) != LUA_OK) {
        verror("Failed to load Lua script %s: %s", script_path, lua_tostring(L, -1));
        lua_close(L);
        kfree(lua_process, sizeof(LuaProcess), MEMORY_TAG_KERNEL);
        return null;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        verror("Failed to execute Lua script %s: %s", script_path, lua_tostring(L, -1));
        lua_close(L);
        kfree(lua_process, sizeof(LuaProcess), MEMORY_TAG_KERNEL);
        return null;
    }
    lua_process->lua_state = L;
    kernel->processes[kernel->process_count - 1] = process; // Store the process in the kernel's processes array
    return process;
}

VAPI Process *kernel_new_gravity_process(Kernel *kern, const char *script_path) {
    Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return null;
    }
    Process *process = kallocate(sizeof(Process), MEMORY_TAG_KERNEL);
    process->type = PROCESS_TYPE_GRAVITY;
    kernel->processes[kernel->process_count] = process;
    process->pid = kernel->process_count++;
    process->path = script_path;
    return process;
}


VAPI Process *kernel_process_new(Kernel *kernel, const char *driver_path) {
    const char *ext = strrchr(driver_path, '.');
    if (ext == null) {
        verror("Invalid driver path. Please provide a valid driver path.")
        return null;
    }
    Process *process = null;
    if (strcmp(ext, platform_dynamic_library_extension()) == 0) {
        process = kernel_new_driver_process(kernel, driver_path);
    }
    if (strcmp(ext, ".lua") == 0) {
        process = kernel_new_lua_process(kernel, driver_path);
    }
    if (strcmp(ext, ".gravity") == 0) {
        process = kernel_new_gravity_process(kernel, driver_path);
    }
    if (process == null) {
        verror("Invalid driver path. Please provide a valid driver path.")
        return null;
    }
    return process;
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

ffi_type *function_type_to_ffi_type(const FunctionType type) {
    switch (type) {
        case FUNCTION_TYPE_VOID: return &ffi_type_void;
        case FUNCTION_TYPE_I32: return &ffi_type_sint32;
        case FUNCTION_TYPE_U32: return &ffi_type_uint32;
        case FUNCTION_TYPE_I64: return &ffi_type_sint64;
        case FUNCTION_TYPE_U64: return &ffi_type_uint64;
        case FUNCTION_TYPE_F32: return &ffi_type_float;
        case FUNCTION_TYPE_F64: return &ffi_type_double;
        case FUNCTION_TYPE_STRING: return &ffi_type_pointer; // For simplicity, we'll assume all strings are pointers.
        case FUNCTION_TYPE_POINTER: return &ffi_type_pointer;
        case FUNCTION_TYPE_BOOL: return &ffi_type_uint8;
        default: return NULL;
    }
}

void *allocate_arg_value(FunctionType type, va_list *args) {
    void *value;
    switch (type) {
        case FUNCTION_TYPE_I32: {
            int *arg = malloc(sizeof(i32));
            *arg = va_arg(*args, int);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_F32: {
            float *arg = malloc(sizeof(f32));
            *arg = va_arg(*args, f32);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_F64: {
            double *arg = malloc(sizeof(f64));
            *arg = va_arg(*args, f64);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_I64: {
            i64 *arg = malloc(sizeof(i64));
            *arg = va_arg(*args, i64);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_U32: {
            u32 *arg = malloc(sizeof(u32));
            *arg = va_arg(*args, u32);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_U64: {
            u64 *arg = malloc(sizeof(u64));
            *arg = va_arg(*args, u64);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_STRING: {
            void **arg = malloc(sizeof(char *));
            *arg = va_arg(*args, char*);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_POINTER: {
            void **arg = malloc(sizeof(void *));
            *arg = va_arg(*args, void*);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_BOOL: {
            b8 *arg = malloc(sizeof(b8));
            *arg = va_arg(*args, b8);
            value = arg;
            break;
        }
        // Add cases for other types as necessary
        default:
            value = NULL;
            break;
    }

    return value;
}


void push_lua_args(lua_State *L, FunctionSignature signature, va_list args) {
    for (int i = 0; i < signature.arg_count; i++) {
        switch (signature.args[i]) {
            case FUNCTION_TYPE_U32:
            case FUNCTION_TYPE_U64:
            case FUNCTION_TYPE_I64:
            case FUNCTION_TYPE_I32: {
                const int value = va_arg(args, int);
                lua_pushinteger(L, value);
                break;
            }
            case FUNCTION_TYPE_F64:
            case FUNCTION_TYPE_F32: {
                // Assuming float values are promoted to double in va_arg
                const double value = va_arg(args, double);
                lua_pushnumber(L, value);
                break;
            }
            case FUNCTION_TYPE_STRING: {
                const char *value = va_arg(args, char*);
                lua_pushstring(L, value);
                break;
            }
            case FUNCTION_TYPE_POINTER: {
                void *value = va_arg(args, void*);
                lua_pushlightuserdata(L, value);
                break;
            }
            case FUNCTION_TYPE_BOOL: {
                const b8 value = va_arg(args, b8);
                lua_pushboolean(L, value);
                break;
            }
            // Add more cases for other types as needed
            default:
                // Handle unknown type or error
                verror("Unknown type")
                break;
        }
    }
}

void populate_function_result_from_ffi_return(void *ret_value, const FunctionType return_type, FunctionResult *result) {
    if (!ret_value || !result) return;

    // Initialize the result type
    result->type = return_type;

    // Based on the return type, interpret ret_value and populate the union in result
    switch (return_type) {
        case FUNCTION_TYPE_I32: {
            result->data.i32 = *(i32 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_F32: {
            result->data.f32 = *(f32 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_F64: {
            result->data.f64 = *(f64 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_U32: {
            result->data.u32 = *(u32 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_U64: {
            result->data.u64 = *(u64 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_I64: {
            result->data.i64 = *(i64 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_STRING: {
            result->data.string = *(const char **) ret_value;
            break;
        }
        case FUNCTION_TYPE_POINTER: {
            result->data.pointer = *(void **) ret_value;
            break;
        }
        case FUNCTION_TYPE_BOOL: {
            result->data.boolean = *(b8 *) ret_value;
            break;
        }
        case FUNCTION_TYPE_VOID:
        default: {
            // For void return type or unknown type, do nothing
            // or set result data to NULL/zero as appropriate
            memset(&result->data, 0, sizeof(result->data));
            break;
        }
    }
}

VAPI FunctionResult kernel_process_function_call(const Function *function, ...) {
    if (function->base->type == PROCESS_TYPE_DRIVER && function->context.pfn) {
        ffi_cif cif;
        ffi_type **arg_types = malloc(sizeof(ffi_type *) * function->signature.arg_count);
        void **arg_values = malloc(sizeof(void *) * function->signature.arg_count);
        void *ret_value = malloc(sizeof(void *));

        va_list args;
        va_start(args, function);

        // Prepare argument types and values
        for (int i = 0; i < function->signature.arg_count; ++i) {
            arg_types[i] = function_type_to_ffi_type(function->signature.args[i]);
            arg_values[i] = allocate_arg_value(function->signature.args[i], &args);
        }

        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, function->signature.arg_count, function_type_to_ffi_type(function->signature.return_type), arg_types) == FFI_OK) {
            ffi_call(&cif, FFI_FN(function->context.pfn), ret_value, arg_values);
        } else {
            verror("Failed to prepare FFI call for function %s", function->signature.name);
        }

        FunctionResult func_result;
        populate_function_result_from_ffi_return(ret_value, function->signature.return_type, &func_result);
        free(ret_value);
        // Clean up
        for (int i = 0; i < function->signature.arg_count; ++i) {
            free(arg_values[i]);
        }
        free(arg_types);
        free(arg_values);
        va_end(args);

        return func_result; // Assuming success
    }
    if (function->base->type == PROCESS_TYPE_LUA) {
        lua_State *L = function->context.lua.lua_state;
        const int base = lua_gettop(L); // Remember the stack's state to clean up later if needed
        lua_rawgeti(L, LUA_REGISTRYINDEX,  function->context.lua.ref); // Push the function onto the stack
        // Assuming the rest of the arguments are handled similarly to how you've been handling them
        va_list args;
        va_start(args, function);
        push_lua_args(L, function->signature, args);
        va_end(args);
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


VAPI b8 kernel_process_run(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_pause(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_resume(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_stop(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_terminate(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI Process *kernel_process_get(const ProcessID pid) {
    const Kernel *kernel = kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return null;
    }
    if (pid >= kernel->process_count) return null;
    return kernel->processes[pid];
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
VAPI b8 kernel_event_listen(const u16 code, const KernelEventPFN on_event) {
    const Kernel *kernel = kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    if (kernel->event_state->registered[code].events == 0) {
        kernel->event_state->registered[code].events = darray_create(KernelRegisterEvent);
    }
    // If at this point, no duplicate was found. Proceed with registration.
    KernelRegisterEvent event;
    event.code = code;
    event.callback = on_event;
    darray_push(KernelRegisterEvent, kernel->event_state->registered[code].events, event);
    vinfo("Registered event in kernel at address %p, %p", kernel, kernel->event_state);
    return true;
}

VAPI b8 kernel_event_trigger(const u16 code, const EventData context) {
    Kernel *kernel = kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    // If nothing is registered for the code, boot out.
    if (kernel->event_state->registered[code].events == 0) {
        return false;
    }
    const u64 registered_count = darray_length(kernel->event_state->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        KernelRegisterEvent e = kernel->event_state->registered[code].events[i];
        if (e.callback(kernel, code, context)) {
            // Message has been handled, do not send to other listeners.
            return true;
        }
    }
    return false;
}

VAPI b8 kernel_event_unlisten(const u16 code, const KernelEventPFN on_event) {
    const Kernel *kernel = kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    // On nothing is registered for the code, boot out.
    if (kernel->event_state->registered[code].events == 0) {
        return false;
    }
    const u64 registered_count = darray_length(kernel->event_state->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        KernelRegisterEvent e = kernel->event_state->registered[code].events[i];
        if (e.callback == on_event) {
            // Found one, remove it
            KernelRegisterEvent popped_event;
            darray_pop_at(kernel->event_state->registered[code].events, i, &popped_event);
            return true;
        }
    }
    // Not found.
    return false;
}
