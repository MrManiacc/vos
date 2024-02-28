/**
 * Created by jraynor on 2/27/2024.
 */
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "kernel_ext.h"
#include "core/vlogger.h"


// Takes in a function name and namespace, looks up the function, then calls it with the var args
int kernel_lua_call(lua_State *L) {
    lua_getglobal(L, "kernel");
    lua_getfield(L, -1, "kernel_ref");
    Kernel *kernel = lua_touserdata(L, -1);
    // use kernel_call to call the function after parsing the args
    const char *qualified_name = _strdup(lua_tostring(L, 1));
    Args args = kernel_proecss_lua_args(L, 2);
    // Assume the rest of the arguments are the arguments to the function
    if (args.count == 0) {
        kernel_call(kernel, qualified_name); // Call the function with no arguments
        return 0;
    }
    //Get the namespace by taking the first part of the qualified name
    const char *ns_name = strtok(qualified_name, ".");
    const char *func_name = strtok(NULL, ".");
    const Namespace *ns = kernel_namespace(kernel, ns_name);
    const Function *function = dict_get(ns->functions, func_name);
    if (function == null) {
        verror("Function %s does not exist in namespace %s", func_name, ns_name);
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
        lua_rawgeti(lprocess->lua_state, LUA_REGISTRYINDEX, function->context.lua.ref);
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
                // Add handling for other types as necessary
            }
        }
        // Call the function
        if (lua_pcall(lprocess->lua_state, args.count, 0, 0) != LUA_OK) {
            verror("Failed to call function %s: %s", function->signature.name, lua_tostring(lprocess->lua_state, -1));
        }
        return 1; // Return the number of values you've pushed onto the stack
    }


    // directly call the function here
}


int kernel_lua_namespace_define(lua_State *L) {
    // Defines a function in a namespace
    Namespace *ns = lua_touserdata(L, lua_upvalueindex(1));
    const char *query = lua_tostring(L, 1);
    Process *process = lua_touserdata(L, lua_upvalueindex(2));
    // Function* function =
    //Assumes the second argument is a reference to the function
    int callback_reference = luaL_ref(L, LUA_REGISTRYINDEX);
    Function *function = malloc(sizeof(Function));
    function->base = process;
    function->type = CALLABLE_TYPE_LUA;
    function->context.lua.ref = callback_reference;
    function->context.lua.lua_state = L;
    function->signature = kernel_process_create_signature(query);
    kernel_namespace_define(ns, function);
    return 1;
}

int kernel_lua_namespace(lua_State *L) {
    lua_getglobal(L, "kernel");
    lua_getfield(L, -1, "kernel_ref");
    Kernel *kernel = lua_touserdata(L, -1);
    const char *name = lua_tostring(L, 1);
    Namespace *ns = kernel_namespace(kernel, name);

    lua_newtable(L); // Create a table for namespace methods

    lua_pushlightuserdata(L, ns);
    // Instead of setting the namespace_ref in the table, we directly push it as an upvalue for the closure below

    lua_pushlightuserdata(L, kernel); // Assume you might need the Kernel reference as well
    // Push the kernel_lua_namespace_define function, setting ns and kernel as upvalues
    lua_pushcclosure(L, kernel_lua_namespace_define, 2);
    lua_setfield(L, -2, "define"); // Set the "define" field of the namespace methods table

    // Return the namespace methods table
    return 1;
}

void kernel_setup_lua_process_internal(Kernel *kernel, Process *process, lua_State *L) {
    // Create a table to store the kernel functions
    lua_newtable(L);
    lua_pushlightuserdata(L, kernel);
    lua_setfield(L, -2, "kernel_ref");

    lua_pushlightuserdata(L, process);
    lua_setfield(L, -2, "process_ref");

    lua_pushcfunction(L, kernel_lua_call);
    lua_setfield(L, -2, "call");

    lua_pushcfunction(L, kernel_lua_namespace);
    lua_setfield(L, -2, "namespace");

    // Add the kernel functions to the table
    lua_setglobal(L, "kernel");
}
