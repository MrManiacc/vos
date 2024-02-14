/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

#include "kernel.h"

b8 install_lua_intrinsics(proc *process);


void intrinsics_initialize();

void intrinsics_shutdown();