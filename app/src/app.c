/**
 * Created by jraynor on 8/24/2023.
 */
#include <string.h>
#include <io.h>
#include "defines.h"
#include "kernel/phost.h"
#include "kernel/kernel.h"
#include "core/logger.h"
#include "core/event.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"

#include "raylib.h"
#include "core/str.h"
int main(int argc, char **argv) {

    //If no arguments are passed, then we are running in the runtime and use the current working directory.
    //Otherwise, we are running in the editor and use the first argument as the root path.
    char *root_path = null;
    if (argc == 1) {
        root_path = getcwd(null, 0);
    } else if (argc == 2) {
        root_path = argv[1];
    }
    KernelResult result = kernel_initialize(root_path);
    if (!is_kernel_success(result.code)) {
        verror("Failed to initialize kernel: %s", get_kernel_result_message(result))
        return 1;
    }

//    Process *process = kernel_locate_process(string_format("%s/env.lua", root_path));
//    if (process == null) {
//        verror("Failed to locate process: %s", "env")
//        return 1;
//    }
//    process_start(process);
    lua_ctx(update)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(800, 600, "VOS");
    SetTargetFPS(270);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        event_fire(EVENT_LUA_CUSTOM, NULL, update);
        kernel_poll_update();
        EndDrawing();
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