/**
 * Created by jraynor on 8/26/2023.
 */
#include "vlua.h"
#include "core/vlogger.h"
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
//#include <raylib.h>
#include "core/vevent.h"
#include "core/vstring.h"
#include "core/vmem.h"
#include "containers/dict.h"
#include "filesystem/paths.h"
#include "platform/platform.h"
#include "core/vinput.h"

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

int lua_execute_process(lua_State *L) {
//    Get the argument passed to the function, it can be a string or an int
    int arg_type = lua_type(L, 1);
    if (arg_type != LUA_TSTRING && arg_type != LUA_TNUMBER) {
        verror("Invalid argument passed to process: %s", lua_typename(L, arg_type));
        return 1;
    }
    Proc *process;
    // Get the process name
    if (arg_type == LUA_TSTRING) {
        const char *process_name = lua_tostring(L, 1);
        
        vdebug("Executing process at: %s", process_name);
//        Asset *node = fs_load_node(process_name);
//        Process *process = kernel_create_process(process_name);
//        if (process == null) {
//            verror("Failed to create process: %s", process_name);
//            return 1;
//        }
//
//        KernelResult lookup_result = kernel_lookup_process((ProcessID) result.data);
//        if (!is_kernel_success(lookup_result.code)) {
//            verror("Failed to lookup process: %s", get_kernel_result_message(lookup_result));
//            return 1;
//        }
//        process = lookup_result.data;
//        if (!process_start(process)) {
//            verror("Failed to start process: %s", process_name);
//            return 1;
//        } else {
//            vinfo("Process %s started, pid %d", process_name, process->pid);
//            //push the process id to the stack
//            lua_pushinteger(L, process->pid);
//        }
    } else {
        ProcID process_id = (ProcID) lua_tointeger(L, 1);
        KernelResult result = kernel_lookup_process(process_id);
        if (!kernel_is_result_success(result.code)) {
            verror("Failed to lookup process: %s", kernel_get_result_message(result));
            return 1;
        }
        process = result.data;
        vdebug("Executing process %s's main function with pid: %d", process->process_name, process_id);
    }
    if (process == null) {
        verror("Failed to execute process, process is null");
        return 1;
    }
    if (process->lua_state == null) {
        verror("Failed to execute process, lua state is null");
        return 1;
    }
    // TODO: check if it exists, if it doesn't we treat it like a library
    // Execute the main function
    lua_getglobal(process->lua_state, "main");
    if (lua_pcall(process->lua_state, 0, 0, 0) != LUA_OK) {
        verror("Error executing Lua main function: %s", lua_tostring(process->lua_state, -1));
        lua_pop(process->lua_state, 1); // Remove error message
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
    KernelResult result = kernel_lookup_process((ProcID) process_id);
    if (!kernel_is_result_success(result.code)) {
        verror("Failed to lookup process: %s", kernel_get_result_message(result));
        return 1;
    }
    Proc *process = result.data;
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
    int x = lua_tonumber(L, 2);
    int y = lua_tonumber(L, 3);
    // Get the font size
    int size = lua_tonumber(L, 4);
    
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
        gui_draw_text(message, x, y, size, "sans", nvgRGBA(r, g, b, a));
        return 0;
    }
    return 0;
}

int lua_draw_rect(lua_State *L) {
    if (lua_gettop(L) < 4) {
        return luaL_error(L, "Expected 4 argument to log_message");
    }
    // Get the x and y coordinates
    int x = (int) lua_tonumber(L, 1);
    int y = (int) lua_tonumber(L, 2);
    
    // Get width and height as integers
    int width = (int) lua_tonumber(L, 3);
    int height = (int) lua_tonumber(L, 4);
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
        gui_draw_rect(x, y, width, height, nvgRGBA(r, g, b, a));
        // log the position and color
        return 0;
    }
//    DrawRectangle(x, y, width, height, DARKGRAY);
    
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
    static u64 start_time = 0;
    if (start_time == 0) {
        start_time = platform_get_absolute_time();
    }
    u64 current_time = platform_get_absolute_time();
    lua_pushinteger(L, current_time - start_time);
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
    char *full_path = string_concat(module_name, ".lua");
    // get the node data from the file system
    FsNode *node = vfs_node_get(full_path);
    
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
    kfree(full_path, string_length(full_path), MEMORY_TAG_STRING);
    //TODO build a list of dependencies that can be used for hot reloading
    return 10;
}

int lua_is_button_down(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_button_down(button));
    return 1;
}

int lua_is_button_up(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_key_up(button));
    return 1;
}

int lua_is_button_pressed(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_button_down(button));
    return 1;
}

int lua_is_button_released(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to is_button_down");
    }
    int button = lua_tointeger(L, 1);
    lua_pushboolean(L, input_is_button_up(button));
    return 1;
}

int lua_mouse(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to mouse");
    }
    i32 x, y;
    input_get_mouse_position(&x, &y);
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
    int top = lua_gettop(L);
    if (top != 1) {
        return luaL_error(L, "Expected 1 argument to keyboard");
    }
    int key = lua_tointeger(L, 1);
    lua_newtable(L);
    //TODO: assign key codes to lua
    lua_pushboolean(L, input_is_key_down(key));
    lua_setfield(L, -2, "is_down");
    lua_pushboolean(L, input_is_key_up(key));
    lua_setfield(L, -2, "is_up");
    lua_pushboolean(L, input_is_key_pressed(key));
    lua_setfield(L, -2, "is_pressed");
    lua_pushboolean(L, input_is_key_released(key));
    lua_setfield(L, -2, "is_released");
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
    void *vg = window_context.vg;
    lua_pushnumber(L, gui_text_width(text, "sans", size));
    return 1;
}

int lua_windwow_size(lua_State *L) {
    int top = lua_gettop(L);
    if (top != 0) {
        return luaL_error(L, "Expected 0 argument to window_size");
    }
    u32 width, height;
    window_get_size(&width, &height);
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

void configure_lua_gui(Proc *process) {
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
    //TODO: add the ability to read files from the file system
    lua_pushstring(L, vfs_to_string());
    return 1;
}

b8 intrinsics_install_to(Proc *process) {
    process->lua_state = luaL_newstate();
    luaL_openlibs(process->lua_state);
    lua_newtable(process->lua_state); // Create the sys table
    // Register the process ID
    lua_pushinteger(process->lua_state, process->pid);
    lua_setfield(process->lua_state, -2, "pid");
    
    lua_pushstring(process->lua_state, process->source_file_node->path);
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
    return 1;
}

b8 lua_payload_passthrough(u16 code, void *sender, void *listener_inst, event_context data) {
    if (code != EVENT_LUA_CUSTOM) return false;
    
    char *event_name = data.data.c;
    
    for (int i = 0; i < lua_context.count; ++i) {
        LuaPayload *payload = &lua_context.payloads[i];
        if (payload->process == NULL)continue;
//        if (strcmp(payload->event_name, event_name) == 0) {
        lua_State *L = payload->process->lua_state; // Get your Lua state from wherever it's stored
        
        lua_rawgeti(L, LUA_REGISTRYINDEX, payload->callback_ref);
        
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            verror("Error executing Lua callback: %s", lua_tostring(L, -1));
            lua_pop(L, 1); // Remove error message
        }
//        }
    }
    
    return true;
}


void intrinsics_initialize() {
    event_register(EVENT_LUA_CUSTOM, 0, lua_payload_passthrough);
}

void intrinsics_shutdown() {
    event_unregister(EVENT_LUA_CUSTOM, 0, lua_payload_passthrough);
}