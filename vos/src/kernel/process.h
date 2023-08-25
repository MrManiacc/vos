/**
 * Created by jraynor on 8/24/2023.
 */
#pragma once

#include "lua.h"

// Unique identifiers for processes and groups.
typedef int ProcessID;
typedef int GroupID;

// Process Structure
typedef struct {
  ProcessID pid;             // Unique process ID
  const char *script_path;  // Path to the Lua script (process)
  lua_State *L;              // Pointer to the shared lua_State
} Process;

// Process Group Structure
typedef struct {
  GroupID group_id;                      // Unique group ID
  lua_State *L;                          // Shared lua_State for the processes in this group
  Process *processes[MAX_PROCESSES_PER_GROUP]; // Array of pointers to processes
  int process_count;                     // Current number of processes in the group
} ProcessGroup;
