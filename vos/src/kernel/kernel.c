#include "kernel.h"
#include "containers/darray.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "vlua.h"
#include "core/vevent.h"
#include "core/vtimer.h"
#include "containers/dict.h"
#include "core/vstring.h"
#include "filesystem/paths.h"
#include "platform/platform.h"
#include "lauxlib.h"
#include "core/vinput.h"
#include "lualib.h"

// Get the next available ID from the pool.
ProcID id_pool_next_id();
void id_pool_release_id(ProcID id);
b8 id_exists_in_stack(ProcPool *pool, ProcID id);

static Kernel *kernel_context = null;
static b8 kernel_initialized = false;
static Dict *processes_by_name = null;

void kernel_load_drivers(const char *path);

void kernel_unload_drivers();


KernelResult kernel_initialize(char *root_path) {
    if (kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_INITIALIZED, null};
        return result;
    }
    memory_system_configuration config;
    config.heap_size = GIBIBYTES(2);
    if (!memory_system_initialize(config)) {
        verror("Failed to initialize memory system; shutting down.");
        return (KernelResult) {KERNEL_ERROR_OUT_OF_MEMORY, null};
    }
    platform_initialize();
    strings_initialize();
    kernel_context = kallocate(sizeof(Kernel), MEMORY_TAG_KERNEL);
    kernel_context->fs_context = vfs_initialize(root_path);
    initialize_logging();
    vdebug("Root path: %s", root_path)
    
    kernel_context->window_context = kallocate(sizeof(WindowContext), MEMORY_TAG_KERNEL);
    kernel_context->window_context->input_state = kallocate(sizeof(InputState), MEMORY_TAG_KERNEL);
    if (!window_initialize(kernel_context->window_context, "VOS", 1600, 900)) {
        verror("Failed to initialize window context");
        return (KernelResult) {KERNEL_ERROR, null};
    }
    kernel_context->processes = kallocate(sizeof(Proc *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    //make sure the processes array is zeroed out
    kzero_memory(kernel_context->processes, sizeof(Proc *) * MAX_PROCESSES);
    kernel_context->id_pool = kallocate(sizeof(ProcPool), MEMORY_TAG_KERNEL);
    kernel_context->id_pool->top_index = 0;
    kernel_context->id_pool->max_id = 0;
    initialize_timer();
    kernel_initialized = true;
    processes_by_name = dict_new();
    kernel_context->drivers = dict_new();
    event_initialize(kernel_context);
//    intrinsics_initialize();
    kernel_load_drivers(string_concat(root_path, "/drivers"));
    vinfo("Kernel initialized")
    KernelResult result = {KERNEL_SUCCESS, kernel_context};
    return result;
}

KernelResult kernel_shutdown() {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_SHUTDOWN, null};
        return result;
    }
    shutdown_logging();
    //destroy all processes
    ProcPool *pool = kernel_context->id_pool;
    for (ProcID i = 1; i <= pool->max_id; i++) {
        if (!id_exists_in_stack(pool, i)) {
            KernelResult process_destroy_result = kernel_destroy_process(i);
            if (!kernel_is_result_success(process_destroy_result.code))return process_destroy_result;
        }
    }
    kernel_unload_drivers();
    event_shutdown(kernel_context);
    //free the kernels associated memory
    kfree(kernel_context->processes, sizeof(Proc *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    kfree(kernel_context->id_pool, sizeof(ProcPool), MEMORY_TAG_KERNEL);
    kfree(kernel_context, sizeof(Kernel), MEMORY_TAG_KERNEL);
    
    //shutdown the vfs
    vfs_shutdown(kernel_context->fs_context);
    kernel_initialized = false;
    timer_cleanup();
//    intrinsics_shutdown();
    dict_delete(processes_by_name);
    strings_shutdown();
    platform_shutdown();
    vtrace("Mem usage: %s", get_memory_usage_str())
    memory_system_shutdown();
    KernelResult result = {KERNEL_SUCCESS, null};
    return result;
}

Proc *kernel_create_process(FsNode *script_node_file) {
    if (!kernel_initialized) return null;
    if (!script_node_file) {
        vwarn("Attempted to create process with null script node")
        return null;
    }
    // Check if a process with the same name already exists.
    if (dict_get(processes_by_name, script_node_file->path) != null) {
        vwarn("Process already exists with name %s", script_node_file->path)
        return null;
    }
    
    // Get the name of the script by removing the path and extension.
    Proc *process = process_create(script_node_file);
    ProcID pid = id_pool_next_id();
    if (pid == MAX_PROCESSES) {
        vwarn("Maximum number of processes reached")
        return null;
    }
    process->pid = pid;
    process->lua_state = luaL_newstate();
    luaL_openlibs(process->lua_state);
//    intrinsics_install_to(process);
    // for now we just install all loaded drivers to each process.
    // In the future, a process should be able to specify which drivers it wants to use.
    dict_for_each(kernel_context->drivers, DriverState, {
        value->driver->install(*process);
        vinfo("Installed driver %s to process %s", key, process->process_name)
    });
    dict_set(processes_by_name, process->source_file_node->path, process);
    kernel_context->processes[pid] = process;
    vdebug("Created process 0x%04x named %s", pid, process->process_name)
    process->kernel = kernel_context;
    return process;
}

//TODO: here we should do some kind of thread watch dog to make sure the process is still running and all
b8 kernel_poll_update() {
    if (!kernel_initialized) {
        vwarn("Attempted to poll kernel before initialization");
        return false;
    }
    timer_poll();
    return true;
}

Proc *kernel_lookup_process(Kernel *kernel, ProcID pid) {
    vinfo("Looking up process 0x%04x", pid);
    if (!kernel || !kernel->processes) {
        verror("Attempted to lookup process before kernel initialization")
        return null;
    }
    Proc *process = kernel->processes[pid];
    if (process == null) {
        return null;
    }
    return process;
}

KernelResult kernel_destroy_process(ProcID pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, null};
        return result;
    }
    Proc *process = kernel_context->processes[pid];
    if (process == null) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    dict_remove(processes_by_name, process->source_file_node->path);
    vdebug("Destroyed process 0x%04x named %s", pid, process->process_name)
    process_destroy(process);
    id_pool_release_id(pid);
    KernelResult result = {KERNEL_SUCCESS, null};
    return result;
}

