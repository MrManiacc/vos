
#pragma once

#include "lua.h"
#include "defines.h"
#include "kernel/vfs/vfs.h"

// Unique identifiers for processes and groups.
typedef u32 procid;
// Process State, used to determine if a process is running, paused, stopped, or dead.
typedef enum ProcessState {
    PROCESS_STATE_RUNNING,   // In this state, the process is running normally. This is the default state of a process.
    PROCESS_STATE_PAUSED,    // In this state, the process is paused. This is used to temporarily halt processing of a process.
    PROCESS_STATE_STOPPED,   // In this state, the process is stopped. This is the initial state of a process.
    PROCESS_STATE_DESTROYED,      // In this state, the process is dead. This is the final state of a process. Once a process is dead, it cannot be restarted.
    PROCESS_STATE_MAX_STATES
} proc_state;

// Process Structure
typedef struct proc {
    // Unique process ID
    procid pid;
    // Name of the process
    const char *process_name;
    // Path to the Lua script (process)
    fs_node *script_file_node;
    // Pointer to the shared lua_State for this process, children will copy the pointer to their own lua_State
    lua_State *lua_state;
    // The context for accessing a process's child processes
    procid *children_pids;
    // Current state of the process
    proc_state state;
} proc;

/**
 * Creates a new process. This will parse the script and create a new lua_State for the process.
 */
proc *process_create(fs_node *script_file_node);

/**
 * Adds a child process to a parent process. This will add the child process to the parent's child process array.
 * @param parent The parent process.
 * @param child The child process.
 * @return TRUE if the child process was successfully added; otherwise FALSE.
 */
b8 process_add_child(proc *parent, proc *child);

/**
 * Removes a child process to a parent process. This will remove the child process to the parent's child process array.
 * @param parent The parent process.
 * @param child The child process.
 * @return TRUE if the child process was successfully added; otherwise FALSE.
 */
b8 process_remove_child(proc *parent, procid child_id);


/**
 * Starts a process. This will start the process and all child processes.
 */
b8 process_start(proc *process);

/**
 * Stops a process. This will stop the process and all child processes.
 * @param process The process to stop.
 * @param force If TRUE, the process will be stopped immediately. If FALSE, the process will be stopped gracefully.
 * @param kill_children If TRUE, all child processes will be stopped immediately. If FALSE, child processes will be stopped gracefully.
 */
b8 process_stop(proc *process, b8 force, b8 kill_children);

/**
 * Destroys a process. This will stop the process and all child processes, and remove the process from the kernel.
 * @param process The process to destroy.
 * @return TRUE if the process was successfully destroyed; otherwise FALSE.
 */
void process_destroy(proc *process);



