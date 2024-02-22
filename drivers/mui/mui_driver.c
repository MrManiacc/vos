/**
 * Created by jraynor on 2/22/2024.
 */
#pragma once

#include "kernel/kernel.h"
#include "core/vlogger.h"


VAPI b8 mui_driver_load(Kernel *kernel) {
    vinfo("Loading MUI driver");
    return true;
}

VAPI b8 mui_driver_install(Proc process) {
    lua_State *L = process.lua_state;
    if (!L) {
        verror("Failed to install MUI driver for process %d", process.pid);
        return false;
    }
    lua_newtable(L); // Create the sys table
    lua_pushstring(L, "Hello, World!");
    lua_setfield(L, -2, "message");
    lua_setglobal(L, "mui");
    return true;
}

VAPI b8 mui_driver_unload(Kernel *kernel) {
    vinfo("Unloading MUI driver");
    return true;
}

VAPI Driver create_driver(Kernel *kernel) {
    Driver driver = {
            .name = "MUI",
            .load = mui_driver_load,
            .install = mui_driver_install,
            .unload = mui_driver_unload
    };
    return driver;
}

