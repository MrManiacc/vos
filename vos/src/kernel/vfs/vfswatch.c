#include <pthread.h>
#include "vfswatch.h"
#include "core/logger.h"
#include "core/timer.h"
#include "containers/dict.h"

static pthread_t watcher_thread;
static b8 *watcher_running;

//TODO: we need to implement a multi threaded file watcher and implement a way to manage files easily
#include <windows.h>

#define DEBOUNCE_DURATION 500 // 500 millisecond delay

struct FileWatcher {
  HANDLE directoryHandle;
  char path[MAX_PATH];
  FileWatcherCallback callback;
  char buffer[1024];
  OVERLAPPED overlapped;
};

FileWatcher *file_watcher_create(const char *path, FileWatcherCallback callback) {
    FileWatcher *watcher = (FileWatcher *) malloc(sizeof(FileWatcher));
    if (!watcher) return NULL;
    strncpy(watcher->path, path, MAX_PATH);
    watcher->callback = callback;
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

void file_watcher_poll(FileWatcher *watcher) {
    DWORD bytesReturned = 0;
    if (!*watcher_running) {
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

        FileEvent event;
        event.path = convertedPath;
        switch (info->Action) {
            case FILE_ACTION_ADDED:event.type = FILE_CREATED;
                break;
            case FILE_ACTION_REMOVED:event.type = FILE_DELETED;
                break;
            case FILE_ACTION_MODIFIED:event.type = FILE_MODIFIED;
                break;
            default:continue;
        }
        watcher->callback(event);
        info = (FILE_NOTIFY_INFORMATION *) ((char *) info + info->NextEntryOffset);
    }
    while (info->NextEntryOffset != 0);
}
void *watcher_thread_func(void *arg) {
    FileWatcher *watcher = (FileWatcher *) arg;
    while (*watcher_running) {
        file_watcher_poll(watcher);
    }
    //TODO: close the thread here ?
    vdebug("Shutting down watcher")
    file_watcher_destroy(watcher);
    pthread_join(watcher_thread, NULL);

}
static FileEvent last_event;

// Callback function for the timer to call after debounce.
void debounced_file_event(const char *id) {
    if (!*watcher_running)
        return;
    char *path = last_event.path;
    vdebug("File event after debounce: %s", path);
}

void on_file_event(FileEvent event) {
    last_event = event;
    timer_set(event.path, DEBOUNCE_DURATION, debounced_file_event);
}

static FileWatcher *watcher;

void watcher_initialize(const char *path, b8 *running) {
    watcher_running = running; //allow for the thread to be stopped
    watcher = file_watcher_create(path, on_file_event);
    pthread_create(&watcher_thread, NULL, watcher_thread_func, watcher);
}

void watcher_shutdown() {
    *watcher_running = false;

}