/**
* Created by jraynor on 2/27/2024.
 */
#include <assert.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>
#include "containers/dict.h"
#include "core/vlogger.h"
#include "kern/kernel_internal.h"


FunctionSignature kernel_process_create_signature(const char *query) {
    FunctionSignature signature = {0};
    const char *open_paren = strchr(query, '(');
    if (open_paren == NULL) {
        signature.name = strdup(query); // strdup to allocate and copy the name
        signature.return_type = FUNCTION_TYPE_VOID; // Assuming default return type is void if not specified
        return (FunctionSignature){.return_type = FUNCTION_TYPE_ERROR};
    }

    // Extract function name
    size_t name_len = open_paren - query;
    char *func_name = (char *) malloc(name_len + 1);
    strncpy(func_name, query, name_len);
    func_name[name_len] = '\0';
    signature.name = func_name;

    // Extract arguments and return type
    const char *close_paren = strchr(open_paren, ')');
    if (close_paren == NULL) {
        verror("Invalid function signature. No closing ) found.");
        free(func_name);
        return (FunctionSignature){.return_type = FUNCTION_TYPE_ERROR};
    }

    // Process arguments
    char *args_str = (char *) malloc(close_paren - open_paren);
    strncpy(args_str, open_paren + 1, close_paren - open_paren - 1);
    args_str[close_paren - open_paren - 1] = '\0';

    char *arg = strtok(args_str, ";");
    while (arg != NULL && signature.arg_count < MAX_FUNCTION_ARGS) {
        signature.args[signature.arg_count++] = kernel_string_to_function_type(arg);
        arg = strtok(NULL, ";");
    }

    free(args_str);

    // Process return type
    const char *return_type_str = close_paren + 1;
    if (*return_type_str == '\0') {
        // No return type specified
        signature.return_type = FUNCTION_TYPE_VOID;
    } else {
        signature.return_type = kernel_string_to_function_type(return_type_str);
    }
    return signature;
}

