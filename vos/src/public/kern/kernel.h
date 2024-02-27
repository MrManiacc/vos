/**
 * Created by jraynor on 2/24/2024.
 */
#pragma once

#include <ffi.h>

#include "defines.h"
#ifndef MAX_FUNCTION_ARGS
#define MAX_FUNCTION_ARGS 16
#endif

/**The kernel should be responsible for managing the state of the system.
 * This includes the process tree, the event system, the file system, and the driver system.
 *
 * We use an opaque pointer to the kernel to prevent the user from accessing the kernel directly.
 *
 * Only the kernel it's self will handle the creation and destruction of processes.
 */
typedef struct Kernel Kernel;

// The unique process id for a process.
typedef u16 ProcessID;

// A process will always be something loaded from somewhere, it may be related to a file, or a driver, or a script etc
typedef const char *ProcessPath;

// The name of a driver process. This is used to identify the driver uniquely in the system.
typedef const char *ProcessName;

// Process Structure that represents a process in the system.
typedef struct Process Process;

// A function pointer that allows callable context to be passed to a function.
typedef struct Function Function;

// A namespace is a collection of functions that can be called by a process.
typedef struct Namespace Namespace;


typedef enum FunctionType {
    FUNCTION_TYPE_STRING,
    FUNCTION_TYPE_F32,
    FUNCTION_TYPE_F64,
    FUNCTION_TYPE_U32,
    FUNCTION_TYPE_U64,
    FUNCTION_TYPE_I32,
    FUNCTION_TYPE_BOOL, //b8
    FUNCTION_TYPE_I64,
    FUNCTION_TYPE_VOID,
    FUNCTION_TYPE_POINTER,
    FUNCTION_TYPE_ERROR
} FunctionType;


typedef struct FunctionResult {
    FunctionType type;

    union {
        const char *error;
        f32 f32;
        f64 f64;
        u32 u32;
        u64 u64;
        i32 i32;
        i64 i64;
        b8 boolean;
        void *pointer;
        const char *string;
    } data;
} FunctionResult;

typedef struct FunctionSignature {
    const char *name; // The function name
    FunctionType args[MAX_FUNCTION_ARGS];
    u8 arg_count;
    FunctionType return_type;
} FunctionSignature;

#define EVENT_RESERVED 0x00, // Reserved event code. No event should have this code.
#define EVENT_KERNEL_INIT 0x01, // The kernel has been initialized.
#define EVENT_PROCESS_START 0x02, // A process has started.
#define EVENT_KERNEL_RENDER 0x07, // The kernel is rendering.
#define EVENT_MAX_CODE 0xFF // The maximum number of event codes.


/**
 * @brief Represents event contextual data to be sent along with an
 * event code when an event is fired.
 * It is a union that is 128 bits in size, meaning data can be mixed
 * and matched as required by the developer.
 * */
typedef union EventData {
    /** @brief An array of 2 64-bit signed integers. */
    i64 i64[2];
    /** @brief An array of 2 64-bit unsigned integers. */
    u64 u64[2];

    /** @brief An array of 2 64-bit floating-point numbers. */
    f64 f64[2];

    /** @brief An array of 4 32-bit signed integers. */
    i32 i32[4];
    /** @brief An array of 4 32-bit unsigned integers. */
    u32 u32[4];
    /** @brief An array of 4 32-bit floating-point numbers. */
    f32 f32[4];

    /** @brief An array of 8 16-bit signed integers. */
    i16 i16[8];

    /** @brief An array of 8 16-bit unsigned integers. */
    u16 u16[8];

    /** @brief An array of 16 8-bit signed integers. */
    i8 i8[16];
    /** @brief An array of 16 8-bit unsigned integers. */
    u8 u8[16];

    /** @brief An array of 16 characters. */
    char c[16];

    /** @brief Two pointers which is 16 bytes total */
    struct {
        void *one;
        void *two;
    } pointers;
} EventData;


VAPI b8 kernel_event_listen(const struct ernel *kernel, u8 code, const struct Function *function);

VAPI b8 kernel_event_trigger(const struct Kernel *kernel, u8 code, EventData *event);

VAPI b8 kernel_event_unlisten(u8 code, const struct Function *function);

// ====================================================================================================================='
// Kernel Management
// =====================================================================================================================

/**
 * @brief Initializes the kernel. This will allocate the kernel context and initialize the root process view.
 *
 * The first initialized Kernel will the the "root kernel" because it will be statically allocated and will be the only kernel in the system.
 * @param root_path The path to the root directory.
 */
VAPI Kernel *kernel_create(const char *root_path);

