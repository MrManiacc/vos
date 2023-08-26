#include <stdio.h>
#include "kresult.h"
#include "core/mem.h"
#include "phost.h"

/**
 * Determines if the kernel operation was successful.
 * @param code The error code.
 * @return TRUE if the operation was successful, else FALSE.
 */
b8 is_kernel_success(KernelCode code) {
    return code >= KERNEL_SUCCESS;
}

/**
 * Gets the result message for a kernel operation.
 * @param result The result of the kernel operation.
 * @return
 */
const char *get_kernel_result_message(KernelResult result) {
    char *message = kallocate(256, MEMORY_TAG_KERNEL);
    switch (result.code) {
        case KERNEL_SUCCESS:
            if (result.data != null)
                sprintf(message, "The kernel operation was successful.");
            else
                sprintf(message, "The kernel operation was successful: %s", (char *) result.data);
            break;
        case KERNEL_ALREADY_INITIALIZED:sprintf(message, "The kernel has already been initialized.");
            break;
        case KERNEL_ALREADY_SHUTDOWN:sprintf(message, "The kernel has already been shutdown.");
            break;
        case KERNEL_CALL_BEFORE_INIT:sprintf(message, "The kernel has not been initialized.");
            break;
        case KERNEL_PROCESS_CREATED:sprintf(message, "The process was successfully created.");
            break;
        case KERNEL_PROCESS_NOT_FOUND:
            sprintf(message,
                    "The process with id %d was not found.",
                    (ProcessID) result.data);
            break;
        case KERNEL_ERROR:sprintf(message, "Kernel error: %s", (char *) result.data);
            break;
        default:sprintf(message, "An unknown kernel error occurred, this may be gibberish: %s", (char *) result.data);
            break;
    }
    return message;
}