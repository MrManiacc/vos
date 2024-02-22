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

b8 lua_payload_passthrough(Kernel *kernel, u16 code, void *sender, void *listener_inst, event_context data) {
    if (code != EVENT_LUA_CUSTOM) return false;
    
    char *event_name = data.data.c;
    
    for (int i = 0; i < lua_context.count; ++i) {
        LuaPayload *payload = &lua_context.payloads[i];
        if (payload->process == NULL)continue;
//        if (!strings_equal(payload->event_name, event_name)) continue;
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

VAPI b8 sys_driver_load() {
    if (!event_register(ctx, EVENT_LUA_CUSTOM, 0, lua_payload_passthrough)) {
        verror("Failed to register lua custom event");
        return false;
    }
    vinfo("Loaded SYS driver");
    return true;
}


VAPI b8 sys_driver_install(Proc process) {
    lua_State *L = process.lua_state;
    if (!L) {
        verror("Failed to install MUI driver for process %d", process.pid);
        return false;
    }
    lua_newtable(L); // Create the sys table
    lua_pushcfunction(L, lua_listen_for_event);
    lua_setfield(L, -2, "listen");
    
    
    lua_setglobal(L, "system");
    return true;
}

VAPI b8 sys_driver_unload() {
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

