/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once
#include "defines.h"


void watcher_initialize(const char *path, b8 *running);

void watcher_shutdown();

//
//VAPI FileWatcher *file_watcher_create(const char *path, FileWatcherCallback callback);
//VAPI void file_watcher_destroy(FileWatcher *watcher);
//VAPI void file_watcher_poll(FileWatcher *watcher);
//