//TODO more advanced process lookup with wildcards and such
ProcID *kernel_lookup_process_id(const char *name) {
    if (!kernel_initialized) {
        vtrace("Attempted to locate process before initialization")
        return null;
    }
    Proc *process = dict_get(processes_by_name, name);
    if (process == null) {
        vtrace("Process not found with name %s", name)
        return null;
    }
    return &process->pid;
}

ProcID id_pool_next_id() {
    ProcPool *pool = kernel_context->id_pool;
    // Check if we are overflowing the pool.
    if (pool->max_id == MAX_PROCESSES) {
        // If we are overflowing the pool, return the top ID.
        return pool->max_id;
    } else if (pool->top_index > 0) {
        // Pop an ID from the stack.
        return pool->ids[--pool->top_index];
    } else {
        // If the pool is empty, generate a new ID.
        return ++pool->max_id;
    }
    
}

void id_pool_release_id(ProcID id) {
    ProcPool *pool = kernel_context->id_pool;
    if (pool->top_index < MAX_PROCESSES) {
        // Push the ID back into the pool.
        pool->ids[pool->top_index++] = id;
    }
    // If the pool overflows (which shouldn't happen in correct usage), you might want to handle the error.
}

// Helper function to check if an ID exists in the IDPool's stack.
b8 id_exists_in_stack(ProcPool *pool, ProcID id) {
    for (u32 i = 0; i < pool->top_index; i++) {
        if (pool->ids[i] == id) {
            return true;
        }
    }
    return false;
}

