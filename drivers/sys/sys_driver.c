/**
 * Created by jraynor on 2/22/2024.
 */
#pragma once

#include <string.h>
#include "kernel/kernel.h"
#include "core/vlogger.h"
#include "core/vevent.h"
#include "core/vstring.h"
#include "lauxlib.h"
#include "core/vinput.h"
#include "platform/platform.h"
#include "core/vmem.h"

#define MAX_LUA_PAYLOADS 100
typedef struct LuaPayload {
    Proc *process;
    char *event_name;
    int callback_ref;
} LuaPayload;

typedef struct LuaPayloadContext {
    LuaPayload payloads[MAX_LUA_PAYLOADS];
    int count;
} LuaPayloadContext;

typedef char *string;
static LuaPayloadContext lua_context;
static Kernel *ctx;
typedef struct EventState EventState;

b8 lua_payload_passthrough(Kernel *kernel, u16 code, void *sender, void *listener_inst, event_context data) {
    if (code != EVENT_LUA_CUSTOM) return false;
    
    char *event_name = data.data.c;
    for (int i = 0; i < lua_context.count; ++i) {
        LuaPayload *payload = &lua_context.payloads[i];
        if (payload->process == NULL)continue;
        lua_State *L = payload->process->lua_state; // Get your Lua state from wherever it's stored
        lua_rawgeti(L, LUA_REGISTRYINDEX, payload->callback_ref);
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            verror("Error executing Lua callback: %s", lua_tostring(L, -1));
            lua_pop(L, 1); // Remove error message
        }
    }
    return true;
}

int lua_listen_for_event(lua_State *L) {
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "Expected 2 arguments to listen_for_event");
    }
    
    const char *event_name = lua_tostring(L, 1);
    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "Expected a function as the second argument");
    }
    
    if (lua_context.count >= MAX_LUA_PAYLOADS) {
        return luaL_error(L, "Exceeded maximum number of Lua callbacks");
    }
    
    //Find free slot
    u64 index = 0;
    for (u64 i = 0; i < MAX_LUA_PAYLOADS; ++i) {
        if (lua_context.payloads[i].process == NULL) {
            index = i;
            break;
        }
    }
    if (index > MAX_LUA_PAYLOADS - 25) {
        vwarn("Lua payload context is getting full, consider increasing MAX_LUA_PAYLOADS");
    }
    
    LuaPayload *payload = &lua_context.payloads[index];
    lua_context.count++;
    payload->event_name = strdup(event_name); // we'll need to free this later
    payload->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_getglobal(L, "sys");
    lua_getfield(L, -1, "pid");
    int process_id = lua_tointeger(L, -1);
    Proc *process = kernel_lookup_process(ctx, (ProcID) process_id);
    if (!process) {
        verror("Failed to find process with id %d", process_id);
        return luaL_error(L, "Failed to find process with id %d", process_id);
    }
    vinfo("found process %d, %s", process_id, process->process_name);
    payload->process = process;
    return 0;
}

b8 sys_driver_load() {
    if (!event_register(ctx, EVENT_LUA_CUSTOM, 0, lua_payload_passthrough)) {
        verror("Failed to register lua custom event");
        return false;
    }
    
    vinfo("Loaded SYS driver");
    return true;
}


int lua_is_button_down(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_button_down(windowContext, button));
    return 1;
}

int lua_is_button_up(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_key_up(windowContext, button));
    return 1;
}

int lua_is_button_pressed(lua_State *L) {

    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_button_pressed(windowContext, button));
    return 1;
}

int lua_is_button_released(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_button_up(windowContext, button));
    return 1;
}

int lua_scroll(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to scroll");
    }
    i8 x, y;
    input_get_scroll_delta(windowContext, &x, &y);
    lua_newtable(L);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");
    return 1;
}

int lua_mouse(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to mouse");
    }
    i32 x, y;
    input_get_mouse_position(windowContext, &x, &y);
    lua_newtable(L);
    lua_pushinteger(L, x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, y);
    lua_setfield(L, -2, "y");
    lua_pushcfunction(L, lua_is_button_down);
    lua_setfield(L, -2, "is_down");
    lua_pushcfunction(L, lua_is_button_up);
    lua_setfield(L, -2, "is_up");
    lua_pushcfunction(L, lua_is_button_pressed);
    lua_setfield(L, -2, "is_pressed");
    lua_pushcfunction(L, lua_is_button_released);
    lua_setfield(L, -2, "is_released");

    return 1;
}

int lua_key(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to keyboard");
    }
    int key = lua_tointeger(L, 1);
    lua_newtable(L);
    //TODO: assign key codes to lua
    lua_pushboolean(L, input_is_key_down(windowContext, key));
    lua_setfield(L, -2, "is_down");
    lua_pushboolean(L, input_is_key_up(windowContext, key));
    lua_setfield(L, -2, "is_up");
    lua_pushboolean(L, input_is_key_pressed(windowContext, key));
    lua_setfield(L, -2, "is_pressed");
    lua_pushboolean(L, input_is_key_released(windowContext, key));
    lua_setfield(L, -2, "is_released");
    return 1;
}


