/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once

#include "kernel.h"
#include "core/vgui.h"

/**
 * @brief Initializes the intrinsics system.
 *
 * This function registers a custom event called `EVENT_LUA_CUSTOM` and sets `lua_payload_passthrough` as the event listener.
 * The `EVENT_LUA_CUSTOM` event is processed by the Lua system and allows for custom payloads.
 */
void intrinsics_initialize();

/**
 * Installs the necessary intrinsics to the given process.
 *
 * @param process The process to install the intrinsics to.
 *
 * @return Returns 1 on success, 0 otherwise.
 */
b8 intrinsics_install_to(Proc *process);

/**
 * @brief Shuts down the intrinsics system.
 *
 * This function unregisters the EVENT_LUA_CUSTOM event with the lua_payload_passthrough listener.
 *
 * @see event_unregister
 * @see EVENT_LUA_CUSTOM
 * @see lua_payload_passthrough
 *
 * @return void
 */
void intrinsics_shutdown();