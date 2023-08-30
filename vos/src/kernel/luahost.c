/**
 * Created by jraynor on 8/26/2023.
 */
#include "luahost.h"
#include "core/logger.h"
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <raylib.h>
#include "luahost.h"
#include "core/event.h"
#include "core/str.h"
#include "core/mem.h"
#include "kernel/vfs/vfs.h"
#include "containers/Map.h"
#include "kernel/vfs/paths.h"
#define MAX_LUA_PAYLOADS 100
typedef struct LuaPayload {
  Process *process;
  char *event_name;
  int callback_ref;
} LuaPayload;

typedef struct LuaPayloadContext {
  LuaPayload payloads[MAX_LUA_PAYLOADS];
  int count;
} LuaPayloadContext;

static LuaPayloadContext lua_context;

int lua_execute_process(lua_State *L) {
//    Get the argument passed to the function, it can be a string or an int
    int arg_type = lua_type(L, 1);
    if (arg_type != LUA_TSTRING && arg_type != LUA_TNUMBER) {
        verror("Invalid argument passed to process: %s", lua_typename(L, arg_type));
        return 1;
    }
    Process *process;
    // Get the process name
    if (arg_type == LUA_TSTRING) {
        const char *process_name = lua_tostring(L, 1);

        //        vdebug("Executing process at: %s", process_name);
        //TODO: create process by looking up asset
//        Process *process = kernel_create_process(process_name);
//        if (!is_kernel_success(result.code)) {
//            verror("Failed to create process: %s", get_kernel_result_message(result));
//            return 1;
//        }
//
//        KernelResult lookup_result = kernel_lookup_process((ProcessID) result.data);
//        if (!is_kernel_success(lookup_result.code)) {
//            verror("Failed to lookup process: %s", get_kernel_result_message(lookup_result));
//            return 1;
//        }
//        process = lookup_result.data;
//        process_start(process);
    } else {
        int process_id = lua_tointeger(L, 1);
        vdebug("Executing process main function with id: %d", process_id);
        KernelResult result = kernel_lookup_process((ProcessID) process_id);
        if (!is_kernel_success(result.code)) {
            verror("Failed to lookup process: %s", get_kernel_result_message(result));
            return 1;
        }
        process = result.data;
    }
    if (process == null) {
        verror("Failed to execute process, process is null");
        return 1;
    }
    if (process->lua_state == null) {
        verror("Failed to execute process, lua state is null");
        return 1;
    }
    lua_getglobal(process->lua_state, "main");
    if (lua_pcall(process->lua_state, 0, 0, 0) != 0) {
        verror("Failed to execute process: %s", lua_tostring(L, -1));
    }
    return 0;
}
/**
 * Will listen for an event from the process.
 *
 * Takes two lua arguments, a string and a function.
 */
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
    payload->event_name = string_duplicate(event_name); // we'll need to free this later
    payload->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_getglobal(L, "sys");
    lua_getfield(L, -1, "pid");
    int process_id = lua_tointeger(L, -1);
    vdebug("Executing process with id: %d", process_id);
    KernelResult result = kernel_lookup_process((ProcessID) process_id);
    if (!is_kernel_success(result.code)) {
        verror("Failed to lookup process: %s", get_kernel_result_message(result));
        return 1;
    }
    Process *process = result.data;
    payload->process = process;
    return 0;
}

int lua_log_message(lua_State *L) {
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "Expected 1 argument to log_message");
    }
    const char *message = lua_tostring(L, 1);
    lua_getglobal(L, "sys");
    lua_getfield(L, -1, "pid");
    int process_id = lua_tointeger(L, -1);
    lua_getfield(L, -2, "name");
//    printf("[%d] %s: %s\n", process_id, lua_tostring(L, -1), message);
    const char *process_name = lua_tostring(L, -1);
    vinfo("[%s - 0x%04x] %s", process_name, process_id, message);
    return 0;
}

int lua_draw_string(lua_State *L) {
    if (lua_gettop(L) != 5) {
        return luaL_error(L, "Expected 1 argument to log_message");
    }
    const char *message = lua_tostring(L, 1);
    // Get the x and y coordinates
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    // Get the font size
    int size = lua_tointeger(L, 4);

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
        DrawText(message, x, y, size, (Color) {r, g, b, a});
        return 0;
    }
    return 0;
}

int lua_draw_rect(lua_State *L) {
    if (lua_gettop(L) < 4) {
        return luaL_error(L, "Expected 4 argument to log_message");
    }
    // Get the x and y coordinates
    int x = (int)lua_tonumber(L, 1);
    int y = (int)lua_tonumber(L, 2);

    // Get width and height as integers
    int width = (int)lua_tonumber(L, 3);
    int height = (int)lua_tonumber(L, 4);
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
        DrawRectangle(x, y, width, height, (Color) {r, g, b, a});
        return 0;
    }
    DrawRectangle(x, y, width, height, DARKGRAY);
    return 0;
}