/**
 * Returns a pointer to the Kernel.
 *
 * This method is used to retrieve a pointer to the Kernel object, which is responsible for managing the state of the system. The Kernel is responsible for managing the process tree, the event system, the file system, and the driver system.
 *
 * Note that a pointer to the Kernel object is returned as an opaque pointer to prevent direct access to the kernel by the user. The Kernel object will handle the creation and destruction of processes internally.
 *
 * @return A pointer to the Kernel object.
 */
VAPI Kernel *kernel_get();

/**
 * @brief Shuts down the kernel. This will free the kernel context and all associated resources.
 * It also stops all processes and unloads all drivers.
 */
VAPI b8 kernel_destroy(const Kernel *kern);

// =====================================================================================================================
// Process Management
// =====================================================================================================================

VAPI Process *kernel_process_load(Kernel *kernel, const char *driver_path);
/**
 * Looks up a callback function in a process.
 */
VAPI Function *kernel_process_function_lookup(Process *process, FunctionSignature signature);
/**
 *Constructs a function signature from a string.
 */
VAPI Function *kernel_process_function_query(Process *process, const char *query);

/**
 *A namespace is a singleton stored directly in the kernel. If it doesn't exist when looking up,
 * it will be created then return.
 */
VAPI Namespace *kernel_namespace(const Kernel *kernel, const char *name);

/**
 * Defines a function in a namespace. This allows the function to be called directly useing the namespace.
 *
 * Namespaces are a wrapper around multiple process's functions. Allowing for a single point of access to multiple functions.
 */
VAPI b8 kernel_namespace_define(const Namespace *namespace, Function *signature);

//Define a function using a query
VAPI b8 kernel_namespace_define_query(const Namespace *namespace, Process *process, const char *query);

VAPI FunctionResult kernel_namespace_call(const Namespace *namespace, char *function, ...);

// Looks up the function in the namespace, if it doesn't exist, it will return null.
VAPI FunctionResult kernel_call(const Kernel *kernel, const char *qualified_name, ...);

/**
 * Calls a function in a process.
 * @param func The function to call.
 * @param ... The arguments to pass to the function.
 */
VAPI FunctionResult kernel_process_function_call(const Function *func, ...);

/**
 * Starts a process. This will start the process and all child processes.
 */
VAPI b8 kernel_process_run(Kernel *kernel, Process *process);

/**
 * Pauses a process. This will pause the process and all child processes.
 * @param process The process to pause.
 * @return TRUE if the process was successfully paused; otherwise FALSE.
 */

VAPI b8 kernel_process_pause(Process *process);

/**
 * Resumes a process. This will resume the process and all child processes.
 * @param process The process to resume.
 * @return TRUE if the process was successfully resumed; otherwise FALSE.
 */
VAPI b8 kernel_process_resume(Process *process);

/**
 * Stops a process. This will stop the process and all child processes. If the process is already stopped, this function will do nothing.
 * @param process The process to stop.
 */
VAPI b8 kernel_process_stop(Process *process);

/**
 * Terminates a process. This will terminate the process and all child processes. If the process is already terminated, this function will do nothing.
 * @param process The process to terminate.
 * @return TRUE if the process was successfully terminated; otherwise FALSE.
 */
VAPI b8 kernel_process_terminate(Process *process);


/**
 * @breif Searches the process tree for a process with the given process id.
 *
 * @return The process with the given process id, or null if no such process exists.
 */
VAPI Process *kernel_process_get(ProcessID pid);

/**
 * @breif Searches the process tree for a process with the given process name.
 *
 * @return The FIRST process with the given process name, starting from the root directory
 */
VAPI Process *kernel_process_find(const Kernel *kernel, const char *query);


// =====================================================================================================================
// Driver Management
// =====================================================================================================================

// Initializes the kernel. This will allocate the kernel context and initialize the root process view.
// Should be called from within the main c file of a driver.
#define $driver_entry(name)\
              b8 driver_create(const Namespace* ns);\
              b8 driver_destroy(const Namespace* ns);\
              \
              static const Kernel *kernel;\
              static Process *process;\
              static const Namespace *ns;\
              \
              VAPI b8 _init_self(Kernel* _kernel, Process* _process) {\
               kernel = _kernel;\
               process = _process;\
               ns = kernel_namespace(kernel, name);\
               if(ns == null) {\
                verror("Failed to create namespace");\
                return false;\
               }\
               return driver_create(ns);\
              }\
              \
              VAPI b8 _destroy_self() {\
               vwarn("Process destroyed");\
               b8 result =  driver_destroy(ns);\
               kernel = null;\
               process = null;\
               ns = null;\
               return result;\
              }
