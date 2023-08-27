/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

#include "kernel.h"

b8 initialize_syscalls();
b8 shutdown_syscalls();

b8 initialize_syscalls_for(Process *process);

