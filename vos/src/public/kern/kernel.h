/**
 * Created by jraynor on 2/24/2024.
 */
#pragma once

#include <ffi.h>

#include "defines.h"

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
} EventData;

/**
 * @brief A function pointer typedef which is used for event subscriptions by the subscriber.
 * @param kernel The kernel that the event is being sent to.
 * @param code The event code to be sent.
 * @param data The event context to be passed with the fired event.
 * @returns True if the message should be considered handled, which means that it will not
 * be sent to any other consumers; otherwise false.
 */
typedef b8 (*KernelEventPFN)(Kernel *kernel, u16 code, EventData data);

/**
 * @brief Enumeration of kernel event codes.
 *
 * The KernelEventCode enumeration defines the different event codes that can be used by the kernel.
 * These event codes represent various system events related to the process management.
 */
typedef enum KernelEventCode {
    /** @brief The minimum event code that can be used internally. */
    EVENT_MIN_CODE = 0x00,
    /** @brief The process started event is used to notify the kernel that a process has started.
    *
    * The EventData for this event should contain the following:
    *  - u16[0] = The process id of the terminated process.
    */
    EVENT_PROCESS_STARTED = 0x01,
    /** @brief The process stopped event is used to notify the kernel that a process has stopped.
    *
    * The EventData for this event should contain the following:
    *  - u16[0] = The process id of the terminated process.
    */
    EVENT_PROCESS_STOPPED = 0x02,
    /** @brief The process paused event is used to notify the kernel that a process has been paused.
     * This happens when a process is temporarily halted and is not ready to continue processing.
     *
     * The EventData for this event should contain the following:
     *  - u16[0] = The process id of the terminated process.
     */
    EVENT_PROCESS_PAUSED = 0x03,
    /** @brief The process resumed event is used to notify the kernel that a process has resumed.
     * This happens after a process has been paused and is ready to continue processing. */
    EVENT_PROCESS_RESUMED = 0x05,
    /** @brief The process terminated event is used to notify the kernel that a process has terminated.
     * This will unload it from the internal process list and named process list
     *
     * The EventData for this event should contain the following:
     *  - u16[0] = The process id of the terminated process.
     */
    EVENT_PROCESS_TERMINATED = 0x06,
    /** @brief The kernel poll event is used to poll the kernel for events.
     *
     * The EventData for this event should contain the following:
     *  - f32[0] = The current delta time in seconds.
     */
    EVENT_KERNEL_RENDER = 0x07,
    /** @brief We reserve the first 255(out of 8192 maxium codes) event codes for system use. */
    EVENT_MAX_CODE = 0xFF
} KernelEventCode;


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
VAPI b8 kernel_destroy(Kernel *kern);

// =====================================================================================================================
// Process Management
// =====================================================================================================================

/**
 * Uses the file system to load a process from a file. The process will be loaded into memory and will be ready to run.
 * @param driver_path The path to the driver file.
 * @return A pointer to the process if it was successfully loaded, otherwise NULL.
 */
VAPI Process *kernel_process_new(const char *driver_path);

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

#define MAX_FUNCTION_ARGS 8

typedef struct FunctionSignature {
    const char *name; // The function name
    FunctionType args[MAX_FUNCTION_ARGS];
    u8 arg_count;
    FunctionType return_type;
} FunctionSignature;

/**
 * Looks up a callback function in a process.
 */
VAPI Function *kernel_process_function_lookup(Process *process, const FunctionSignature signature);

/**
 * Calls a function in a process.
 * @param func The function to call.
 * @param ... The arguments to pass to the function.
 */
VAPI FunctionResult kernel_process_function_call(const Function *func, ...);

/**
 * Starts a process. This will start the process and all child processes.
 */
VAPI b8 kernel_process_run(Process *process);

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
// Event System
// =====================================================================================================================

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
VAPI b8 kernel_event_listen(u16 code, KernelEventPFN on_event);

/**
 * @brief Fires an event in the kernel with the specified code and data.
 *
 * @param kernel The pointer to the kernel object.
 * @param code The code of the event to be fired.
 * @param data The data associated with the event.
 *
 * @return True if the event was successfully fired, false otherwise.
 */
VAPI b8 kernel_event_trigger(u16 code, EventData data);

/**
 * @brief Unregisters a listener function for a specific kernel event code.
 *
 * The listener function will no longer be invoked when the specified kernel event occurs.
 *
 * @param kernel A pointer to the Kernel object.
 * @param code The code of the kernel event to unregister a listener for.
 * @param on_event The listener function to be unregistered.
 * @return Returns true if the listener was successfully unregistered, false otherwise.
 */
VAPI b8 kernel_event_unlisten(u16 code, KernelEventPFN on_event);
