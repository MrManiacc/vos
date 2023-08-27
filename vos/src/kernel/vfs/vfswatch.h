/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once
#include "defines.h"

typedef enum {
  FILE_CREATED,
  FILE_MODIFIED,
  FILE_DELETED,
} FileEventType;

typedef struct {
  FileEventType type;
  char *path; // Path to the changed file.
} FileEvent;

typedef void (*FileWatcherCallback)(FileEvent event);

typedef struct FileWatcher FileWatcher;

void watcher_initialize(const char *path, b8 *running);

void watcher_shutdown();

//
//VAPI FileWatcher *file_watcher_create(const char *path, FileWatcherCallback callback);
//VAPI void file_watcher_destroy(FileWatcher *watcher);
//VAPI void file_watcher_poll(FileWatcher *watcher);
//
