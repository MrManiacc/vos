
#pragma once

#include "lua.h"
#include "defines.h"
// The maximum number of nested processes allowed.
#define INITIAL_NESTED_PROCESSES 4
// Unique identifiers for processes and groups.
typedef u32 ProcessID;
// Process State, used to determine if a process is running, paused, stopped, or dead.
typedef enum ProcessState {
  PROCESS_STATE_RUNNING,   // In this state, the process is running normally. This is the default state of a process.
  PROCESS_STATE_PAUSED,    // In this state, the process is paused. This is used to temporarily halt processing of a process.
  PROCESS_STATE_STOPPED,   // In this state, the process is stopped. This is the initial state of a process.
  PROCESS_STATE_DESTROYED,      // In this state, the process is dead. This is the final state of a process. Once a process is dead, it cannot be restarted.
  PROCESS_STATE_MAX_STATES
} ProcessState;

// Process Structure
typedef struct Process {
  // Unique process ID
  ProcessID process_id;
  // Path to the Lua script (process)
  // TODO: create vfs.h that can be used to load scripts from a virtual file system with hot reloading
  const char *script_path;
  // Pointer to the shared lua_State for this process, children will copy the pointer to their own lua_State
  lua_State *lua_state;
  // The context for accessing a process's child processes
  struct ProcessView *child_context;
  // Current state of the process
  ProcessState state;
} Process;

//A dynamically growing array of processes
typedef struct ProcessView {
  // Current number of child processes
  u32 count;
  // Current capacity of child process array
  u32 capacity;
  // Array of pointers to child processes
  Process *processes;
} ProcessView;

/**
 * Creates a new process. This will parse the script and create a new lua_State for the process.
 */
Process *process_create(const char *script_path);

/**
 * Adds a child process to a parent process. This will add the child process to the parent's child process array.
 * @param parent The parent process.
 * @param child The child process.
 * @return TRUE if the child process was successfully added; otherwise FALSE.
 */
b8 process_add_child(Process *parent, Process *child);

/**
 * Removes a child process to a parent process. This will remove the child process to the parent's child process array.
 * @param parent The parent process.
 * @param child The child process.
 * @return TRUE if the child process was successfully added; otherwise FALSE.
 */
b8 process_remove_child(Process *parent, ProcessID child_id);

/**
 * Destroys a process. This will stop the process and free all memory.
 */
void process_destroy(Process *process);

/**
 * Starts a process. This will start the process and all child processes.
 */
b8 process_start(Process *process);

/**
 * Stops a process. This will stop the process and all child processes.
 * @param process The process to stop.
 * @param force If TRUE, the process will be stopped immediately. If FALSE, the process will be stopped gracefully.
 * @param kill_children If TRUE, all child processes will be stopped immediately. If FALSE, child processes will be stopped gracefully.
 */
b8 process_stop(Process *process, b8 force, b8 kill_children);