int lua_windwow_size(lua_State *L) {
    WindowContext *windowContext = ctx->window_context;
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to window_size");
    }
    u32 width, height;
    window_get_size(windowContext, &width, &height);
    lua_newtable(L);
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");
    return 1;
}


int configure_lua_window(Proc *process) {
    // Create the gui table
    lua_newtable(process->lua_state);
    
    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_windwow_size);
    lua_setfield(process->lua_state, -2, "size");
    
    // Attach the gui table to the sys table
    lua_setfield(process->lua_state, -2, "window");
    return 1;
}

void configure_lua_input(Proc *process) {
    // Create the gui table
    lua_newtable(process->lua_state);
    
    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_mouse);
    lua_setfield(process->lua_state, -2, "mouse");
    
    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_key);
    lua_setfield(process->lua_state, -2, "key");
    
    lua_pushcfunction(process->lua_state, lua_scroll);
    lua_setfield(process->lua_state, -2, "scroll");
    
    // Attach the gui table to the sys table
    lua_setfield(process->lua_state, -2, "input");
}

void configure_lua_listener(Proc *process) {
    lua_pushcfunction(process->lua_state, lua_listen_for_event);
    lua_setfield(process->lua_state, -2, "listen");
}

int lua_log_info(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "Expected 1 argument to log");
    }
    const char *message = lua_tostring(L, 1);
    vinfo("Lua: %s", message);
    return 0;
}

int lua_log_error(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "Expected 1 argument to log");
    }
    const char *message = lua_tostring(L, 1);
    verror("Lua: %s", message);
    return 0;
}

int lua_log_warn(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "Expected 1 argument to log");
    }
    const char *message = lua_tostring(L, 1);
    vwarn("Lua: %s", message);
    return 0;
}

void configure_lua_log(Proc *process) {
    lua_newtable(process->lua_state);
    lua_pushcfunction(process->lua_state, lua_log_info);
    lua_setfield(process->lua_state, -2, "info");
    lua_pushcfunction(process->lua_state, lua_log_error);
    lua_setfield(process->lua_state, -2, "error");
    lua_pushcfunction(process->lua_state, lua_log_warn);
    lua_setfield(process->lua_state, -2, "warn");
    // Attach the gui table to the sys table;
    lua_setfield(process->lua_state, -2, "log");
}


int lua_time(lua_State *L) {
    static f64 start_time = 0;
    if (start_time == 0) {
        start_time = platform_get_absolute_time();
    }
    f64 current_time = platform_get_absolute_time();
    lua_pushnumber(L, current_time - start_time);
    return 1;
}

void configure_lua_time(Proc *process) {
    lua_pushcfunction(process->lua_state, lua_time);
    lua_setfield(process->lua_state, -2, "time");
}


int lua_import(lua_State *L) {
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "Expected 1 argument to import");
    }
    const char *module_name = lua_tostring(L, 1);
    lua_getglobal(L, "sys");
    lua_getfield(L, -1, "path");
    const char *path = lua_tostring(L, -1);
    char full_path[256];
    sprintf(full_path, "%s.lua", module_name);
    FsNode *node = vfs_node_get(ctx->fs_context, full_path);
    
    if (node == null) {
        verror("Failed to import module %s, file not found", module_name);
        return 1;
    }
    
    //Execute the lua from source
    char *data = node->data.file.data;
    u64 size = node->data.file.size;
    
    if (luaL_loadbuffer(L, data, size, full_path) != LUA_OK) {
        const char *error_string = lua_tostring(L, -1);
        verror("Failed to run script %d: %s", full_path, error_string);
        return 1;
    }
    // execute the script
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char *error_string = lua_tostring(L, -1);
        verror("Failed to run script %d: %s", full_path, error_string);
        return 1;
    }
    //TODO build a list of dependencies that can be used for hot reloading
    return 10;
}

void configure_lua_import(Proc *process) {
    lua_pushcfunction(process->lua_state, lua_import);
    lua_setfield(process->lua_state, -2, "import");
}

b8 sys_driver_install(Proc process) {
    lua_State *L = process.lua_state;
    if (!L) {
        verror("Failed to install MUI driver for process %d", process.pid);
        return false;
    }
    lua_newtable(L); // Create the sys table
    //    // Register the process ID
    lua_pushinteger(L, process.pid);
    lua_setfield(L, -2, "pid");
    configure_lua_log(&process);
    configure_lua_input(&process);
    configure_lua_window(&process);
    configure_lua_time(&process);
    configure_lua_import(&process);
    configure_lua_listener(&process);
    lua_setglobal(L, "sys");
    return true;
}

b8 sys_driver_unload() {
    event_unregister(ctx, EVENT_LUA_CUSTOM, 0, lua_payload_passthrough);
    return true;
}

VAPI Driver create_driver(Kernel *kernel) {
    ctx = kernel;
    Driver driver = {
            .name = "SYS",
            .load = sys_driver_load,
            .install = sys_driver_install,
            .unload = sys_driver_unload
    };
    return driver;
}

