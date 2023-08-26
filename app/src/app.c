/**
 * Created by jraynor on 8/24/2023.
 */
#include "defines.h"
#include "kernel/phost.h"
#include "kernel/kernel.h"
#include "core/logger.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"
int main(void) {
//    Process* root_process = process_create("assets/scripts/main.lua");
//
//    Process *child_process = process_create("assets/scripts/child.lua");
//    process_add_child(root_process, child_process);
//    process_start(root_process);
//
//    process_destroy(root_process);

    KernelResult result = kernel_initialize();
    if (!is_kernel_success(result.code)) {
        verror("Failed to initialize kernel: %s", get_kernel_result_message(result))
        return 1;
    }

    KernelResult main_process = kernel_create_process("assets/scripts/main.lua");
    if (!is_kernel_success(main_process.code)) {
        verror("Failed to create process: %s", get_kernel_result_message(main_process))
        return 1;
    }

    KernelResult child_process = kernel_create_process("assets/scripts/child.lua");
    if (!is_kernel_success(child_process.code)) {
        verror("Failed to create process: %s", get_kernel_result_message(child_process))
        return 1;
    }

    KernelResult attach_result = kernel_attach_process((ProcessID) child_process.data, (ProcessID) main_process.data);
    if (!is_kernel_success(attach_result.code)) {
        verror("Failed to attach process: %s", get_kernel_result_message(attach_result))
        return 1;
    }

    KernelResult shutdown_result = kernel_shutdown();
    if (!is_kernel_success(shutdown_result.code)) {
        verror("Failed to shutdown kernel: %s", get_kernel_result_message(shutdown_result))
        return 1;
    }
    return 0;
}
#pragma clang diagnostic pop