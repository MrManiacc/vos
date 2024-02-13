#include "asset_watcher.h"

//#include <pthread.h>
#include "core/vlogger.h"
#include "core/vtimer.h"
#include "containers/dict.h"
#include "core/vstring.h"
#include "core/vmem.h"
#include "core/vevent.h"

typedef struct AssetWatcher {
//  pthread_t thread;
  b8 *running;
  dict *watched_events;
  struct FileWatcher *watcher;
} AssetWatcher;

//static pthread_t watcher_thread;
//static b8 *watcher_running;
//static Map *watched_events;

//TODO: we need to implement a multi threaded file watcher and implement a way to manage files easily
#include <windows.h>

#define DEBOUNCE_DURATION 500 // 500 millisecond delay
typedef enum {
  FILE_CREATED = 10,
  FILE_MODIFIED = 11,
  FILE_DELETED = 12
} FileEventType;

typedef struct {
  FileEventType type;
  char *path; // Path to the changed file.
} FileEvent;

typedef struct FileWatcher {
  HANDLE directoryHandle;
  char path[MAX_PATH];
  char buffer[1024];
  OVERLAPPED overlapped;
} FileWatcher;

// Callback function for the timer to call after debounce.
void debounced_file_event(void *raw_id);

FileWatcher *file_watcher_create(const char *path) {
    FileWatcher *watcher = (FileWatcher *) malloc(sizeof(FileWatcher));
    if (!watcher) return NULL;
    strncpy(watcher->path, path, MAX_PATH);
    watcher->directoryHandle = CreateFileA(
        path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (watcher->directoryHandle == INVALID_HANDLE_VALUE) {
        free(watcher);
        return NULL;
    }

    memset(&watcher->overlapped, 0, sizeof(OVERLAPPED));
    return watcher;
}

void file_watcher_destroy(FileWatcher *watcher) {
    if (watcher) {
        CloseHandle(watcher->directoryHandle);
        free(watcher);
    }
}

char *wide_to_ansi(const WCHAR *wide, int wide_len) {
    int len = WideCharToMultiByte(CP_ACP, 0, wide, wide_len, NULL, 0, NULL, NULL);
    char *ansi = (char *) malloc(len + 1);  // +1 for null terminator
    WideCharToMultiByte(CP_ACP, 0, wide, wide_len, ansi, len, NULL, NULL);
    ansi[len] = '\0';  // Null terminate the string
    return ansi;
}
typedef struct timer_data {
  FileEvent *event;
  AssetWatcher *watcher;
} timer_data;

void file_watcher_poll(AssetWatcher *asset_watcher) {
    FileWatcher *watcher = asset_watcher->watcher;
    DWORD bytesReturned = 0;
    if (!asset_watcher->running) {
        vwarn("Watcher is not running")
        return;
    }
    if (!ReadDirectoryChangesW(
        watcher->directoryHandle,
        watcher->buffer,
        sizeof(watcher->buffer),
        TRUE,
        FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME,
        NULL, // NULL because we are using overlapped
        &watcher->overlapped,
        NULL
    )) {
        // Handle error, for example:
        vdebug("ReadDirectoryChangesW failed with %d\n", GetLastError());
        return;
    }

    if (!GetOverlappedResult(watcher->directoryHandle, &watcher->overlapped, &bytesReturned, TRUE)) {
        // No changes detected, just return
        vdebug("No changes");
        return;
    }
//
//    // In a real-world scenario, you'd want to handle these changes asynchronously.
//    // Here, we're simplifying by polling.
//    SleepEx(0, TRUE);

    FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *) watcher->buffer;

    do {
        int fileNameLen = info->FileNameLength / sizeof(WCHAR);  // Convert byte length to WCHAR length
        char *convertedPath = wide_to_ansi(info->FileName, fileNameLen);

        // Check if the filename ends with ~ and if so, ignore it.
        int pathLen = strlen(convertedPath);
        if (pathLen > 0 && convertedPath[pathLen - 1] == '~') {
            free(convertedPath);
            continue;
        }

        FileEvent *event = kallocate(sizeof(FileEvent), MEMORY_TAG_VFS);
        event->path = convertedPath;
        if (timer_exists(event->path) || dict_get(asset_watcher->watched_events, event->path))
            continue;
        switch (info->Action) {
            case FILE_ACTION_ADDED:event->type = FILE_CREATED;
                break;
            case FILE_ACTION_REMOVED:event->type = FILE_DELETED;
                break;
            case FILE_ACTION_MODIFIED:event->type = FILE_MODIFIED;
                break;
            default:continue;
        }
        //Wait for a second
        struct timer_data *data = kallocate(sizeof(struct timer_data), MEMORY_TAG_VFS);
        data->event = event;
        data->watcher = asset_watcher;
        timer_set(event->path, 1000, debounced_file_event, data);
        info = (FILE_NOTIFY_INFORMATION *) ((char *) info + info->NextEntryOffset);
    }
    while (info->NextEntryOffset != 0);
}

void *watcher_thread_func(void *arg) {
    AssetWatcher *watcher = (AssetWatcher *) arg;
    while (*watcher->running) {
        file_watcher_poll(watcher);
    }
    vdebug("Shutting down watcher")
    file_watcher_destroy(watcher->watcher);
//    pthread_join(watcher->thread, NULL);
    free(watcher);
}

//static FileWatcher *watcher;

AssetWatcher *asset_watcher_initialize(const char *path, b8 *running) {
//    vdebug("Initializing watcher at %s", path)
//    AssetWatcher *watcher = kallocate(sizeof(AssetWatcher), MEMORY_TAG_VFS);
//    watcher->running = running; //allow for the thread to be stopped
//    watcher->watcher = file_watcher_create(path);
//    watcher->watched_events = dict_create_default();
//    pthread_create(&watcher->thread, NULL, watcher_thread_func, watcher);
}

void asset_watcher_shutdown(AssetWatcher *watcher) {
    *watcher->running = false;
    dict_destroy(watcher->watched_events);
}

void debounced_file_event(void *raw_watcher) {
//    //Wait for a second
//    timer_data *data = (timer_data *) raw_watcher;
//    AssetWatcher *watcher = data->watcher;
//    FileEvent *event = data->event;
//    if (!*watcher->running)
//        return;
//    if (!event)
//        return;
//    //build path to file
//    char *pathed = string_concat("/", event->path);
//    char *path = string_concat(watcher->watcher->path, string_replace(pathed, "\\", "/"));
//    kfree(pathed, string_length(pathed) + 1, MEMORY_TAG_STRING);
//    //check if the file still exists
//    event_context event_context;
//    memcpy(event_context.data.c, path, strlen(path));
//    //null terminate the string
//    event_context.data.c[strlen(path)] = '\0';
//    vdebug("File event after debounce: %s", path)
//    switch (event->type) {
//        case FILE_CREATED:event_fire(EVENT_FILE_CREATED, NULL, event_context);
//            break;
//        case FILE_MODIFIED:event_fire(EVENT_FILE_MODIFIED, NULL, event_context);
//            break;
//        case FILE_DELETED:event_fire(EVENT_FILE_DELETED, NULL, event_context);
//            break;
//    }
//    kfree(event->path, string_length(path) + 1, MEMORY_TAG_STRING);
//    //free the event_content data
//    kfree(event, sizeof(FileEvent), MEMORY_TAG_VFS);
////    vdebug("File event after debounce: %s", path);
}