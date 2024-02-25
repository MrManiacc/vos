// /**
//  * The kernel allows for multi-threaded execution of lua scripts. Currently, lua scripts are executed from the top down.
//  * When a root process is excuted, it's children recursively execute on the same thread. This allows for a single lua
//  * state to be shared between all processes. This is useful for sharing data between processes. For example, a process
//  * that is responsible for rendering the scene can share a lua state with a process that is responsible for updating
//  * the scene. This allows the render process to access the scene data without having to copy the data to a shared
//  * memory location. This also allows for the render process to be updated without having to restart the entire
//  * application.
//  */
// #pragma once
//
// #include "defines.h"
// #include "proc.h"
// #include "vresult.h"
// #include "core/vgui.h"
//
// //The maximum number of processes that can be created.
// #ifndef MAX_PROCESSES
// #define MAX_PROCESSES 512 //does this need to be dynamic?
// #endif
//
// typedef u32 DriverID;
//
// /**
//  * The id pool will be internally used to track the next available process id. This will allow for the reuse of
//  * process ids.
//  */
// typedef struct ProcPool {
//     ProcID ids[MAX_PROCESSES];
//     u32 top_index; // For stack operations, indicates the top of the stack.
//     ProcID max_id; // The highest ID ever generated.
// } ProcPool;
//
//
// /**
//  * Internally store the state of the kernel.
//  */
// typedef struct Kernel {
//     // The root process view is the root of the process tree. This is the first process that is executed.
//     Proc **processes;
//     // The id pool is used to track the next available process id.
//     ProcPool *id_pool;
//     // A dictionary of all registered drivers. They are indexed by their name.
//     Dict *drivers;
//     // The event state is used to track the state of the event system.
//     struct KernelEventState *event_state;
//     // The root window of the application.
//     WindowContext *window_context;
//     // The file system context.
//     FSContext *fs_context;
//
//
// } Kernel;
//
//
// typedef struct Driver {
//     //The driver load pfn
//     b8 (*load)();
//     // Install the driver to a process
//     b8 (*install)(Proc process);
//     //The driver unload pfn
//     b8 (*unload)();
//     // Can define a name for the driver, by default it's given the filename without the extension
//     char *name;
// } Driver;
//
// typedef Driver (*pfn_driver_load)(Kernel *kernel);
//
//
// typedef struct DriverState {
//     // Unique driver ID
//     DriverID did;
//     // Name of the process
//     const char *driver_name;
//     // Path to the driver (shared library)
//     FsNode *driver_file;
//     // The library handle
//     Driver *driver;
//     // The library handle
//     struct DynLib *handle;
// } DriverState;
//
// /**
//  * Initializes the kernel. This will allocate the kernel context and initialize the root process view.
//  * This will always load the process at assets/scripts/main.lua as the root process.
//  * We will keep a static pointer to the kernel context. This will allow us to access the kernel context from
//  * anywhere in the application. There will only ever be one kernel context.
//  * @param root_path The path to the root directory.
//  * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
//  */
// KernelResult kernel_initialize(char *root_path);
//
// /**
//  * Creates a new process from a lua script. This will parse the script and create a new lua_State for the process.
//  *
//  * @param script_path The path to the lua script.
//  * @return KERNEL_SUCCESS if the function was successfully registered along with a pointer to the process id, else an error code.
//  */
// Proc *kernel_create_process(FsNode *script_node_file, b8 should_watch);
//
//
// /**
//  * Looks up a process by id. The KernelResult will contain a pointer to the process if it was found.
//  * @param pid The process id.
//  * @return KERNEL_SUCCESS if the function was successfully registered along with a pointer to the process, else an error code.
//  */
// Proc *kernel_lookup_process(Kernel *kernel, ProcID pid);
//
// /**
//  * Destroys a process. This will stop the process and free all memory. It also frees the id from the id pool.
//  * @param pid  The process id.
//  * @return  KERNEL_SUCCESS if the function was successfully registered, else an error code.
//  */
// KernelResult kernel_destroy_process(ProcID pid);
//
// /**
//  * Updates the kernel. This will update the root process view.
//  * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
//  */
// b8 kernel_poll_update(Kernel *kernel);
//
// /**
//  * Locates a process by name.
//  * @param name The name of the process.
//  * @return A pointer to the process if it was found, otherwise NULL.
//  */
// ProcID *kernel_lookup_process_id(const char *name);
//
// /**
//  * Destroys the kernel. This will deallocate the kernel context and destroy the root process view.
//  * @param context The kernel context.
//  * @return KERNEL_SUCCESS if the function was successfully registered, else an error code.
//  */
// KernelResult kernel_shutdown();