// returns a darray of all the drivers in the given path
b8 kernel_collect_drivers(const char *path, Dict *drivers) {
    if (!kernel_initialized) {
        vwarn("Attempted to load drivers before kernel initialization")
        return false;
    }
    // Collects all drivers in the given path, then loads them
    VFilePathList *files = platform_collect_files_direct(path);
    if (files == null) {
        vwarn("No drivers found in path %s", path)
        return false;
    }
    const char *extension = platform_dynamic_library_extension();
    // Load each driver
    for (u32 i = 0; i < files->count; i++) {
        char *file = files->paths[i];
        if (string_ends_with(file, extension)) {
            // Gets the name of the driver without hte platform extension
            char *driver_name = path_file_name(file);
            char *relative_path = path_relative(file);
            // Load the driver
            FsNode *driver_node = vfs_node_get(kernel_context->fs_context, relative_path);
            if (driver_node == null) {
                vwarn("Failed to load driver %s", file)
                continue;
            }
            dict_set(drivers, driver_name, driver_node);
            vdebug("Discovered driver %s", driver_name)
        }
    }
    return true;
}


b8 kernel_create_drivers(Dict *drivers) {
    if (drivers == null) {
        vwarn("No drivers to initialize")
        return false;
    }
    kernel_context->drivers = dict_new();
    //Iterate through the drivers and load them
    dict_for_each(drivers, DriverState, {
        DynamicLibrary *library = value->handle;
        if (!platform_dynamic_library_load_function("create_driver", library)) {
            vwarn("Failed to initialize driver %s", key)
            return false;
        } else {
            dynamic_library_function *dlf = dict_get(library->functions, "create_driver");
            if (dlf == null) {
                vwarn("Failed to find create_driver function in driver %s", key)
                return false;
            }
            pfn_driver_load create_driver = (pfn_driver_load) dlf->pfn;
            Driver init = create_driver(kernel_context);
            value->driver = kallocate(sizeof(Driver), MEMORY_TAG_KERNEL);
            *value->driver = init;
            char *name = init.name ? init.name : key;
            value->driver_name = name;
            dict_set(kernel_context->drivers, name, value);
            init.load();
            vdebug("Initialized driver %s", name)
        }
    });
    dict_delete(drivers);
    return true;
}

void kernel_load_drivers(const char *path) {
    if (!kernel_initialized) {
        vwarn("Attempted to load drivers before kernel initialization")
        return;
    }
    Dict *collected_drivers = dict_new();
    Dict *loaded_drivers = dict_new();
    if (!kernel_collect_drivers(path, collected_drivers)) {
        vwarn("Failed to load drivers from path %s", path)
    }
    
    //Iterate through the drivers and load them
    dict_for_each(collected_drivers, FsNode, {
        DynamicLibrary *library = kallocate(sizeof(DynamicLibrary), MEMORY_TAG_KERNEL);
        FsPath absolute_path = path_absolute(value->path);
        absolute_path = path_to_platform(absolute_path);
        if (!platform_dynamic_library_load(absolute_path, library)) {
            vwarn("Failed to load driver %s", absolute_path)
            kfree(library, sizeof(DynamicLibrary), MEMORY_TAG_KERNEL);
            continue;
        } else {
            DriverState *driver = kallocate(sizeof(DriverState), MEMORY_TAG_KERNEL);
            driver->handle = library;
            driver->driver_name = key;
            driver->driver_file = value;
            driver->driver = kallocate(sizeof(Driver), MEMORY_TAG_KERNEL);
            dict_set(loaded_drivers, key, driver);
        }
    });
    dict_delete(collected_drivers);
    if (!kernel_create_drivers(loaded_drivers)) {
        vwarn("Failed to initialize drivers")
    }
}

void kernel_unload_drivers() {
    if (!kernel_initialized) {
        vwarn("Attempted to unload drivers before kernel initialization")
        return;
    }
    //Iterate through the drivers and unload them
    dict_for_each(kernel_context->drivers, DriverState, {
        value->driver->unload();
        DynamicLibrary *library = value->handle;
        if (!platform_dynamic_library_unload(library)) {
            vwarn("Failed to unload driver %s", key)
        }
        kfree(library, sizeof(DynamicLibrary), MEMORY_TAG_KERNEL);
        kfree(value, sizeof(DriverState), MEMORY_TAG_KERNEL);
        vdebug("Unloaded driver %s", key)
    });
    dict_delete(kernel_context->drivers);
}
