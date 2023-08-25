/**
 * Created by jraynor on 8/24/2023.
 */
#include "defines.h"
#include "kernel/phost.h"
#include "kernel/kernel.h"
#include "core/logger.h"

int main(void){
    Process* root_process = process_create("assets/scripts/main.lua");

    Process *child_process = process_create("assets/scripts/child.lua");
    process_add_child(root_process, child_process);
    process_start(root_process);

    process_destroy(root_process);

    return 0;
}