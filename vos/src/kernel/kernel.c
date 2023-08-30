#include "kernel.h"
#include "containers/darray.h"
#include "core/mem.h"
#include "core/logger.h"
#include "luahost.h"
#include "core/event.h"
#include "core/timer.h"
#include "containers/Map.h"
#include "kernel/asset/asset.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"
// Get the next available ID from the pool.
ProcessID id_pool_next_id();

void id_pool_release_id(ProcessID id);
b8 id_exists_in_stack(IDPool *pool, ProcessID id);

static KernelContext *kernel_context = null;
static b8 kernel_initialized = false;
static Map *processes_by_name = null;

KernelResult kernel_initialize(char *root_path) {
    if (kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_INITIALIZED, null};
        return result;
    }
//    path_move(root_path);
    asset_manager_initialize(root_path);
    vdebug("Root path: %s", root_path)
    initialize_memory();
    kernel_context = kallocate(sizeof(KernelContext), MEMORY_TAG_KERNEL);
    kernel_context->processes = kallocate(sizeof(Process *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    //make sure the processes array is zeroed out
    kzero_memory(kernel_context->processes, sizeof(Process *) * MAX_PROCESSES);
    kernel_context->id_pool = kallocate(sizeof(IDPool), MEMORY_TAG_KERNEL);
    kernel_context->id_pool->top_index = 0;
    kernel_context->id_pool->max_id = 0;
    timer_initialize();
    kernel_initialized = true;
    processes_by_name = dict_create_default();
    event_initialize();
    initialize_syscalls();
    fs_initialize(root_path);
    vdebug("Kernel initialized")
    KernelResult result = {KERNEL_SUCCESS, kernel_context};
    asset_manager_sync();
    return result;
}

Process *kernel_create_process(Asset *script_asset) {
    if (!kernel_initialized) {
        return NULL;
    }
    if (dict_get(processes_by_name, script_asset->path) != null) {
        vwarn("Process already exists with name %s", script_asset->path)
        return null;
    }

    // Get the name of the script by removing the path and extension.
    Process *process = process_create(script_asset);
    ProcessID pid = id_pool_next_id();
    if (pid == MAX_PROCESSES) {
        return NULL;
    }
    process->pid = pid;
    initialize_syscalls_for(process);
    dict_set(processes_by_name, process->script_asset->path, process);
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

KernelResult kernel_lookup_process(ProcessID pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, null};
        return result;
    }
    Process *process = kernel_context->processes[pid];
    if (process == null) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    KernelResult result = {KERNEL_PROCESS_CREATED, process};
    return result;
}

KernelResult kernel_destroy_process(ProcessID pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, null};
        return result;
    }
    Process *process = kernel_context->processes[pid];
    if (process == null) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    dict_remove(processes_by_name, process->script_asset->path);
    vdebug("Destroyed process 0x%04x named %s", pid, process->process_name)
//    kfree(process->script_asset->data, string_length(process->script_asset->data), MEMORY_TAG_VFS);
    kfree(process->script_asset, sizeof(Asset), MEMORY_TAG_VFS);
    process_destroy(process);
    id_pool_release_id(pid);

//    kfree(process, sizeof(Process), MEMORY_TAG_PROCESS);
    KernelResult result = {KERNEL_SUCCESS, null};
    return result;
}

Process *kernel_locate_process(const char *name) {
    if (!kernel_initialized) {
        return null;
    }
    Process *process = dict_get(processes_by_name, name);
    if (process == null) {
        return null;
    }
    return process;
}

KernelResult kernel_shutdown() {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_SHUTDOWN, null};
        return result;
    }
    //TODO: do we need to destroy the processes?

    IDPool *pool = kernel_context->id_pool;
    for (ProcessID i = 1; i <= pool->max_id; i++) {
        if (!id_exists_in_stack(pool, i)) {
            KernelResult process_destroy_result = kernel_destroy_process(i);
            if (!is_kernel_success(process_destroy_result.code))return process_destroy_result;
        }
    }
    asset_manager_shutdown();
    dict_destroy(processes_by_name);
    fs_shutdown();
    kfree(kernel_context->processes, sizeof(Process *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    kfree(kernel_context->id_pool, sizeof(IDPool), MEMORY_TAG_KERNEL);
    kfree(kernel_context, sizeof(KernelContext), MEMORY_TAG_KERNEL);
    kernel_initialized = false;
    KernelResult result = {KERNEL_SUCCESS, null};
    timer_cleanup();
    shutdown_memory();
    shutdown_syscalls();
    event_shutdown();
    vinfo("Mem usuage: %s", get_memory_usage_str())
    return result;
}

ProcessID id_pool_next_id() {
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

void id_pool_release_id(ProcessID id) {
    IDPool *pool = kernel_context->id_pool;
    if (pool->top_index < MAX_PROCESSES) {
        // Push the ID back into the pool.
        pool->ids[pool->top_index++] = id;
    }
    // If the pool overflows (which shouldn't happen in correct usage), you might want to handle the error.
}

// Helper function to check if an ID exists in the IDPool's stack.
b8 id_exists_in_stack(IDPool *pool, ProcessID id) {
    for (u32 i = 0; i < pool->top_index; i++) {
        if (pool->ids[i] == id) {
            return true;
        }
    }
    return false;
}
#pragma clang diagnostic pop