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
#include "kresult.h"
#include "vfs.h"

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
  // The virtual file system is used to load lua scripts.
  Vfs *vfs;
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
 * Initializes the kernel. This will allocate the kernel context and initialize the root process view.
 * This will always load the process at assets/scripts/main.lua as the root process.
 * We will keep a static pointer to the kernel context. This will allow us to access the kernel context from
 * anywhere in the application. There will only ever be one kernel context.
 *
 * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
 */
KernelResult kernel_initialize_from(Node *root_node);

/**
 * Creates a new process from a lua script. This will parse the script and create a new lua_State for the process.
 *
 * @param script_path The path to the lua script.
 * @return KERNEL_SUCCESS if the function was successfully registered along with a pointer to the process id, else an error code.
 */
KernelResult kernel_create_process( char *script_path);

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

