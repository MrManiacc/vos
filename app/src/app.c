/**
 * Created by jraynor on 8/24/2023.
 */
#include <string.h>
#include "defines.h"
#include "kernel/phost.h"
#include "kernel/kernel.h"
#include "core/logger.h"
#include "core/event.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"

#include "raylib.h"
int main(void) {
    KernelResult result = kernel_initialize();
    if (!is_kernel_success(result.code)) {
        verror("Failed to initialize kernel: %s", get_kernel_result_message(result))
        return 1;
    }

    KernelResult main_process_result = kernel_create_process("assets/env.lua");
    if (!is_kernel_success(main_process_result.code)) {
        verror("Failed to create process: %s", get_kernel_result_message(main_process_result))
        return 1;
    }

    ProcessID main_process = (ProcessID) main_process_result.data;
    KernelResult proc_lookup = kernel_lookup_process(main_process);
    if (!is_kernel_success(proc_lookup.code)) {
        verror("Failed to lookup process: %s", get_kernel_result_message(proc_lookup))
        return 1;
    }
    process_start(proc_lookup.data);
    int count = 0;
    lua_context(update)

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "VOS");
    SetTargetFPS(270 );
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        EndDrawing();
        event_fire(EVENT_LUA_CUSTOM, NULL, update);
    }

    CloseWindow();

    KernelResult shutdown_result = kernel_shutdown();
    if (!is_kernel_success(shutdown_result.code)) {
        verror("Failed to shutdown kernel: %s", get_kernel_result_message(shutdown_result))
        return 1;
    }
    return 0;
}
#pragma clang diagnostic pop