#include <stdio.h>
#include <string.h>
#include "kernel.h"
#include "containers/darray.h"
#include "core/mem.h"
#include "core/logger.h"
#include "pthread.h"
#include "core/str.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"
// Get the next available ID from the pool.
ProcessID id_pool_next_id();

void id_pool_release_id(ProcessID id);
b8 id_exists_in_stack(IDPool *pool, ProcessID id);
char *id_to_string(ProcessID id);

static KernelContext *kernel_context = NULL;
static b8 kernel_initialized = FALSE;

/**
 * Determines if the kernel operation was successful.
 * @param code The error code.
 * @return TRUE if the operation was successful, else FALSE.
 */
b8 is_kernel_success(KernelCode code) {
    return code >= KERNEL_SUCCESS;
}

/**
 * Gets the result message for a kernel operation.
 * @param result The result of the kernel operation.
 * @return
 */
const char *get_kernel_result_message(KernelResult result) {
    char *message = kallocate(256, MEMORY_TAG_KERNEL);
    switch (result.code) {
        case KERNEL_SUCCESS:sprintf(message, "The kernel operation was successful.");
            break;
        case KERNEL_ALREADY_INITIALIZED:sprintf(message, "The kernel has already been initialized.");
            break;
        case KERNEL_ALREADY_SHUTDOWN:sprintf(message, "The kernel has already been shutdown.");
            break;
        case KERNEL_CALL_BEFORE_INIT:sprintf(message, "The kernel has not been initialized.");
            break;
        case KERNEL_PROCESS_CREATED:sprintf(message, "The process was successfully created.");
            break;
        case KERNEL_PROCESS_NOT_FOUND:
            sprintf(message,
                    "The process with id %d was not found.",
                    (ProcessID) result.data);
            break;
        case KERNEL_ERROR:sprintf(message, "Kernel error: %s", (char *) result.data);
            break;
        default:sprintf(message, "An unknown kernel error occurred.");
            break;
    }
    return message;
}

KernelResult kernel_initialize() {
    if (kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_INITIALIZED, NULL};
        return result;
    }

    kernel_context = kallocate(sizeof(KernelContext), MEMORY_TAG_KERNEL);
    kernel_context->processes = kallocate(sizeof(Process *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
    //make sure the processes array is zeroed out
    kzero_memory(kernel_context->processes, sizeof(Process *) * MAX_PROCESSES);
    kernel_context->id_pool = kallocate(sizeof(IDPool), MEMORY_TAG_KERNEL);
    kernel_context->id_pool->top_index = 0;
    kernel_context->id_pool->max_id = 0;
    kernel_initialized = TRUE;
    KernelResult result = {KERNEL_SUCCESS, NULL};
    return result;
}

KernelResult kernel_create_process(const char *script_path) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, NULL};
        return result;
    }

    // Get the name of the script by removing the path and extension.
    i32 count = string_split_count(script_path, "/");
    char *script_name = string_split_at(script_path, "/", count - 1);
    vdebug("Creating process for script %s", script_name)
    Process *process = process_create(script_path);
    ProcessID pid = id_pool_next_id();
    if (pid == MAX_PROCESSES) {
        KernelResult result = {KERNEL_ID_POOL_OVERFLOW, (void *) pid};
        return result;
    }
    process->pid = pid;
    kernel_context->processes[pid] = process;
    KernelResult result = {KERNEL_PROCESS_CREATED, (void *) pid};
    return result;
}

KernelResult kernel_attach_process(ProcessID pid, ProcessID parent_pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, NULL};
        return result;
    }
    Process *process = kernel_context->processes[pid];
    if (process == NULL) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    Process *parent_process = kernel_context->processes[parent_pid];
    if (parent_process == NULL) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) parent_pid};
        return result;
    }
    process_add_child(parent_process, process);
    vdebug("Attached process 0x%04x to process 0x%04x", pid, parent_pid)
    KernelResult result = {KERNEL_SUCCESS, NULL};
    return result;
}

KernelResult kernel_lookup_process(ProcessID pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, NULL};
        return result;
    }
    Process *process = kernel_context->processes[pid];
    if (process == NULL) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    KernelResult result = {KERNEL_PROCESS_CREATED, process};
    return result;
}
KernelResult kernel_destroy_process(ProcessID pid) {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_CALL_BEFORE_INIT, NULL};
        return result;
    }
    Process *process = kernel_context->processes[pid];
    if (process == NULL) {
        KernelResult result = {KERNEL_PROCESS_NOT_FOUND, (void *) pid};
        return result;
    }
    process_destroy(process);
    id_pool_release_id(pid);
    KernelResult result = {KERNEL_SUCCESS, NULL};
    vdebug("Destroyed process 0x%04x", pid)
    return result;
}

KernelResult kernel_shutdown() {
    if (!kernel_initialized) {
        KernelResult result = {KERNEL_ALREADY_SHUTDOWN, NULL};
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
    kfree(kernel_context->processes, sizeof(Process *) * MAX_PROCESSES, MEMORY_TAG_KERNEL);
//    darray_destroy(kernel_context->processes);
    kfree(kernel_context->id_pool, sizeof(IDPool), MEMORY_TAG_KERNEL);
    kfree(kernel_context, sizeof(KernelContext), MEMORY_TAG_KERNEL);
    kernel_initialized = FALSE;
    KernelResult result = {KERNEL_SUCCESS, NULL};
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
            return TRUE;
        }
    }
    return FALSE;
}
#pragma clang diagnostic pop