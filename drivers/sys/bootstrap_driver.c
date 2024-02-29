/**
 * Created by jraynor on 2/25/2024.
 */
// ReSharper disable CppPointerConversionDropsQualifiers
#include <stdlib.h>
#include <string.h>

#include "kern/kernel_internal.h"
#include "containers/darray.h"
#include "core/vlogger.h"
#include "kern/kernel.h"
#include "core/vgui.h"
#include "core/vmem.h"
#include "platform/platform.h"

// Creates the driver entry point
$driver_entry("boot")

struct InternalState {
    void **renderers;
    void **lua_processes;
    void *defined_functions;
} InternalState;

static struct InternalState *state = null;

typedef struct NamespaceFunction {
    Namespace *ns;
    Function *func;
} NamespaceFunction;

b8 driver_create(const Namespace *ns) {
    state = kallocate(sizeof(InternalState), MEMORY_TAG_KERNEL);
    state->renderers = darray_create(Function*);
    state->lua_processes = darray_create(Process*);
    state->defined_functions = darray_create(NamespaceFunction);
    kernel_event_listen(kernel, EVENT_FUNCTION_DEFINED_IN_NAMESPACE, kernel_process_function_query(process, "on_defined(pointer)bool"));
    kernel_event_listen(kernel, EVENT_PROCESS_CREATED, kernel_process_function_query(process, "on_process_created(pointer)bool"));
    kernel_namespace_define_query(ns, process, "renderer(pointer)bool");
    kernel_namespace_define_query(ns, process, "color(u32;u32;u32;u32)pointer");
    kernel_namespace_define_query(ns, process, "rect(pointer;f32;f32;f32;f32;pointer)void");
    kernel_event_listen(kernel, EVENT_KERNEL_RENDER, kernel_process_function_query(process, "render(pointer)bool"));
    kernel_event_listen(kernel, EVENT_PROCESS_STARTED, kernel_process_function_query(process, "on_process_started(pointer)bool"));
    return true;
}


VAPI b8 renderer(Function* function) {
    // const Function *function = kernel_namespace_function_lookup(kernel, query);
    if (function == null) {
        verror("Failed to find function");
        return false;
    }
    darray_push(Function*, state->renderers, function);
    // vinfo("Registering render function, %s", query);
    return true;
}

VAPI b8 render(const EventData *data) {
    NVGcontext *ctx = (NVGcontext *) ((uintptr_t) data->f64[1]);
    for (u64 i = 0; i < darray_length(state->renderers); ++i) {
        const Function *allocation = *(Function **) darray_get(state->renderers, i);
        kernel_process_function_call(allocation, data->f64[0], ctx);
    }

    return false; // We didn't handle the event
}


