#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include "phost.h"

#include "core/vmem.h"
#include "core/vlogger.h"
#include "core/vstring.h"

#include "platform/platform.h"
#include "containers/darray.h"
#include "kernel.h"
#include "filesystem/paths.h"

/**
 * Creates a new process. This will parse the script and create a new lua_State for the process.
 */
proc *process_create(fs_node *script_file_node) {
    proc *process = kallocate(sizeof(proc), MEMORY_TAG_PROCESS);
    process->script_file_node = script_file_node;
    process->state = PROCESS_STATE_STOPPED;
    process->children_pids = darray_create(procid);
    Path path = process->script_file_node->path;
    //get the name by getting the last part of the path after the last slash
    char *name = string_split_at(path, "/", string_split_count(path, "/") - 1);
    //remove the extension
    name = string_split_at(name, ".", 0);
    process->process_name = name;
    return process;
}


/**
 * Starts a process. This will start the process and all child processes.
 */
b8 process_start(proc *process) {
    //run the lua source file
    process->state = PROCESS_STATE_RUNNING;
    //    process->process_name =
    fs_node *asset = process->script_file_node;
    if (asset == null || asset->type != NODE_FILE) {
        verror("Failed to load script %s", process->pid);
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    const char *source = asset->data.file.data;
    if (source == null) {
        verror("Failed to load script %s", process->pid);
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    char sources[asset->data.file.size + 1];
    kcopy_memory(sources, source, asset->data.file.size);
    sources[asset->data.file.size] = '\0';
    if (luaL_dostring(process->lua_state, sources) != LUA_OK) {
        const char *error_string = lua_tostring(process->lua_state, -1);
        verror("Failed to run script %d: %s", process->pid, error_string);
        //try to run it from file
        if (luaL_dofile(process->lua_state, platform_path(path_absolute(asset->path))) != LUA_OK) {
            error_string = lua_tostring(process->lua_state, -1);
            verror("Failed to run script %d: %s", process->pid, error_string);
            process->state = PROCESS_STATE_STOPPED;
            return false;
        }
    }
    vinfo("Process %d started", process->pid);
    return true;
}

/**
 * Stops a process. This will stop the process and all child processes.
 * @param process The process to stop.
 * @param force If TRUE, the process will be stopped immediately. If FALSE, the process will be stopped gracefully.
 * @param kill_children If TRUE, all child processes will be stopped immediately. If FALSE, child processes will be stopped gracefully.
 */
b8 process_stop(proc *process, b8 force, b8 kill_children) {
    vinfo("Process %d stopped", process->pid);
    if (process->state == PROCESS_STATE_STOPPED && !force && !kill_children) {
        vwarn("Process %d is already stopped", process->pid);
        return false;
    }
    if (kill_children) {
        u64 length = darray_length(process->children_pids);
        for (u64 i = 0; i < length; ++i) {
            procid child_pid = process->children_pids[i];
            KernelResult result = kernel_lookup_process(child_pid);
            if (result.code == KERNEL_PROCESS_CREATED) {
                proc *child = result.data;
                process_stop(child, force, kill_children);
            }
        }
    }
    darray_destroy(process->children_pids)
    // close the lua state
    lua_close(process->lua_state);
    kfree(process, sizeof(proc), MEMORY_TAG_PROCESS);
    return true;
}

/**
 * Destroys a process. This will stop the process and all child processes, and remove the process from the kernel.
 * @param process The process to destroy.
 * @return TRUE if the process was successfully destroyed; otherwise FALSE.
 */
void process_destroy(proc *process) {
    process_stop(process, true, true);
}

// Adds a child process to a parent process. This will add the child process to the parent's child process array.
b8 process_add_child(proc *parent, proc *child) {
// Make the child's lua_State a copy of the parent's lua_State
    child->lua_state = parent->lua_state;
    darray_push(parent->children_pids, child->pid)
    return true;
}

// Removes a child process to a parent process. This will remove the child process to the parent's child process array.
b8 process_remove_child(proc *parent, procid child_id) {
    u64 length = darray_length(parent->children_pids);
    for (u64 i = 0; i < length; ++i) {
        if (parent->children_pids[i] == child_id) {
            procid child_pid;
            darray_pop_at(parent->children_pids, i, &child_pid);
            vdebug("Child process %d removed from parent process %d", child_pid, parent->pid);
            return true;
        }
    }
    
    return false;
}