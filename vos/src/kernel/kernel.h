/**
 * The kernel allows for multi-threaded execution of lua scripts. Currently, lua scripts are executed from the top down.
 * When a root process is excuted, it's children recursively execute on the same thread. This allows for a single lua
 * state to be shared between all processes. This is useful for sharing data between processes. For example, a process
 * that is responsible for rendering the scene can share a lua state with a process that is responsible for updating
 * the scene. This allows the render process to access the scene data without having to copy the data to a shared
 * memory location. This also allows for the render process to be updated without having to restart the entire
 * application.
 */
#pragma once

#include "defines.h"
#include "phost.h"

//The maximum number of processes that can be created.
#define MAX_PROCESSES 10000
/**
 * The id pool will be internally used to track the next available process id. This will allow for the reuse of
 * process ids.
 */
typedef struct IDPool {
  ProcessID ids[MAX_PROCESSES];
  u32 top_index; // For stack operations, indicates the top of the stack.
  ProcessID max_id; // The highest ID ever generated.
} IDPool;

/**
 * This is used to mark the result of a kernel operation.
 * All error codes are > KERNEL_SUCCESS.
 */
typedef enum KernelCode {
  /**
   * The kernel id pool has overflowed.
   * @data the max id that was generated (u32)
   */
  KERNEL_ID_POOL_OVERFLOW = -6,
  /**
   * The kernel has already been initialized.
   * @data no data is returned
   */
  KERNEL_ALREADY_INITIALIZED = -5,
  /**
   * The kernel has already been shutdown.
   * @data no data is returned
   */
  KERNEL_ALREADY_SHUTDOWN = -4,
  /**
   * The kernel has not been initialized.
   * @data no data is returned
   */
  KERNEL_CALL_BEFORE_INIT = -3,
  /**
   * The process was not found.
   * @data a pointer to the process id that was not found
   */
  KERNEL_PROCESS_NOT_FOUND = -2,
  /**
   * The operation failed due to a lack of memory.
   * @data total memory used
   */
  KERNEL_ERROR_OUT_OF_MEMORY = -1,

  /**
   * A general kernel error occurred.
   * @data a general error message
   */
  KERNEL_ERROR = 0,

  /**
   * The operation was successful. This allows KernelCode to be used as a boolean. 0 is SUCCESS, all other values are errors.
   * @data no data is returned
   */
  KERNEL_SUCCESS = 1,

  /**
   * A kernel process was successfully created.
   * This is a success code.
   * @data A pointer to the process id.
   */
  KERNEL_PROCESS_CREATED = 2,

} KernelCode;

/**
 * Allows us to return a result from a kernel operation.
 * @param code The error code.
 * @param data The data to return.
 */
typedef struct KernelResult {
  // The error code of the operation.
  KernelCode code;
  // Can be used to return data from a kernel operation, may be NULL depending on the operation.
  void *data;
} KernelResult;

/**
 * Determines if the kernel operation was successful.
 * @param code The error code.
 * @return TRUE if the operation was successful, else FALSE.
 */
b8 is_kernel_success(KernelCode code);

/**
 * Gets the result message for a kernel operation.
 * @param result The result of the kernel operation.
 * @return
 */
const char *get_kernel_result_message(KernelResult result);

/**
 * A process context is used to store the state of a process and it's thread.
 */
typedef struct ProcessContext {
  // The process id.
  Process *process;
  // the thread id.
} ProcessContext;

/**
 * Internally store the state of the kernel.
 */
typedef struct KernelContext {
  // The root process view is the root of the process tree. This is the first process that is executed.
  Process **processes;
  // The id pool is used to track the next available process id.
  IDPool *id_pool;
} KernelContext;

/**
 * Initializes the kernel. This will allocate the kernel context and initialize the root process view.
 * This will always load the process at assets/scripts/main.lua as the root process.
 * We will keep a static pointer to the kernel context. This will allow us to access the kernel context from
 * anywhere in the application. There will only ever be one kernel context.
 *
 * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
 */
KernelResult kernel_initialize();

/**
 * Creates a new process from a lua script. This will parse the script and create a new lua_State for the process.
 *
 * @param script_path The path to the lua script.
 * @return KERNEL_SUCCESS if the function was successfully registered along with a pointer to the process id, else an error code.
 */
KernelResult kernel_create_process(const char *script_path);


/**
 * Attaches a process to a parent process. This will add the child process to the parent's child process array.
 *
 * @param pid the process id to attach
 * @param parent_pid  the parent process id to attach to
 * @return KERNEL_SUCCESS if the function was successfully registered along with a pointer to the process id, else an error code.
 */
KernelResult kernel_attach_process(ProcessID pid, ProcessID parent_pid);

/**
 * Looks up a process by id. The KernelResult will contain a pointer to the process if it was found.
 * @param pid The process id.
 * @return KERNEL_SUCCESS if the function was successfully registered along with a pointer to the process, else an error code.
 */
KernelResult kernel_lookup_process(ProcessID pid);
/**
 * Destroys a process. This will stop the process and free all memory. It also frees the id from the id pool.
 * @param pid  The process id.
 * @return  KERNEL_SUCCESS if the function was successfully registered, else an error code.
 */
KernelResult kernel_destroy_process(ProcessID pid);
/**
 * Destroys the kernel. This will deallocate the kernel context and destroy the root process view.
 * @param context The kernel context.
 * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
 */
KernelResult kernel_shutdown();

/**
 * Registers a system function with the kernel. This will allow the function to be called from a lua script.
 * @param name The name of the function.
 * @param function The function to register.
 * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
 */
KernelResult kernel_register_function(const char *name, lua_CFunction function);

