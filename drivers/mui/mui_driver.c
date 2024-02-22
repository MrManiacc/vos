/**
 * Created by jraynor on 2/22/2024.
 */
#pragma once

#include "kernel/kernel.h"
#include "core/vlogger.h"
#include "lauxlib.h"
#include "core/vgui.h"

static Kernel *ctx;

b8 mui_driver_load() {
    // initialize any resources here
    vinfo("Loading MUI driver");
    return true;
}

int lua_color(lua_State *L) {
    int artiy = lua_gettop(L);
    if (artiy != 4 && artiy != 1) {
        return luaL_error(L, "Expected 4 argument to log_message");
    }
    if (artiy == 1) {
        //read hex string
        const char *hex = lua_tostring(L, 1);
        int r, g, b, a;
        sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a);
        lua_newtable(L);
        lua_pushinteger(L, r);
        lua_setfield(L, -2, "r");
        lua_pushinteger(L, g);
        lua_setfield(L, -2, "g");
        lua_pushinteger(L, b);
        lua_setfield(L, -2, "b");
        lua_pushinteger(L, a);
        lua_setfield(L, -2, "a");
        return 1;
    }
    int r = lua_tointeger(L, 1);
    int g = lua_tointeger(L, 2);
    int b = lua_tointeger(L, 3);
    int a = lua_tointeger(L, 4);
    lua_newtable(L);
    lua_pushinteger(L, r);
    lua_setfield(L, -2, "r");
    lua_pushinteger(L, g);
    lua_setfield(L, -2, "g");
    lua_pushinteger(L, b);
    lua_setfield(L, -2, "b");
    lua_pushinteger(L, a);
    lua_setfield(L, -2, "a");
    return 1;
}

int lua_draw_string(lua_State *L) {
    if (lua_gettop(L) != 5) {
        return luaL_error(L, "Expected 1 argument to log_message");
    }
    const char *message = lua_tostring(L, 1);
    // Get the x and y coordinates
    float x = lua_tonumber(L, 2);
    float y = lua_tonumber(L, 3);
    // Get the font size
    float size = lua_tonumber(L, 4);
    
    //check if we have a color at the top of the stack in a table
    if (lua_istable(L, 5)) {
        lua_getfield(L, 5, "r");
        int r = lua_tonumber(L, -1);
        lua_getfield(L, 5, "g");
        int g = lua_tonumber(L, -1);
        lua_getfield(L, 5, "b");
        int b = lua_tonumber(L, -1);
        lua_getfield(L, 5, "a");
        int a = lua_tonumber(L, -1);
        // log the position and color
//        DrawText(message, x, y, size, (Color) {r, g, b, a});
        gui_draw_text(ctx->window_context, message, x, y, size, "sans", nvgRGBA(r, g, b, a));
        return 0;
    }
    return 0;
}

int lua_draw_rect(lua_State *L) {
    if (lua_gettop(L) < 4) {
        return luaL_error(L, "Expected 4 argument to log_message");
    }
    // Get the x and y coordinates
    float x = (float) lua_tonumber(L, 1);
    float y = (float) lua_tonumber(L, 2);
    
    // Get width and height as integers
    float width = (float) lua_tonumber(L, 3);
    float height = (float) lua_tonumber(L, 4);
//    vdebug("Got: %d, %d, %d, %d", x, y, width, height);
    //check if we have a color at the top of the stack in a table
    if (lua_istable(L, 5)) {
        lua_getfield(L, 5, "r");
        int r = lua_tointeger(L, -1);
        lua_getfield(L, 5, "g");
        int g = lua_tointeger(L, -1);
        lua_getfield(L, 5, "b");
        int b = lua_tointeger(L, -1);
        lua_getfield(L, 5, "a");
        int a = lua_tointeger(L, -1);
        gui_draw_rect(ctx->window_context, x, y, width, height, nvgRGBA(r, g, b, a));
        // log the position and color
        return 0;
    }
    return 0;
}

int lua_text_width(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 2) {
        return luaL_error(L, "Expected 2 argument to text_width");
    }
    const char *text = lua_tostring(L, 1);
    int size = lua_tointeger(L, 2);
    lua_pushnumber(L, gui_text_width(ctx->window_context, text, "sans", size));
    return 1;
}

void configure_lua_gui(lua_State *L) {
    // Add a color function to the gui table
    lua_pushcfunction(L, lua_color);
    lua_setfield(L, -2, "color");
    
    // Add a draw_string function to the gui table
    lua_pushcfunction(L, lua_draw_string);
    lua_setfield(L, -2, "draw_text");
    
    // Add a draw_rect function to the gui table
    lua_pushcfunction(L, lua_draw_rect);
    lua_setfield(L, -2, "draw_rect");
    
    // Add a text_width function to the gui table
    lua_pushcfunction(L, lua_text_width);
    lua_setfield(L, -2, "text_width");
}


b8 mui_driver_install(Proc process) {
    lua_State *L = process.lua_state;
    if (!L) {
        verror("Failed to install MUI driver for process %d", process.pid);
        return false;
    }
    lua_newtable(L); // Create the sys table
    configure_lua_gui(L);
    lua_setglobal(L, "mui");
    return true;
}

b8 mui_driver_unload() {
    vinfo("Unloading MUI driver");
    return true;
}

VAPI Driver create_driver(Kernel *kernel) {
    ctx = kernel;
    Driver driver = {
            .name = "MUI",
            .load = mui_driver_load,
            .install = mui_driver_install,
            .unload = mui_driver_unload
    };
    return driver;
}