void *kernel_allocate_arg_value(FunctionType type, va_list *args) {
    void *value;
    switch (type) {
        case FUNCTION_TYPE_I32: {
            int *arg = malloc(sizeof(i32));
            *arg = va_arg(*args, int);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_F32: {
            float *arg = malloc(sizeof(f32));
            *arg = va_arg(*args, f32);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_F64: {
            double *arg = malloc(sizeof(f64));
            *arg = va_arg(*args, f64);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_I64: {
            i64 *arg = malloc(sizeof(i64));
            *arg = va_arg(*args, i64);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_U32: {
            u32 *arg = malloc(sizeof(u32));
            *arg = va_arg(*args, u32);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_U64: {
            u64 *arg = malloc(sizeof(u64));
            *arg = va_arg(*args, u64);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_STRING: {
            void **arg = malloc(sizeof(char *));
            *arg = va_arg(*args, char*);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_POINTER: {
            void **arg = malloc(sizeof(void *));
            *arg = va_arg(*args, void*);
            value = arg;
            break;
        }
        case FUNCTION_TYPE_BOOL: {
            b8 *arg = malloc(sizeof(b8));
            *arg = va_arg(*args, b8);
            value = arg;
            break;
        }
        // Add cases for other types as necessary
        default:
            value = NULL;
            break;
    }

    return value;
}

FunctionType kernel_string_to_function_type(const char *type_str) {
    if (strcmp(type_str, "f32") == 0) return FUNCTION_TYPE_F32;
    if (strcmp(type_str, "f64") == 0) return FUNCTION_TYPE_F64;
    if (strcmp(type_str, "u32") == 0) return FUNCTION_TYPE_U32;
    if (strcmp(type_str, "u64") == 0) return FUNCTION_TYPE_U64;
    if (strcmp(type_str, "i32") == 0) return FUNCTION_TYPE_I32;
    if (strcmp(type_str, "bool") == 0) return FUNCTION_TYPE_BOOL;
    if (strcmp(type_str, "i64") == 0) return FUNCTION_TYPE_I64;
    if (strcmp(type_str, "void") == 0) return FUNCTION_TYPE_VOID;
    if (strcmp(type_str, "pointer") == 0) return FUNCTION_TYPE_POINTER;
    if (strcmp(type_str, "string") == 0) return FUNCTION_TYPE_STRING;
    return FUNCTION_TYPE_ERROR;
}

ffi_type *kernel_function_type_to_ffi_type(const FunctionType type) {
    switch (type) {
        case FUNCTION_TYPE_I32:
            return &ffi_type_sint;
        case FUNCTION_TYPE_F32:
            return &ffi_type_float;
        case FUNCTION_TYPE_F64:
            return &ffi_type_double;
        case FUNCTION_TYPE_I64:
            return &ffi_type_sint64;
        case FUNCTION_TYPE_U32:
            return &ffi_type_uint;
        case FUNCTION_TYPE_U64:
            return &ffi_type_uint64;
        case FUNCTION_TYPE_BOOL:
            return &ffi_type_uchar;
        case FUNCTION_TYPE_STRING:
            return &ffi_type_pointer;
        case FUNCTION_TYPE_POINTER:
            return &ffi_type_pointer;
        case FUNCTION_TYPE_VOID:
            return &ffi_type_void;
        default:
            return NULL;
    }
}

void kernel_function_result_from_ffi_return(void *ret_value, const FunctionType return_type, FunctionResult *result) {
    switch (return_type) {
        case FUNCTION_TYPE_I32:
            result->type = FUNCTION_TYPE_I32;
            result->data.i32 = *(i32 *) ret_value;
            break;
        case FUNCTION_TYPE_F32:
            result->type = FUNCTION_TYPE_F32;
            result->data.f32 = *(f32 *) ret_value;
            break;
        case FUNCTION_TYPE_F64:
            result->type = FUNCTION_TYPE_F64;
            result->data.f64 = *(f64 *) ret_value;
            break;
        case FUNCTION_TYPE_I64:
            result->type = FUNCTION_TYPE_I64;
            result->data.i64 = *(i64 *) ret_value;
            break;
        case FUNCTION_TYPE_U32:
            result->type = FUNCTION_TYPE_U32;
            result->data.u32 = *(u32 *) ret_value;
            break;
        case FUNCTION_TYPE_U64:
            result->type = FUNCTION_TYPE_U64;
            result->data.u64 = *(u64 *) ret_value;
            break;
        case FUNCTION_TYPE_BOOL:
            result->type = FUNCTION_TYPE_BOOL;
            result->data.boolean = *(b8 *) ret_value;
            break;
        case FUNCTION_TYPE_STRING:
            result->type = FUNCTION_TYPE_STRING;
            result->data.string = *(const char **) ret_value;
            break;
        case FUNCTION_TYPE_POINTER:
            result->type = FUNCTION_TYPE_POINTER;
            result->data.pointer = *(void **) ret_value;
            break;
        case FUNCTION_TYPE_VOID:
            result->type = FUNCTION_TYPE_VOID;
            break;
        default:
            result->type = FUNCTION_TYPE_ERROR;
            result->data.error = "Invalid return type";
            break;
    }
}


// A helper function to convert the value on the top of the stack to a LuaArgs struct
Args kernel_proecss_lua_args(lua_State *L, const int argStartIndex) {
    Args args = {0};
    // Starting index for arguments is 2 since 1 is the function name
    // Correctly set the count based on the total number of arguments on the stack
    // minus the first one (the function name), not the passed count which may be incorrect
    args.count = lua_gettop(L); // Subtracting the function name from the total count

    for (u32 i = 0; i < args.count; i++) {
        int stackIndex = argStartIndex + i;
        if (lua_isinteger(L, stackIndex)) {
            args.args[i].type = ARG_INTEGER;
            args.args[i].value.i = lua_tointeger(L, stackIndex);
        } else if (lua_islightuserdata(L, stackIndex)) {
            args.args[i].type = ARG_POINTER;
            args.args[i].value.p = lua_touserdata(L, stackIndex);
        } else if (lua_isnumber(L, stackIndex)) {
            args.args[i].type = ARG_NUMBER;
            args.args[i].value.f = lua_tonumber(L, stackIndex);
        } else if (lua_isstring(L, stackIndex)) {
            args.args[i].type = ARG_STRING;
            args.args[i].value.s = lua_tostring(L, stackIndex);
        } else if (lua_isboolean(L, stackIndex)) {
            args.args[i].type = ARG_BOOLEAN;
            args.args[i].value.b = lua_toboolean(L, stackIndex);
        } else if (lua_isnil(L, stackIndex)) {
            args.args[i].type = ARG_NIL;
        } else if (lua_isfunction(L, stackIndex)) {
            Function *fn = malloc(sizeof(Function));
            args.args[i].type = ARG_FUNCTION;
            lua_getglobal(L, "kernel");
            lua_getfield(L, -1, "process_ref");
            Process *process = lua_touserdata(L, -1);
            lua_pop(L, 2);
            fn->base = process;
            fn->context.lua.ref = luaL_ref(L, stackIndex);
            fn->context.lua.lua_state = L;
            fn->signature = (FunctionSignature){
                //TODO: maybe look up the function signature in the namespace?
                .name = "anonymous",
                .arg_count = 2,
                .args[0] = FUNCTION_TYPE_F64,
                .args[1] = FUNCTION_TYPE_POINTER,
                .return_type = FUNCTION_TYPE_VOID
            };
            fn->type = CALLABLE_TYPE_LUA;
            args.args[i].value.fn = fn;
        }
        // Add handling for other types as necessary
    }
    return args;
}


void *kernel_allocate_and_set_arg_value(FunctionType type, LuaArg larg) {
    void *value;
    switch (type) {
        case FUNCTION_TYPE_I32: {
            int *arg = malloc(sizeof(i32));
            *arg = larg.value.i;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_F32: {
            float *arg = malloc(sizeof(f32));
            *arg = larg.value.f;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_F64: {
            double *arg = malloc(sizeof(f64));
            *arg = larg.value.f;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_I64: {
            i64 *arg = malloc(sizeof(i64));
            *arg = larg.value.i;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_U32: {
            u32 *arg = malloc(sizeof(u32));
            *arg = larg.value.i;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_U64: {
            u64 *arg = malloc(sizeof(u64));
            *arg = larg.value.i;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_STRING: {
            void **arg = malloc(sizeof(char *));
            *arg = larg.value.s;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_POINTER: {
            void **arg = malloc(sizeof(void *));
            *arg = larg.value.p;
            value = arg;
            break;
        }
        case FUNCTION_TYPE_BOOL: {
            b8 *arg = malloc(sizeof(b8));
            *arg = larg.value.b;
            value = arg;
            break;
        }
    }
    return value;
}

void kernel_push_lua_args(lua_State *L, FunctionSignature signature, va_list args) {
    for (int i = 0; i < signature.arg_count; i++) {
        switch (signature.args[i]) {
            case FUNCTION_TYPE_U32:
            case FUNCTION_TYPE_U64:
            case FUNCTION_TYPE_I64:
            case FUNCTION_TYPE_I32: {
                const int value = va_arg(args, int);
                lua_pushinteger(L, value);
                break;
            }
            case FUNCTION_TYPE_F64:
            case FUNCTION_TYPE_F32: {
                // Assuming float values are promoted to double in va_arg
                const double value = va_arg(args, double);
                lua_pushnumber(L, value);
                break;
            }
            case FUNCTION_TYPE_STRING: {
                const char *value = va_arg(args, char*);
                lua_pushstring(L, value);
                break;
            }
            case FUNCTION_TYPE_POINTER: {
                void *value = va_arg(args, void*);
                lua_pushlightuserdata(L, value);
                break;
            }
            case FUNCTION_TYPE_BOOL: {
                const b8 value = va_arg(args, b8);
                lua_pushboolean(L, value);
                break;
            }
            // Add more cases for other types as needed
            default:
                // Handle unknown type or error
                verror("Unknown type")
                break;
        }
    }
}