int lua_color(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 4 && top != 1) {
        return luaL_error(L, "Expected 4 argument to log_message");
    }
    if (top == 1) {
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

int lua_time(lua_State *L) {
    lua_pushnumber(L, GetTime());
    return 1;
}

int lua_import(lua_State *L) {
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "Expected 1 argument to import");
    }
    const char *module_name = lua_tostring(L, 1);
    lua_getglobal(L, "sys");
    lua_getfield(L, -1, "path");
    const char *path = lua_tostring(L, -1);
    char *parent_dir = strdup(path);
    char *last_slash = strrchr(parent_dir, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
    }
    char *full_path = string_format("%s/%s.lua", parent_dir, module_name);
    //lookup the process, if it exists then we mark it as a dependency on the process



    vdebug("Importing module %s from %s", module_name, full_path);
    if (luaL_dofile(L, full_path) != LUA_OK) {
        verror("Failed to run script %s", full_path);
        //print the error
        verror("Error: %s", lua_tostring(L, -1));
    }
    kfree(full_path, string_length(full_path), MEMORY_TAG_STRING);
    //TODO build a list of dependencies that can be used for hot reloading


}

int lua_is_button_down(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, IsMouseButtonDown(button));
    return 1;
}

int lua_is_button_up(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, IsMouseButtonUp(button));
    return 1;
}

int lua_is_button_pressed(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, IsMouseButtonPressed(button));
    return 1;
}

int lua_is_button_released(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, IsMouseButtonReleased(button));
    return 1;
}

int lua_mouse(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to mouse");
    }
    Vector2 mouse = GetMousePosition();

    lua_newtable(L);
    lua_pushinteger(L, mouse.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, mouse.y);
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
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to keyboard");
    }
    int key = lua_tointeger(L, 1);
    lua_newtable(L);
    //TODO: assign key codes to lua
    lua_pushboolean(L, IsKeyDown(key));
    lua_setfield(L, -2, "is_down");
    lua_pushboolean(L, IsKeyUp(key));
    lua_setfield(L, -2, "is_up");
    lua_pushboolean(L, IsKeyPressed(key));
    lua_setfield(L, -2, "is_pressed");
    lua_pushboolean(L, IsKeyReleased(key));
    lua_setfield(L, -2, "is_released");
    return 1;
}



void configure_lua_input(Process *process) {
    // Create the gui table
    lua_newtable(process->lua_state);

    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_mouse);
    lua_setfield(process->lua_state, -2, "mouse");

    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_key);
    lua_setfield(process->lua_state, -2, "key");

    // Attach the gui table to the sys table
    lua_setfield(process->lua_state, -2, "input");
}
int lua_text_width(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 2) {
        return luaL_error(L, "Expected 2 argument to text_width");
    }
    const char *text = lua_tostring(L, 1);
    int size = lua_tointeger(L, 2);
    lua_pushinteger(L, MeasureText(text, size));
    return 1;
}

int lua_windwow_size(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to window_size");
    }
    lua_newtable(L);
    lua_pushinteger(L, GetScreenWidth());
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, GetScreenHeight());
    lua_setfield(L, -2, "height");
    return 1;
}

int configure_lua_window(Process *process) {
    // Create the gui table
    lua_newtable(process->lua_state);

    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_windwow_size);
    lua_setfield(process->lua_state, -2, "size");

    // Attach the gui table to the sys table
    lua_setfield(process->lua_state, -2, "window");
}

void configure_lua_gui(Process *process) {
    // Create the gui table
    lua_newtable(process->lua_state);

    // Add a color function to the gui table
    lua_pushcfunction(process->lua_state, lua_color);
    lua_setfield(process->lua_state, -2, "color");

    // Add a draw_string function to the gui table
    lua_pushcfunction(process->lua_state, lua_draw_string);
    lua_setfield(process->lua_state, -2, "draw_text");

    // Add a draw_rect function to the gui table
    lua_pushcfunction(process->lua_state, lua_draw_rect);
    lua_setfield(process->lua_state, -2, "draw_rect");

    // Add a text_width function to the gui table
    lua_pushcfunction(process->lua_state, lua_text_width);
    lua_setfield(process->lua_state, -2, "text_width");

    // Attach the gui table to the sys table
    lua_setfield(process->lua_state, -2, "gui");
}

