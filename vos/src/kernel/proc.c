#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include "proc.h"

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
Proc *process_create(FsNode *script_file_node) {
    Proc *process = kallocate(sizeof(Proc), MEMORY_TAG_PROCESS);
    process->source_file_node = script_file_node;
    process->state = PROCESS_STATE_STOPPED;
    process->children_pids = darray_create(ProcID);
    FsPath path = process->source_file_node->path;
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
b8 process_start(Proc *process) {
    //run the lua source file
    process->state = PROCESS_STATE_RUNNING;
    //    process->process_name =
    FsNode *asset = process->source_file_node;
    if (asset == null || asset->type != NODE_FILE) {
        verror("Failed to load script %s", process->pid);
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    const char *source = asset->data.file.data;
    u64 size = asset->data.file.size;
    if (source == null) {
        verror("Failed to load script %s", process->pid);
        process->state = PROCESS_STATE_STOPPED;
        return false;
    }
    if (luaL_loadbuffer(process->lua_state, source, size, asset->path) != LUA_OK) {
        const char *error_string = lua_tostring(process->lua_state, -1);
        verror("Failed to run script %d: %s", process->pid, error_string);
        return false;
    }
    // execute the script
    if (lua_pcall(process->lua_state, 0, 0, 0) != LUA_OK) {
        const char *error_string = lua_tostring(process->lua_state, -1);
        verror("Failed to run script %d: %s", process->pid, error_string);
        process->state = PROCESS_STATE_STOPPED;
        return false;
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
b8 process_stop(Proc *process, b8 force, b8 kill_children) {
    vinfo("Process %d stopped", process->pid);
    if (process->state == PROCESS_STATE_STOPPED && !force && !kill_children) {
        vwarn("Process %d is already stopped", process->pid);
        return false;
    }
    if (kill_children) {
        u64 length = darray_length(process->children_pids);
        for (u64 i = 0; i < length; ++i) {
            ProcID child_pid = process->children_pids[i];
            KernelResult result = kernel_lookup_process(child_pid);
            if (result.code == KERNEL_PROCESS_CREATED) {
                Proc *child = result.data;
                process_stop(child, force, kill_children);
            }
        }
    }
    darray_destroy(process->children_pids)
    // close the lua state
    lua_close(process->lua_state);
    kfree(process, sizeof(Proc), MEMORY_TAG_PROCESS);
    return true;
}

/**
 * Destroys a process. This will stop the process and all child processes, and remove the process from the kernel.
 * @param process The process to destroy.
 * @return TRUE if the process was successfully destroyed; otherwise FALSE.
 */
void process_destroy(Proc *process) {
    process_stop(process, true, true);
}

// Adds a child process to a parent process. This will add the child process to the parent's child process array.
b8 process_add_child(Proc *parent, Proc *child) {
// Make the child's lua_State a copy of the parent's lua_State
    child->lua_state = parent->lua_state;
    darray_push(ProcID, parent->children_pids, child->pid)
    return true;
}

// Removes a child process to a parent process. This will remove the child process to the parent's child process array.
b8 process_remove_child(Proc *parent, ProcID child_id) {
    u64 length = darray_length(parent->children_pids);
    for (u64 i = 0; i < length; ++i) {
        if (parent->children_pids[i] == child_id) {
            ProcID child_pid;
            darray_pop_at(parent->children_pids, i, &child_pid);
            vdebug("Child process %d removed from parent process %d", child_pid, parent->pid);
            return true;
        }
    }
    
    return false;
}