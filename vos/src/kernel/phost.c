#include <lauxlib.h>
#include <lualib.h>
#include "phost.h"

#include "core/mem.h"
#include "core/logger.h"
#include "core/str.h"

#include "platform/platform.h"

ProcessView *create_process_view(u32 initial_process_count) {
    ProcessView *view = kallocate(sizeof(ProcessView), MEMORY_TAG_PROCESS);
    view->processes = kallocate(sizeof(Process) * initial_process_count, MEMORY_TAG_PROCESS);
    view->count = 0;
    view->capacity = initial_process_count * 2;
    return view;
}

void destroy_process_view(ProcessView *view) {
    kfree(view->processes, sizeof(Process) * view->capacity, MEMORY_TAG_PROCESS);
    kfree(view, sizeof(ProcessView), MEMORY_TAG_PROCESS);
}

void resize_process_view(ProcessView *view, u32 new_capacity) {
    Process **new_processes =
            kreallocate(view->processes, sizeof(Process *) * new_capacity,
                        MEMORY_TAG_PROCESS); // It's a pointer to Process
    if (new_processes == NULL) {
        vwarn("Failed to resize the process view.");
        return;
    }
    view->processes = *new_processes;
    view->capacity = new_capacity;
}

static const char *process_state_strings[PROCESS_STATE_MAX_STATES] = {
        "STOPPED",
        "RUNNING",
        "PAUSED",
        "TERMINATED"
};

static ProcessID next_process_id = 0;

/**
 * Creates a new process. This will parse the script and create a new lua_State for the process.
 */
Process *process_create(const char *script_path) {
    Process *process = kallocate(sizeof(Process), MEMORY_TAG_PROCESS);
    process->script_path = string_duplicate(script_path);
    process->state = PROCESS_STATE_STOPPED;
    process->lua_state = luaL_newstate();

    // TODO: Create a process ID pool
    process->process_id = next_process_id++;
    process->child_context = create_process_view(INITIAL_NESTED_PROCESSES);
    luaL_openlibs(process->lua_state);
    return process;
}

b8 process_add_child(Process *parent, Process *child) {
// Make the child's lua_State a copy of the parent's lua_State
    child->lua_state = parent->lua_state;

    if (parent->child_context->count == parent->child_context->capacity) {
        // Double the capacity
        u32 new_capacity = parent->child_context->capacity * 2;
        resize_process_view(parent->child_context, new_capacity);
    }

    // Add the new child to the processes array and increment the count
    parent->child_context->processes[parent->child_context->count++] = *child;
    return TRUE;
}

b8 process_remove_child(Process *parent, ProcessID child_id) {
    // Find the child in the processes array
    for (u32 i = 0; i < parent->child_context->count; ++i) {
        if (parent->child_context->processes[i].process_id == child_id) {
            Process *child = &parent->child_context->processes[i];
            // Swap the child with the last child in the array
            parent->child_context->processes[i] = parent->child_context->processes[parent->child_context->count - 1];
            // Decrement the count
            parent->child_context->count--;
            // Destroy the child
            process_destroy(child);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Destroys a process. This will stop the process and free all memory.
 */
void process_destroy(Process *process) {
    // Stop the process
    process_stop(process, TRUE, TRUE);

    // Destroy the child context
    destroy_process_view(process->child_context);

    // Free the script path
    kfree((void *) process->script_path, string_length(process->script_path) + 1, MEMORY_TAG_PROCESS);

    // Free the process
    kfree(process, sizeof(Process), MEMORY_TAG_PROCESS);
}

/**
 * Starts a process. This will start the process and all child processes.
 */
b8 process_start(Process *process) {
    vinfo("Process %d started", process->process_id);
    //run the lua source file
    process->state = PROCESS_STATE_RUNNING;

    if (luaL_dofile(process->lua_state, process->script_path) != LUA_OK) {
        verror("Failed to run script %s", process->script_path);
        process->state = PROCESS_STATE_STOPPED;
        return FALSE;
    }
    // iterate through all the children and start them
    for (u32 i = 0; i < process->child_context->count; ++i) {
        Process *child = &process->child_context->processes[i];
        process_start(child);
    }
    return TRUE;
}

/**
 * Stops a process. This will stop the process and all child processes.
 * @param process The process to stop.
 * @param force If TRUE, the process will be stopped immediately. If FALSE, the process will be stopped gracefully.
 * @param kill_children If TRUE, all child processes will be stopped immediately. If FALSE, child processes will be stopped gracefully.
 */
b8 process_stop(Process *process, b8 force, b8 kill_children) {
    vinfo("Process %d stopped", process->process_id);
    return TRUE;
}