int call_function(lua_State *L) {
    const Function *function = (Function *) lua_touserdata(L, lua_upvalueindex(1));
    Args args = kernel_proecss_lua_args(L, 1);
    if (args.count == 0) {
        kernel_process_function_call(function);
        return 0;
    }
    if (function == null) {
        verror("Function %s does not exist in namespace %s", function->signature.name);
        return 0;
    }

    if (function->base->type == PROCESS_TYPE_DRIVER && function->context.pfn) {
        ffi_cif cif;
        ffi_type **arg_types = malloc(sizeof(ffi_type *) * function->signature.arg_count);
        void **arg_values = malloc(sizeof(void *) * function->signature.arg_count);
        void *ret_value = malloc(sizeof(void *));

        // Prepare argument types and values
        for (u32 i = 0; i < function->signature.arg_count; ++i) {
            arg_types[i] = kernel_function_type_to_ffi_type(function->signature.args[i]);
            arg_values[i] = kernel_allocate_and_set_arg_value(function->signature.args[i], args.args[i]);
        }

        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, function->signature.arg_count, kernel_function_type_to_ffi_type(function->signature.return_type), arg_types) == FFI_OK) {
            ffi_call(&cif, FFI_FN(function->context.pfn), ret_value, arg_values);
        } else {
            verror("Failed to prepare FFI call for function %s", function->signature.name);
        }

        FunctionResult func_result;
        kernel_function_result_from_ffi_return(ret_value, function->signature.return_type, &func_result);
        free(ret_value);
        // Clean up
        for (u32 i = 0; i < function->signature.arg_count; ++i) {
            free(arg_values[i]);
        }
        free(arg_types);
        free(arg_values);
        //TODO: convert the function result in a table that can be returned
        // Now, convert and push the function result onto the Lua stack
        switch (func_result.type) {
            case FUNCTION_TYPE_I32:
                lua_pushinteger(L, func_result.data.i32);
                break;
            case FUNCTION_TYPE_F32:
                lua_pushnumber(L, func_result.data.f32);
                break;
            case FUNCTION_TYPE_F64:
                lua_pushnumber(L, func_result.data.f64);
                break;
            case FUNCTION_TYPE_U32:
                lua_pushinteger(L, func_result.data.u32);
                break;
            case FUNCTION_TYPE_U64:
                lua_pushinteger(L, func_result.data.u64);
                break;
            case FUNCTION_TYPE_BOOL:
                lua_pushboolean(L, func_result.data.boolean);
                break;
            case FUNCTION_TYPE_I64:
                lua_pushinteger(L, func_result.data.i64);
                break;
            case FUNCTION_TYPE_STRING:
                lua_pushstring(L, func_result.data.string);
                break;
            case FUNCTION_TYPE_POINTER:
                lua_pushlightuserdata(L, func_result.data.pointer);
                break;
            case FUNCTION_TYPE_ERROR:
                // Handle error appropriately, perhaps pushing nil or an error message
                lua_pushnil(L);
                break;
            default:
                lua_pushnil(L); // Fallback for unsupported or unknown types
                break;
        }
        return 1; // Return the number of values you've pushed onto the stack
    }
    if (function->base->type == PROCESS_TYPE_LUA) {
        LuaProcess *lprocess = (LuaProcess *) function->base;
        lua_rawgeti(function->context.lua.lua_state, LUA_REGISTRYINDEX, function->context.lua.ref);
        // Push the arguments to the Lua stack
        for (u32 i = 0; i < args.count; i++) {
            switch (args.args[i].type) {
                case ARG_INTEGER:
                    lua_pushinteger(lprocess->lua_state, args.args[i].value.i);
                    break;
                case ARG_NUMBER:
                    lua_pushnumber(lprocess->lua_state, args.args[i].value.f);
                    break;
                case ARG_STRING:
                    lua_pushstring(lprocess->lua_state, args.args[i].value.s);
                    break;
                case ARG_BOOLEAN:
                    lua_pushboolean(lprocess->lua_state, args.args[i].value.b);
                    break;
                case ARG_NIL:
                    lua_pushnil(lprocess->lua_state);
                    break;
                case ARG_POINTER:
                    lua_pushlightuserdata(lprocess->lua_state, args.args[i].value.p);
                    break;
                case ARG_FUNCTION:
                    lua_pushlightuserdata(lprocess->lua_state, args.args[i].value.fn);
                    break;
                // Add handling for other types as necessary
            }
        }
        // Call the function
        if (lua_pcall(lprocess->lua_state, args.count, 0, 0) != LUA_OK) {
            verror("Failed to call function %s: %s", function->signature.name, lua_tostring(lprocess->lua_state, -1));
        }
        return 1; // Return the number of values you've pushed onto the stack
    }
    return 0;
}

void create_lua_mappings_from_namespace(lua_State *L, const Namespace *ns, const Function *func) {
    const char *name = ns->name;
    const char *func_name = func->signature.name;
    lua_getglobal(L, name);
    if (lua_isnil(L, -1)) {
        lua_newtable(L);
        lua_setglobal(L, name);
        lua_getglobal(L, name);
    }
    lua_pushlightuserdata(L, (void *) func);
    lua_pushcclosure(L, call_function, 1);
    lua_setfield(L, -2, func_name);
    lua_pop(L, 1);
}


VAPI b8 on_defined(const EventData *data) {
    const Namespace *ns = (Namespace *) data->pointers.one;
    const Function *func = (Function *) data->pointers.two;
    vinfo("Function defined: %s", func->signature.name);
    const NamespaceFunction namespace_function = {
        .ns = ns,
        .func = func
    };
    darray_push(NamespaceFunction, state->defined_functions, namespace_function);
    if (darray_length(state->lua_processes) == 0) return false;
    darray_for_each(Process, state->lua_processes, proc) {
        if (proc->type == PROCESS_TYPE_LUA) {
            lua_State *L = ((LuaProcess *) proc)->lua_state;
            create_lua_mappings_from_namespace(L, ns, func);
        }
    }
    return false;
}

VAPI b8 on_process_created(const EventData *data) {
    const Process *process = data->pointers.one;
    if (process->type == PROCESS_TYPE_LUA) {
        darray_push(Process*, state->lua_processes, process);
        lua_State *L = ((LuaProcess *) process)->lua_state;
        darray_for_each(NamespaceFunction, state->defined_functions, func) {
            const Namespace *ns = func->ns;
            const Function *function = func->func;
            create_lua_mappings_from_namespace(L, ns, function);
        }
        vinfo("Process created: %s", process->name);
    }
    return false;
}

VAPI b8 on_process_started(const EventData *data) {
    const Process *process = data->pointers.one;
    if (process->type == PROCESS_TYPE_LUA) {
        lua_State *L = ((LuaProcess *) process)->lua_state;
        darray_for_each(NamespaceFunction, state->defined_functions, func) {
            const Namespace *ns = func->ns;
            const Function *function = func->func;
            create_lua_mappings_from_namespace(L, ns, function);
        }
        lua_getglobal(L, "on_start");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                verror("Failed to call on_start: %s", lua_tostring(L, -1));
            }
        }
    }
    return false;
}


b8 driver_destroy(const Namespace *ns) {
    vinfo("Destroying process");
    return true;
}