int lua_file_system_string(lua_State *L) {
    //
    lua_pushstring(L, fs_to_string());
    return 1;
}

b8 initialize_syscalls_for(Process *process) {
    process->lua_state = luaL_newstate();
    luaL_openlibs(process->lua_state);

    lua_newtable(process->lua_state); // Create the sys table

    // Register the process ID
    lua_pushinteger(process->lua_state, process->pid);
    lua_setfield(process->lua_state, -2, "pid");

    lua_pushstring(process->lua_state, process->script_asset->path);
    lua_setfield(process->lua_state, -2, "path");

    // Register the process name
    lua_pushstring(process->lua_state, process->process_name);
//    lua_pushstring(process->lua_state, process->process_name);
    lua_setfield(process->lua_state, -2, "name");

    lua_pushcfunction(process->lua_state, lua_execute_process);
    lua_setfield(process->lua_state, -2, "execute");

    lua_pushcfunction(process->lua_state, lua_listen_for_event);
    lua_setfield(process->lua_state, -2, "listen");

    lua_pushcfunction(process->lua_state, lua_log_message);
    lua_setfield(process->lua_state, -2, "log");

    lua_pushcfunction(process->lua_state, lua_time);
    lua_setfield(process->lua_state, -2, "time");

    lua_pushcfunction(process->lua_state, lua_import);
    lua_setfield(process->lua_state, -2, "import");

    lua_pushcfunction(process->lua_state, lua_file_system_string);
    lua_setfield(process->lua_state, -2, "fs_str");

    configure_lua_gui(process);

    configure_lua_input(process);

    configure_lua_window(process);

    lua_setglobal(process->lua_state, "sys"); // Set the sys table as a global variable
}

b8 lua_payload_passthrough(u16 code, void *sender, void *listener_inst, event_context data) {
    if (code != EVENT_LUA_CUSTOM) return false;

    char *event_name = data.data.c;

    for (int i = 0; i < lua_context.count; ++i) {
        LuaPayload *payload = &lua_context.payloads[i];
        if (payload->process == NULL)continue;
        if (strcmp(payload->event_name, event_name) == 0) {
            lua_State *L = payload->process->lua_state; // Get your Lua state from wherever it's stored

            lua_rawgeti(L, LUA_REGISTRYINDEX, payload->callback_ref);

            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                verror("Error executing Lua callback: %s", lua_tostring(L, -1));
                lua_pop(L, 1); // Remove error message
            }
        }
    }

    return true;
}
static asset_loader *lua_loader;

void load_lua_asset(Node *node, Asset *asset) {
//    asset->path = path_relative((char *) node->path);


    asset->path = node->path;
    asset->data = node->data.file.data;
    asset->size = node->data.file.size;
//    vdebug("Loaded lua asset %s", asset->data);
//    Process *process = process_create(asset);
//    initialize_syscalls_for(process);
//    process_start(process);
    Process *process = kernel_create_process(asset);
    if (strings_equal(process->process_name, "env")) {
        process_start(process);
    }
//    process_start(process);

}

void unload_lua_asset(Node *node) {
//    asset->data = fs_read_file(asset->path, &asset->size);
    Process *process = kernel_locate_process(node->path);
//    vdebug("Unloaded lua asset %s at %s", process->process_name, node->path);
    //Check if the process is running and stop it
    //find and free any payloads with our process
    for (int i = 0; i < lua_context.count; ++i) {
        LuaPayload *payload = &lua_context.payloads[i];
        if (payload->process == process) {
            luaL_unref(process->lua_state, LUA_REGISTRYINDEX, payload->callback_ref);
            kfree(payload->event_name, string_length(payload->event_name), MEMORY_TAG_STRING);
            payload->event_name = null;
            payload->callback_ref = LUA_NOREF;
            payload->process = NULL;
            lua_context.count--;
        }
    }
//    free the asset resources
    if (process->state == PROCESS_STATE_RUNNING) {
        process_stop(process, true, true);
    }
    kernel_destroy_process(process->pid);
}

b8 initialize_syscalls() {
    event_register(EVENT_LUA_CUSTOM, 0, lua_payload_passthrough);
    lua_loader = kallocate(sizeof(asset_loader), MEMORY_TAG_KERNEL);
    lua_loader->extension = "lua";
    lua_loader->load = load_lua_asset;
    lua_loader->unload = unload_lua_asset;
    fs_register_asset_loader(lua_loader);
    return true;
}

b8 shutdown_syscalls() {
    event_unregister(EVENT_LUA_CUSTOM, 0, lua_payload_passthrough);
    kfree(lua_loader, sizeof(asset_loader), MEMORY_TAG_KERNEL);
}