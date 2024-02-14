/**
 * Created by jraynor on 8/24/2023.
 */
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "defines.h"
#include "kernel/vproc.h"
#include "kernel/kernel.h"
#include "core/vlogger.h"
#include "core/vevent.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"

#include "filesystem/paths.h"

#include "raylib.h"
#include "core/vstring.h"


// launch our bootstrap code
void startup_script_init() {
    //at this piont we know the file system is ready to go. Let's lookup the startup script and run it.
    char *startup_script = "boot.lua";
    //check if the file exists
    proc *proc = kernel_create_process(vfs_get(startup_script));
    if (proc == null) {
        verror("Failed to create process for startup script %s", startup_script)
        return;
    }
    //run the process
    process_start(proc);
}


int main(int argc, char **argv) {
    //If no arguments are passed, then we are running in the runtime and use the current working directory.
    //Otherwise, we are running in the editor and use the first argument as the root path.
    char *root_path = null;
    if (argc == 1) {
        root_path = getcwd(null, 0);
        // Append /assets to the root path.
        root_path = string_append(root_path, "/assets");
    } else if (argc == 2) {
        root_path = argv[1];
    }
    kernel_result result = kernel_initialize(root_path);
    if (!is_kernel_success(result.code)) {
        verror("Failed to initialize kernel: %s", get_kernel_result_message(result))
        return 1;
    }
    startup_script_init();
    lua_ctx(update)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(1280, 720, "VOS");
    SetTargetFPS(270);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGREEN);
        event_fire(EVENT_LUA_CUSTOM, null, update);
        kernel_poll_update();
        EndDrawing();
    }
    CloseWindow();
    kernel_result shutdown_result = kernel_shutdown();
    if (!is_kernel_success(shutdown_result.code)) {
        verror("Failed to shutdown kernel: %s", get_kernel_result_message(shutdown_result))
        return 1;
    }
    return 0;
}

#pragma clang diagnostic pop