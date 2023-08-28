#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include "phost.h"

#include "core/mem.h"
#include "core/logger.h"
#include "core/str.h"

#include "platform/platform.h"
#include "containers/darray.h"

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
Process *process_create(Asset *script_asset) {
    Process *process = kallocate(sizeof(Process), MEMORY_TAG_PROCESS);
    process->script_asset = script_asset;
    process->state = PROCESS_STATE_STOPPED;
    process->children_pids = darray_create(ProcessID);
    Path path = process->script_asset->path;
    //get the name by getting the last part of the path after the last slash
    char *name = string_split_at(path, "/", string_split_count(path, "/") - 1);
    //remove the extension
    name = string_split_at(name, ".", 0);
    process->process_name = name;
    return process;
}

b8 process_add_child(Process *parent, Process *child) {
// Make the child's lua_State a copy of the parent's lua_State
    child->lua_state = parent->lua_state;
    darray_push(parent->children_pids, child->pid)
    return true;
}

b8 process_remove_child(Process *parent, ProcessID child_id) {
    u64 length = darray_length(parent->children_pids);
    for (u64 i = 0; i < length; ++i) {
        if (parent->children_pids[i] == child_id) {
            ProcessID child_pid;
            darray_pop_at(parent->children_pids, i, &child_pid);
            vdebug("Child process %d removed from parent process %d", child_pid, parent->pid);
            return true;
        }
    }

    return false;
}

/**
 * Destroys a process. This will stop the process and free all memory.
 */
void process_destroy(Process *process) {
    // Stop the process
    process_stop(process, true, true);
    darray_destroy(process->children_pids)

    // Free the script path
//    kfree((void *) process->process_name, string_length(process->script_path) + +1, MEMORY_TAG_STRING);
//    kfree((void *) process->script_path, string_length(process->script_path) + 1, MEMORY_TAG_STRING);

    // Free the process
    lua_close(process->lua_state);
    kfree(process, sizeof(Process), MEMORY_TAG_PROCESS);
}

/**
 * Starts a process. This will start the process and all child processes.
 */
b8 process_start(Process *process) {
    vinfo("Process %d started", process->pid);
    //run the lua source file
    process->state = PROCESS_STATE_RUNNING;
    //    process->process_name =
    Asset *asset = process->script_asset;
    if (asset == NULL) {
        verror("Failed to load script %s", process->pid);
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    const char *source = asset->data;
    if (source == NULL) {
        verror("Failed to load script %s", process->pid);
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    if (luaL_dostring(process->lua_state, source) != LUA_OK) {
        verror("Failed to run script %d", process->pid);
        //print the error
        verror("Error: %s", lua_tostring(process->lua_state, -1));
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    return true;
}

/**
 * Stops a process. This will stop the process and all child processes.
 * @param process The process to stop.
 * @param force If TRUE, the process will be stopped immediately. If FALSE, the process will be stopped gracefully.
 * @param kill_children If TRUE, all child processes will be stopped immediately. If FALSE, child processes will be stopped gracefully.
 */
b8 process_stop(Process *process, b8 force, b8 kill_children) {
//    vinfo("Process %d stopped", process->pid);
//    if (process->state == PROCESS_STATE_STOPPED) {
//        vwarn("Process %d is already stopped", process->pid);
//        return false;
//    }
//
//    // close the lua state
//    lua_close(process->lua_state);
    return true;
}