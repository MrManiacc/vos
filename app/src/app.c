///**
// * Created by jraynor on 8/24/2023.
// */
//#include <string.h>
//
//#ifdef _WIN32
//
//#include <io.h>
//#include <stdio.h>
//
//#else
//#include <unistd.h>
//#include <printf.h>
//
//#endif
//
#include <stdio.h>
#include "defines.h"
#include "kernel/proc.h"
#include "kernel/kernel.h"
#include "core/vlogger.h"
#include "core/vevent.h"
#include "filesystem/paths.h"
#include "core/vgui.h"
#include "platform/platform.h"
#include "core/vstring.h"

// //
// //
// ////// launch our bootstrap code
// void startup_script_init(Kernel *kernel) {
//     //at this piont we know the file system is ready to go. Let's lookup the startup script and run it.
//     char *startup_script = "boot.lua";
//     //check if the file exists
//     Proc *proc = kernel_create_process(vfs_node_get(kernel->fs_context, startup_script), true);
//     if (proc == null) {
//         verror("Failed to create process for startup script %s", startup_script)
//         return;
//     }
//     //run the process
//     process_start(proc);
// }
//
// int main(int argc, char **argv) {
//     char *root_path = path_locate_root();
//     vdebug("Root path: %s", root_path)
//     KernelResult result = kernel_initialize(root_path);
//     if (!kernel_is_result_success(result.code)) {
//         verror("Failed to initialize kernel: %s", kernel_get_result_message(result))
//         return 1;
//     }
//     Kernel *kernel = result.data;
//     startup_script_init(kernel);
//     lua_ctx(update)
//     WindowContext *window = kernel->window_context;
//     gui_load_font(kernel, "sys/fonts/JetBrainsMono-Bold.ttf", "sans");
//     while (!window_should_close(window)) {
//         window_begin_frame(window);
//         event_fire(kernel, EVENT_LUA_CUSTOM, null, update);
//         window_end_frame(window);
//         kernel_poll_update(kernel);
//     }
//     window_shutdown(window);
//     KernelResult shutdown_result = kernel_shutdown();
//     if (!kernel_is_result_success(shutdown_result.code)) {
//         verror("Failed to shutdown kernel: %s", kernel_get_result_message(shutdown_result))
//         return 1;
//     }
//     //waits for user input
//     return 0;
// }

//
//
////void visit_component(ASTNode *node, Visitor *visitor) {
////    vdebug("Visiting component: %s", node->data.component.name)
////
////}
//
//// testing the lexer
//int main(int argc, char **argv) {
//    char *root_path = path_locate_root();
//    vdebug("Root path: %s", root_path)
//    KernelResult result = kernel_initialize(root_path);
////    if (!kernel_is_result_success(result.code)) {
////        verror("Failed to initialize kernel: %s", kernel_get_result_message(result))
////        return 1;
////    }
////    startup_script_init();
//
//    FsNode *gui = vfs_node_get("sys/gui/testing.mgl");
//
//    if (gui == null) {
//        verror("Failed to load gui file")
//        return 1;
//    }
//
//    ProgramSource lexerResult = lexer_analysis_from_mem(gui->data.file.data, gui->data.file.size);
//    char *tokens_dump = lexer_dump_tokens(&lexerResult);
//    vdebug("Tokens: %s", tokens_dump)
//    ProgramAST ast = parser_parse(&lexerResult);
//
//    char *ast_dump = parser_dump(&ast);
//    vinfo("AST: \n%s", ast_dump);
//
//    PassManager *state = muil_pass_manager_new();
//    SymtabPass *symbols = new_SymtabPass();
//    ReferencesPass *references = new_ReferencesPass();
//    TypePass *types = new_TypePass();
//    muil_pass_manager_add(state, (SemanticsPass *) symbols, PASS_EXECUTE_CONCURRENT);
//    muil_pass_manager_add(state, (SemanticsPass *) references, PASS_EXECUTE_CONSECUTIVE);
//    muil_pass_manager_add(state, (SemanticsPass *) types, PASS_EXECUTE_CONSECUTIVE);
//    muil_pass_manager_run(state, &ast);
//    muil_pass_manager_destroy(state);
//    parser_free_program(&ast);
//    delete_SymtabPass(symbols);
//    delete_TypePass(types);
//    delete_ReferencesPass(references);
//    KernelResult shutdown_result = kernel_shutdown();
//
//
//    if (!kernel_is_result_success(shutdown_result.code)) {
//        verror("Failed to shutdown kernel: %s", kernel_get_result_message(shutdown_result))
//        return 1;
//    }
//    // wait for input
////    getchar();
//    return 0;
//}


