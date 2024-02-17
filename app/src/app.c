/**
 * Created by jraynor on 8/24/2023.
 */
#include <string.h>

#ifdef _WIN32

#include <io.h>
#include <stdio.h>

#else
#include <unistd.h>
#include <printf.h>

#endif

#include "defines.h"
#include "kernel/vproc.h"
#include "kernel/kernel.h"
#include "core/vlogger.h"
#include "core/vevent.h"


#include "filesystem/paths.h"

#include "muil/muil.h"

// launch our bootstrap code
void startup_script_init() {
    //at this piont we know the file system is ready to go. Let's lookup the startup script and run it.
    char *startup_script = "boot.lua";
    //check if the file exists
    Proc *proc = kernel_create_process(vfs_node_get(startup_script));
    if (proc == null) {
        verror("Failed to create process for startup script %s", startup_script)
        return;
    }
    //run the process
    process_start(proc);
}


//void visit_component(ASTNode *node, Visitor *visitor) {
//    vdebug("Visiting component: %s", node->data.component.name)
//
//}

// testing the lexer
int main(int argc, char **argv) {
    char *root_path = path_locate_root();
    vdebug("Root path: %s", root_path)
    KernelResult result = kernel_initialize(root_path);
//    if (!kernel_is_result_success(result.code)) {
//        verror("Failed to initialize kernel: %s", kernel_get_result_message(result))
//        return 1;
//    }
//    startup_script_init();
    
    FsNode *gui = vfs_node_get("sys/gui/testing.mgl");
    
    if (gui == null) {
        verror("Failed to load gui file")
        return 1;
    }
    
    ProgramSource lexerResult = lexer_analysis_from_mem(gui->data.file.data, gui->data.file.size);
    char *tokens_dump = lexer_dump_tokens(&lexerResult);
    vdebug("Tokens: %s", tokens_dump)
    ProgramAST ast = parser_parse(&lexerResult);
    
    char *ast_dump = parser_dump(&ast);
    printf("AST: \n%s", ast_dump);
    parser_free_program(&ast);
    
    KernelResult shutdown_result = kernel_shutdown();
    
    
    if (!kernel_is_result_success(shutdown_result.code)) {
        verror("Failed to shutdown kernel: %s", kernel_get_result_message(shutdown_result))
        return 1;
    }
    return 0;
}

//int main(int argc, char **argv) {
//    char *root_path = path_locate_root();
//    vdebug("Root path: %s", root_path)
//    KernelResult result = kernel_initialize(root_path);
//    if (!kernel_is_result_success(result.code)) {
//        verror("Failed to initialize kernel: %s", kernel_get_result_message(result))
//        return 1;
//    }
//    startup_script_init();
//    lua_ctx(update)
//
//    if (!window_initialize("VOS", 1600, 900)) {
//        verror("Failed to initialize window context");
//        return 1;
//    }
//    gui_load_font("sys/fonts/JetBrainsMono-Bold.ttf", "sans");
//    while (!window_should_close()) {
//        window_begin_frame();
//        event_fire(EVENT_LUA_CUSTOM, null, update);
//        kernel_poll_update();
//        window_end_frame();
//    }
//    window_shutdown();
//    KernelResult shutdown_result = kernel_shutdown();
//    if (!kernel_is_result_success(shutdown_result.code)) {
//        verror("Failed to shutdown kernel: %s", kernel_get_result_message(shutdown_result))
//        return 1;
//    }
//    return 0;
//}
//
