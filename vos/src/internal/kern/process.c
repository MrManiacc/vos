/**
 * Created by jraynor on 2/27/2024.
 */
#include <assert.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

#include "kernel_ext.h"
#include "core/vlogger.h"
#include "core/vmem.h"
#include "platform/platform.h"

Process *kernel_new_driver_process(Kernel *kernel, const char *driver_path) {
    if (!kernel->initialized) {
        verror("Attempted to create a Lua process without initializing the kernel. Please call kernel_create before creating a Lua process.");
        return NULL;
    }
    Process *process = kallocate(sizeof(DriverProcess), MEMORY_TAG_KERNEL);
    if (!process) {
        verror("Failed to allocate memory for Driver process.");
        return NULL;
    }
    process->type = PROCESS_TYPE_DRIVER;
    process->path = driver_path;
    process->name = platform_file_name(driver_path);
    if (kernel->process_count >= KERNEL_MAX_PROCESSES) {
        verror("Too many processes");
        kfree(process, sizeof(DriverProcess), MEMORY_TAG_KERNEL);
        return NULL;
    }
    process->pid = kernel->process_count++;
    DriverProcess *dprocess = (DriverProcess *) process;
    DynLib *handle = kallocate(sizeof(DynLib), MEMORY_TAG_KERNEL);
    if (!platform_dynamic_library_load(driver_path, handle)) {
        verror("Failed to load driver %s", driver_path);
        kfree(handle, sizeof(DynLib), MEMORY_TAG_KERNEL);
        kfree(process, sizeof(DriverProcess), MEMORY_TAG_KERNEL);
        return null;
    }
    dprocess->handle = handle;
    // At this point, the Lua script is loaded but not executed.
    // You can now lookup functions within the script and call them later.
    kernel->processes[kernel->process_count - 1] = process; // Store the process in the kernel's processes array
    return process;
}


Process *kernel_new_lua_process(Kernel *kern, const char *script_path) {
    Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attempted to create a Lua process without initializing the kernel. Please call kernel_create before creating a Lua process.");
        return NULL;
    }

    LuaProcess *lua_process = kallocate(sizeof(LuaProcess), MEMORY_TAG_KERNEL);
    if (!lua_process) {
        verror("Failed to allocate memory for Lua process.");
        return NULL;
    }

    Process *process = (Process *) lua_process;
    process->type = PROCESS_TYPE_LUA;
    if (kernel->process_count >= KERNEL_MAX_PROCESSES) {
        verror("Too many processes");
        kfree(lua_process, sizeof(LuaProcess), MEMORY_TAG_KERNEL);
        return NULL;
    }
    process->pid = kernel->process_count++;
    process->path = script_path;
    process->state = PROCESS_STATE_STOPPED;
    process->name = platform_file_name(script_path);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    kernel_setup_lua_process_internal(kernel, process, L);
    // Load and execute the Lua script

    const char *source = platform_read_file(script_path);
    const u32 size = platform_file_size(script_path);
    if (luaL_loadbuffer(L, source, size, script_path) != LUA_OK) {
        verror("Failed to load Lua script %s: %s", script_path, lua_tostring(L, -1));
        lua_close(L);
        kfree(lua_process, sizeof(LuaProcess), MEMORY_TAG_KERNEL);
        return null;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        verror("Failed to execute Lua script %s: %s", script_path, lua_tostring(L, -1));
        lua_close(L);
        kfree(lua_process, sizeof(LuaProcess), MEMORY_TAG_KERNEL);
        return null;
    }
    lua_process->lua_state = L;
    kernel->processes[kernel->process_count - 1] = process; // Store the process in the kernel's processes array
    return process;
}

VAPI Process *kernel_new_gravity_process(Kernel *kern, const char *script_path) {
    Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return null;
    }
    Process *process = kallocate(sizeof(Process), MEMORY_TAG_KERNEL);
    process->type = PROCESS_TYPE_GRAVITY;
    kernel->processes[kernel->process_count] = process;
    process->pid = kernel->process_count++;
    process->path = script_path;
    return process;
}

VAPI Process *kernel_process_load(Kernel *kernel, const char *driver_path) {
    const char *ext = strrchr(driver_path, '.');
    if (ext == null) {
        verror("Invalid driver path. Please provide a valid driver path.")
        return null;
    }
    Process *process = null;
    if (strcmp(ext, platform_dynamic_library_extension()) == 0) {
        process = kernel_new_driver_process(kernel, driver_path);
    }
    if (strcmp(ext, ".lua") == 0) {
        process = kernel_new_lua_process(kernel, driver_path);
    }
    if (strcmp(ext, ".gravity") == 0) {
        process = kernel_new_gravity_process(kernel, driver_path);
    }
    if (process == null) {
        verror("Invalid driver path. Please provide a valid driver path.")
        return null;
    }
    return process;
}


VAPI b8 kernel_process_run(Kernel *kern, Process *process) {
    Kernel *kernel = kern ? kern : kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return false;
    }
    // If the process is already running, do nothing.
    if (process->state == PROCESS_STATE_RUNNING) {
        return true;
    }
    const Function *init_self = null;
    // run the setup_process function and return it's result
    if (process->type == PROCESS_TYPE_DRIVER) {
        init_self = kernel_process_function_lookup(process, (FunctionSignature){
            .name = "_init_self",
            .arg_count = 2,
            .return_type = FUNCTION_TYPE_BOOL,
            .args[0] = FUNCTION_TYPE_POINTER,
            .args[1] = FUNCTION_TYPE_POINTER,
        });
    }
    if (process->type == PROCESS_TYPE_LUA) {
        init_self = kernel_process_function_lookup(process, (FunctionSignature){
            .name = "_init_self",
            .arg_count = 0,
            .return_type = FUNCTION_TYPE_VOID
        });
    }
    if (init_self == null) {
        process->state = PROCESS_STATE_DESTROYED;
        // If the process is not a driver, do nothing.
        return false;
    }
    const b8 result = kernel_process_function_call(init_self, kernel, process).data.boolean;
    if (result) {
        process->state = PROCESS_STATE_RUNNING;
    }
    return result;
}


VAPI b8 kernel_process_pause(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_resume(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_stop(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI b8 kernel_process_terminate(Process *process) {
    assert(0 && "Not implemented");
    return false;
}

VAPI Process *kernel_process_get(const ProcessID pid) {
    const Kernel *kernel = kernel_get();
    if (!kernel->initialized) {
        verror("Attmepted to create a driver process without initializing the kernel. Please call kernel_create before creating a driver process.")
        return null;
    }
    if (pid >= kernel->process_count) return null;
    return kernel->processes[pid];
}
