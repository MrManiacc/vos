#pragma once

#include "kern/kernel.h"
#include <lua.h>
#include "containers/darray.h"
#include "containers/dict.h"
#include "platform/platform.h"

// This should be more than enough codes...
#ifndef MAX_MESSAGE_CODES
#define MAX_MESSAGE_CODES 255
#endif
#ifndef KERNEL_MAX_PROCESSES
#define KERNEL_MAX_PROCESSES 255
#endif

// Process State, used to determine if a process is running, paused, stopped, or dead.
typedef enum ProcessState {
    PROCESS_STATE_UNINITIALIZED, // In this state, the process is running normally. This is the default state of a process.
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


// The namespace is used to store a list of functions that are available to a process.
struct Namespace {
    const Kernel *kernel; //Store a refernce to the kernel to allow for easy access to the kernel.
    const char *name;
    Dict *functions;
};


// The kernel should be responsible for managing the state of the system.
// This includes the process tree, the event system, the file system, and the driver system.
//
struct Kernel {
    b8 initialized; // Whether the kernel has been initialized.
    ProcessPath root_path; // The root path of the file system.
    Process *processes[KERNEL_MAX_PROCESSES]; // The root process view is the root of the process tree. This is the first process that is executed.
    u32 process_count; // The number of processes in the system.
    struct EventState *event_state; // The event state is used to track the state of the event system.
    Dict *namespaces; // The namespaces are used to store a list of functions that are available to a process.
};


typedef struct EventListener {
    u8 code; // The event code.
    void *context; // The context to be passed to the event handler.
    const Function *function; // The function to be called when the event is fired.
} EventListener;

typedef struct EventBag {
    void *register_listeners; // a darray of EventListeners
} EventBag;


// State structure.
typedef struct EventState {
    // Lookup table for event codes.
    EventBag registered[EVENT_MAX_CODE];
} EventState;


typedef enum {
    ARG_INTEGER,
    ARG_NUMBER,
    ARG_STRING,
    ARG_BOOLEAN,
    ARG_POINTER,
    ARG_FUNCTION,
    ARG_NIL,
    // Add other types as necessary
} ArgType;


typedef struct LuaArg {
    ArgType type;

    union {
        i32 i;
        f64 f;
        const char *s;
        const void *p;
        Function *fn;
        b8 b;
    } value;
} LuaArg;

typedef struct Args {
    LuaArg args[8];
    u32 count;
} Args;


void kernel_setup_lua_process_internal(Kernel *kernel, Process *process, lua_State *L);

void *kernel_allocate_arg_value(FunctionType type, va_list *args);

ffi_type *kernel_function_type_to_ffi_type(const FunctionType type);

FunctionType kernel_string_to_function_type(const char *type_str);

void kernel_function_result_from_ffi_return(void *ret_value, const FunctionType return_type, FunctionResult *result);

FunctionSignature kernel_process_create_signature(const char *query);

Args kernel_proecss_lua_args(lua_State *L, const int argStartIndex);

void *kernel_allocate_and_set_arg_value(FunctionType type, LuaArg larg);

void kernel_push_lua_args(lua_State *L, FunctionSignature signature, va_list args);
