#include "kernel.h"
#include "containers/darray.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "luahost.h"
#include "core/vevent.h"
#include "core/vtimer.h"
#include "containers/dict.h"
#include "core/vstring.h"
#include "kernel/vfs/paths.h"

// Get the next available ID from the pool.
procid id_pool_next_id();

void id_pool_release_id(procid id);

b8 id_exists_in_stack(IDPool *pool, procid id);

static KernelContext *kernel_context = null;
static b8 kernel_initialized = false;
static dict *processes_by_name = null;

KernelResult kernel_initialize(char *root_path) {
    if (kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_INITIALIZED, null};
        return result;
    }
    // Memory system must be the first thing to be stood up.
    memory_system_configuration memory_system_config = {};
    memory_system_config.total_alloc_size = GIBIBYTES(2);
    if (!memory_system_initialize(memory_system_config)) {
        verror("Failed to initialize memory system; shutting down.");
        return (KernelResult) {KERNEL_ERROR_OUT_OF_MEMORY, null};
    }
    strings_initialize();
    vfs_initialize(root_path);
    initialize_logging();
//    initialize_asset_manager(root_path);
    vdebug("Root path: %s", root_path)
    
    kernel_context = kallocate(sizeof(KernelContext), MEMORY_TAG_KERNEL);
    kernel_context->processes = kallocate(sizeof(proc *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    //make sure the processes array is zeroed out
    kzero_memory(kernel_context->processes, sizeof(proc *) * MAX_PROCESSES);
    kernel_context->id_pool = kallocate(sizeof(IDPool), MEMORY_TAG_KERNEL);
    kernel_context->id_pool->top_index = 0;
    kernel_context->id_pool->max_id = 0;
    initialize_timer();
    kernel_initialized = true;
    processes_by_name = dict_create_default();
    event_initialize();
    intrinsics_initialize();
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
    
    //TODO: do we need to destroy the processes?
    
    IDPool *pool = kernel_context->id_pool;
    for (procid i = 1; i <= pool->max_id; i++) {
        if (!id_exists_in_stack(pool, i)) {
            KernelResult process_destroy_result = kernel_destroy_process(i);
            if (!is_kernel_success(process_destroy_result.code))return process_destroy_result;
        }
    }
    vfs_shutdown();
    kfree(kernel_context->processes, sizeof(proc *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    kfree(kernel_context->id_pool, sizeof(IDPool), MEMORY_TAG_KERNEL);
    kfree(kernel_context, sizeof(KernelContext), MEMORY_TAG_KERNEL);
    kernel_initialized = false;
    KernelResult result = {KERNEL_SUCCESS, null};
    timer_cleanup();
    intrinsics_shutdown();
    event_shutdown();
    dict_destroy(processes_by_name);
    strings_shutdown();
//    vtrace("Mem usage: %s", get_memory_usage_str())
    memory_system_shutdown();
    return result;
}

proc *kernel_create_process(fs_node *script_node_file) {
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
    proc *process = process_create(script_node_file);
    procid pid = id_pool_next_id();
    if (pid == MAX_PROCESSES) {
        vwarn("Maximum number of processes reached")
        return null;
    }
    process->pid = pid;
    install_lua_intrinsics(process);
    dict_set(processes_by_name, process->script_file_node->path, process);
    kernel_context->processes[pid] = process;
    vdebug("Created process 0x%04x named %s", pid, process->process_name)
    return process;
}

b8 kernel_poll_update() {
    if (!kernel_initialized) {
        vwarn("Attempted to poll kernel before initialization");
        return false;
    }
    timer_poll();
}

KernelResult kernel_lookup_process(procid pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, null};
        return result;
    }
    proc *process = kernel_context->processes[pid];
    if (process == null) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    KernelResult result = {KERNEL_PROCESS_CREATED, process};
    return result;
}

KernelResult kernel_destroy_process(procid pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, null};
        return result;
    }
    proc *process = kernel_context->processes[pid];
    if (process == null) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    dict_remove(processes_by_name, process->script_file_node->path);
    vdebug("Destroyed process 0x%04x named %s", pid, process->process_name)
//    kfree(process->script_asset->data, process->script_asset->size, MEMORY_TAG_VFS);
//    kfree(process->script_asset, sizeof(Asset), MEMORY_TAG_ASSET);
    process_destroy(process);
    id_pool_release_id(pid);

//    kfree(process, sizeof(Process), MEMORY_TAG_PROCESS);
    KernelResult result = {KERNEL_SUCCESS, null};
    return result;
}

//TODO more advanced process lookup with wildcards and such
proc *kernel_locate_process(const char *name) {
    if (!kernel_initialized) {
        vtrace("Attempted to locate process before initialization")
        return null;
    }
    proc *process = dict_get(processes_by_name, name);
    if (process == null) {
        return null;
    }
    return process;
}


procid id_pool_next_id() {
    IDPool *pool = kernel_context->id_pool;
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

void id_pool_release_id(procid id) {
    IDPool *pool = kernel_context->id_pool;
    if (pool->top_index < MAX_PROCESSES) {
        // Push the ID back into the pool.
        pool->ids[pool->top_index++] = id;
    }
    // If the pool overflows (which shouldn't happen in correct usage), you might want to handle the error.
}

// Helper function to check if an ID exists in the IDPool's stack.
b8 id_exists_in_stack(IDPool *pool, procid id) {
    for (u32 i = 0; i < pool->top_index; i++) {
        if (pool->ids[i] == id) {
            return true;
        }
    }
    return false;
}
