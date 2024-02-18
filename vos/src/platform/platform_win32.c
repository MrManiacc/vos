#include "core/vsemaphore.h"
#include "platform/platform.h"

// Windows platform layer.
#if KPLATFORM_WINDOWS

#include "containers/darray.h"
#include "core/vevent.h"
#include "core/vmem.h"
#include "core/vmutex.h"
#include "core/vstring.h"
#include "core/vthread.h"
#include "core/vlogger.h"
#include "filesystem/paths.h"

#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <windows.h>
#include <pathcch.h>
#include <stdio.h>

typedef struct win32_handle_info {
    HINSTANCE h_instance;
    HWND hwnd;
} win32_handle_info;

typedef struct win32_file_watch {
    u32 id;
    const char *file_path;
    FILETIME last_write_time;
} win32_file_watch;

typedef struct platform_state {
    win32_handle_info handle;
    CONSOLE_SCREEN_BUFFER_INFO std_output_csbi;
    CONSOLE_SCREEN_BUFFER_INFO err_output_csbi;
    // darray
    win32_file_watch *watches;
    f32 device_pixel_ratio;
} platform_state;

static platform_state *state_ptr;

// Clock
static f64 clock_frequency;
static LARGE_INTEGER start_time;

static void platform_update_watches(void);

//LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

void clock_setup(void) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64) frequency.QuadPart;
    QueryPerformanceCounter(&start_time);
}


b8 platform_initialize() {
    if (state_ptr) {
        return false;
    }
    SetConsoleOutputCP(65001);
    clock_setup();
    
    
    return true;
}

void platform_shutdown() {
    if (state_ptr && state_ptr->handle.hwnd) {
        DestroyWindow(state_ptr->handle.hwnd);
        state_ptr->handle.hwnd = 0;
    }
    if (state_ptr) {
        platform_free(state_ptr, false);
        state_ptr = 0;
    }
}

b8 platform_pump_messages(void) {
    if (state_ptr) {
        MSG message;
        while (PeekMessageA(&message, null, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    platform_update_watches();
    return true;
}

void *platform_allocate(u64 size, b8 aligned) {
    // return malloc(size);
    return (void *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void platform_free(void *block, b8 aligned) {
    // free(block);
    HeapFree(GetProcessHeap(), 0, block);
}

void *platform_zero_memory(void *block, u64 size) {
    return memset(block, 0, size);
}

void *platform_copy_memory(void *dest, const void *source, u64 size) {
    return memcpy(dest, source, size);
}

void *platform_set_memory(void *dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

b8 platform_is_debugger_attached(void) {
    return IsDebuggerPresent();
}

void platform_console_write(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (state_ptr) {
        csbi = state_ptr->std_output_csbi;
    } else {
        GetConsoleScreenBufferInfo(console_handle, &csbi);
    }
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[7] = {64, 4, 6, 2, 1, 8, 15};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    DWORD number_written = 0;
    WriteConsoleA(console_handle, message, (DWORD) length, &number_written, 0);
    
    SetConsoleTextAttribute(console_handle, csbi.wAttributes);
    
}

void platform_console_write_error(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (state_ptr) {
        csbi = state_ptr->err_output_csbi;
    } else {
        GetConsoleScreenBufferInfo(console_handle, &csbi);
    }
    
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[7] = {64, 4, 6, 2, 1, 8, 15};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    DWORD number_written = 0;
    WriteConsoleA(console_handle, message, (DWORD) length, &number_written, 0);
    
    SetConsoleTextAttribute(console_handle, csbi.wAttributes);
}

f64 platform_get_absolute_time(void) {
    if (!clock_frequency) {
        clock_setup();
    }
    
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64) now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms) {
    Sleep(ms);
}

i32 platform_get_processor_count(void) {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    vinfo("%i processor cores detected.", sysinfo.dwNumberOfProcessors);
    return sysinfo.dwNumberOfProcessors;
}

void platform_get_handle_info(u64 *out_size, void *memory) {
    *out_size = sizeof(win32_handle_info);
    if (!memory) {
        return;
    }
    
    kcopy_memory(memory, &state_ptr->handle, *out_size);
}

f32 platform_device_pixel_ratio(void) {
    return state_ptr->device_pixel_ratio;
}

// NOTE: Begin threads
b8 kthread_create(pfn_thread_start start_function_ptr, void *params, b8 auto_detach, kthread *out_thread) {
    if (!start_function_ptr) {
        return false;
    }
    
    out_thread->internal_data = CreateThread(
            0,
            0,                                           // Default stack size
            (LPTHREAD_START_ROUTINE) start_function_ptr,  // function ptr
            params,                                      // param to pass to thread
            0,
            (DWORD *) &out_thread->thread_id);
    vdebug("Starting process on thread id: %#x", out_thread->thread_id);
    if (!out_thread->internal_data) {
        return false;
    }
    if (auto_detach) {
        CloseHandle(out_thread->internal_data);
    }
    return true;
}

void kthread_destroy(kthread *thread) {
    if (thread && thread->internal_data) {
        DWORD exit_code;
        GetExitCodeThread(thread->internal_data, &exit_code);
        // if (exit_code == STILL_ACTIVE) {
        //     TerminateThread(thread->internal_data, 0);  // 0 = failure
        // }
        CloseHandle((HANDLE) thread->internal_data);
        thread->internal_data = 0;
        thread->thread_id = 0;
    }
}

void kthread_detach(kthread *thread) {
    if (thread && thread->internal_data) {
        CloseHandle(thread->internal_data);
        thread->internal_data = 0;
    }
}

void kthread_cancel(kthread *thread) {
    if (thread && thread->internal_data) {
        TerminateThread(thread->internal_data, 0);
        thread->internal_data = 0;
    }
}

b8 kthread_wait(kthread *thread) {
    if (thread && thread->internal_data) {
        DWORD exit_code = WaitForSingleObject(thread->internal_data, INFINITE);
        if (exit_code == WAIT_OBJECT_0) {
            return true;
        }
    }
    return false;
}

b8 kthread_wait_timeout(kthread *thread, u64 wait_ms) {
    if (thread && thread->internal_data) {
        DWORD exit_code = WaitForSingleObject(thread->internal_data, wait_ms);
        if (exit_code == WAIT_OBJECT_0) {
            return true;
        } else if (exit_code == WAIT_TIMEOUT) {
            return false;
        }
    }
    return false;
}

b8 kthread_is_active(kthread *thread) {
    if (thread && thread->internal_data) {
        DWORD exit_code = WaitForSingleObject(thread->internal_data, 0);
        if (exit_code == WAIT_TIMEOUT) {
            return true;
        }
    }
    return false;
}

void kthread_sleep(kthread *thread, u64 ms) {
    platform_sleep(ms);
}

u64 platform_current_thread_id(void) {
    return (u64) GetCurrentThreadId();
}

// NOTE: End threads.

// NOTE: Begin mutexes
b8 kmutex_create(kmutex *out_mutex) {
    if (!out_mutex) {
        return false;
    }
    out_mutex->internal_data = CreateMutex(0, 0, 0);
    if (!out_mutex->internal_data) {
        verror("Unable to create mutex.");
        return false;
    }
    vtrace("Created mutex.");
    return true;
}

void kmutex_destroy(kmutex *mutex) {
    if (mutex && mutex->internal_data) {
        CloseHandle(mutex->internal_data);
        // KTRACE("Destroyed mutex.");
        mutex->internal_data = 0;
    }
}

b8 kmutex_lock(kmutex *mutex) {
    if (!mutex) {
        return false;
    }
    
    DWORD result = WaitForSingleObject(mutex->internal_data, INFINITE);
    switch (result) {
        // The thread got ownership of the mutex
        case WAIT_OBJECT_0:
            // KTRACE("Mutex locked.");
            return true;
            
            // The thread got ownership of an abandoned mutex.
        case WAIT_ABANDONED:verror("Mutex lock failed.");
            return false;
    }
    // KTRACE("Mutex locked.");
    return true;
}

b8 kmutex_unlock(kmutex *mutex) {
    if (!mutex || !mutex->internal_data) {
        return false;
    }
    i32 result = ReleaseMutex(mutex->internal_data);
    // KTRACE("Mutex unlocked.");
    return result != 0;  // 0 is a failure
}

// NOTE: End mutexes.

b8 vsemaphore_create(vsemaphore *out_semaphore, u32 max_count, u32 start_count) {
    if (!out_semaphore) {
        return false;
    }
    
    out_semaphore->internal_data = CreateSemaphore(0, start_count, max_count, "semaphore");
    
    return true;
}

void vsemaphore_destroy(vsemaphore *semaphore) {
    if (semaphore && semaphore->internal_data) {
        CloseHandle(semaphore->internal_data);
        vtrace("Destroyed semaphore handle.");
        semaphore->internal_data = 0;
    }
}

b8 vsemaphore_signal(vsemaphore *semaphore) {
    if (!semaphore || !semaphore->internal_data) {
        return false;
    }
    // W: release/Increment
    LONG previous_count = 0;
    // NOTE: release 1 at a time.
    if (!ReleaseSemaphore(semaphore->internal_data, 1, &previous_count)) {
        verror("Failed to release semaphore.");
        return false;
    }
    return true;
    // L: post/Increment
}

b8 vsemaphore_wait(vsemaphore *semaphore, u64 timeout_ms) {
    if (!semaphore || !semaphore->internal_data) {
        return false;
    }
    
    DWORD result = WaitForSingleObject(semaphore->internal_data, timeout_ms);
    switch (result) {
        case WAIT_ABANDONED:verror(
                    "The specified object is a mutex object that was not released by the thread that owned the mutex object before the owning thread terminated. Ownership of the mutex object is granted to the calling thread and the mutex state is set to nonsignaled. If the mutex was protecting persistent state information, you should check it for consistency.");
            return false;
        case WAIT_OBJECT_0:
            // The state is signaled.
            return true;
        case WAIT_TIMEOUT:verror("Semaphore wait timeout occurred.");
            return false;
        case WAIT_FAILED:verror("WaitForSingleObject failed.");
            // TODO: GetLastError and print message.
            return false;
        default:verror("An unknown error occurred while waiting on a semaphore.");
            // TODO: GetLastError and print message.
            return false;
    }
    // W: wait/decrement, blocks when 0
    // L: wait/decrement, blocks when 0
    return true;
}

b8 platform_dynamic_library_load(const char *name, dynamic_library *out_library) {
    if (!out_library) {
        return false;
    }
    kzero_memory(out_library, sizeof(dynamic_library));
    if (!name) {
        return false;
    }
    
    char filename[MAX_PATH];
    kzero_memory(filename, sizeof(char) * MAX_PATH);
    string_format(filename, "%s.dll", name);
    
    HMODULE library = LoadLibraryA(filename);
    if (!library) {
        return false;
    }
    
    out_library->name = string_duplicate(name);
    out_library->filename = string_duplicate(filename);
    
    out_library->internal_data_size = sizeof(HMODULE);
    out_library->internal_data = library;
    
    out_library->functions = darray_create(dynamic_library_function);
    
    return true;
}

b8 platform_dynamic_library_unload(dynamic_library *library) {
    if (!library) {
        return false;
    }
    
    HMODULE internal_module = (HMODULE) library->internal_data;
    if (!internal_module) {
        return false;
    }
    
    if (library->name) {
        u64 length = string_length(library->name);
        kfree((void *) library->name, sizeof(char) * (length + 1), MEMORY_TAG_STRING);
    }
    
    if (library->filename) {
        u64 length = string_length(library->filename);
        kfree((void *) library->filename, sizeof(char) * (length + 1), MEMORY_TAG_STRING);
    }
    
    if (library->functions) {
        u32 count = darray_length(library->functions);
        for (u32 i = 0; i < count; ++i) {
            dynamic_library_function *f = &library->functions[i];
            if (f->name) {
                u64 length = string_length(f->name);
                kfree((void *) f->name, sizeof(char) * (length + 1), MEMORY_TAG_STRING);
            }
        }
        
        darray_destroy(library->functions);
        library->functions = 0;
    }
    
    BOOL result = FreeLibrary(internal_module);
    if (result == 0) {
        return false;
    }
    
    kzero_memory(library, sizeof(dynamic_library));
    
    return true;
}

b8 platform_dynamic_library_load_function(const char *name, dynamic_library *library) {
    if (!name || !library) {
        return false;
    }
    
    if (!library->internal_data) {
        return false;
    }
    
    FARPROC f_addr = GetProcAddress((HMODULE) library->internal_data, name);
    if (!f_addr) {
        return false;
    }
    
    dynamic_library_function f = {0};
    f.pfn = f_addr;
    f.name = string_duplicate(name);
    darray_push(dynamic_library_function, library->functions, f);
    
    return true;
}

const char *platform_dynamic_library_extension(void) {
    return ".dll";
}

const char *platform_dynamic_library_prefix(void) {
    return "";
}

platform_error_code platform_copy_file(const char *source, const char *dest, b8 overwrite_if_exists) {
    BOOL result = CopyFileA(source, dest, !overwrite_if_exists);
    if (!result) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            return PLATFORM_ERROR_FILE_NOT_FOUND;
        } else if (err == ERROR_SHARING_VIOLATION) {
            return PLATFORM_ERROR_FILE_LOCKED;
        } else {
            return PLATFORM_ERROR_UNKNOWN;
        }
    }
    return PLATFORM_ERROR_SUCCESS;
}

static b8 register_watch(const char *file_path, u32 *out_watch_id) {
    if (!state_ptr || !file_path || !out_watch_id) {
        if (out_watch_id) {
            *out_watch_id = INVALID_ID;
        }
        return false;
    }
    *out_watch_id = INVALID_ID;
    
    if (!state_ptr->watches) {
        state_ptr->watches = darray_create(win32_file_watch);
    }
    
    WIN32_FIND_DATAA data;
    HANDLE file_handle = FindFirstFileA(file_path, &data);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    BOOL result = FindClose(file_handle);
    if (result == 0) {
        return false;
    }
    
    u32 count = darray_length(state_ptr->watches);
    for (u32 i = 0; i < count; ++i) {
        win32_file_watch *w = &state_ptr->watches[i];
        if (w->id == INVALID_ID) {
            // Found a free slot to use.
            w->id = i;
            w->file_path = string_duplicate(file_path);
            w->last_write_time = data.ftLastWriteTime;
            *out_watch_id = i;
            return true;
        }
    }
    
    // If no empty slot is available, create and push a new entry.
    win32_file_watch w = {0};
    w.id = count;
    w.file_path = string_duplicate(file_path);
    w.last_write_time = data.ftLastWriteTime;
    *out_watch_id = count;
    darray_push(win32_file_watch, state_ptr->watches, w);
    
    return true;
}

static b8 unregister_watch(u32 watch_id) {
    if (!state_ptr || !state_ptr->watches) {
        return false;
    }
    
    u32 count = darray_length(state_ptr->watches);
    if (count == 0 || watch_id > (count - 1)) {
        return false;
    }
    
    win32_file_watch *w = &state_ptr->watches[watch_id];
    w->id = INVALID_ID;
    u32 len = string_length(w->file_path);
    kfree((void *) w->file_path, sizeof(char) * (len + 1), MEMORY_TAG_STRING);
    w->file_path = 0;
    kzero_memory(&w->last_write_time, sizeof(FILETIME));
    
    return true;
}

b8 platform_watch_file(const char *file_path, u32 *out_watch_id) {
    return register_watch(file_path, out_watch_id);
}

b8 platform_unwatch_file(u32 watch_id) {
    return unregister_watch(watch_id);
}

b8 platform_is_directory(const char *path) {
    //checks if the path is a directory on windows
    DWORD file_attributes = GetFileAttributesA(path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return (file_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

b8 platform_is_file(const char *path) {
    //checks if the path is a file on windows
    DWORD file_attributes = GetFileAttributesA(path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return (file_attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

b8 platform_file_exists(const char *path) {
    DWORD file_attributes = GetFileAttributesA(path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
}


u32 platform_file_size(const char *path) {
    HANDLE file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size)) {
        CloseHandle(file_handle);
        return 0;
    }
    
    CloseHandle(file_handle);
    return (u32) file_size.QuadPart;
}

void *platform_read_file(const char *path) {
    HANDLE file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    u32 file_size = platform_file_size(path);
    void *data = platform_allocate(file_size, false);
    if (!data) {
        CloseHandle(file_handle);
        return 0;
    }
    
    DWORD bytes_read;
    if (!ReadFile(file_handle, data, file_size, &bytes_read, 0)) {
        CloseHandle(file_handle);
        platform_free(data, false);
        return 0;
    }
    
    CloseHandle(file_handle);
    return data;
}

// Free the FilePathList and its contents
void file_path_list_free(VFilePathList *fileList) {
    for (int i = 0; i < fileList->count; i++) {
        free(fileList->paths[i]); // Free each string
    }
    free(fileList->paths); // Free the array of pointers
    free(fileList); // Free the structure itself
}


// Add a file path to the FilePathList
void file_path_list_add(VFilePathList *fileList, const char *path) {
    fileList->paths = realloc(fileList->paths, (fileList->count + 1) * sizeof(char *));
    if (!fileList->paths) {
        perror("Failed to realloc filePaths");
        exit(1); // Or handle error as appropriate
    }
    fileList->paths[fileList->count] = strdup(path); // Copy the path
    fileList->count++;
}

// Recursively collect files into the FilePathList
void collect_files_recursive(const char *base_path, VFilePathList *fileList) {
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", base_path);
    
    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return; // Directory not found or unable to open directory.
    }
    
    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue; // Skip the '.' and '..' directory entries.
        }
        
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", base_path, find_data.cFileName);
        file_path_list_add(fileList, full_path); // Add both files and folders
        
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // If it's a directory, recurse into it
            collect_files_recursive(full_path, fileList);
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
}


// Function to collect files directly within a given directory, non-recursively.
void collect_files_direct(const char *base_path, VFilePathList *fileList) {
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", base_path);
    
    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return; // Directory not found or unable to open directory.
    }
    
    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue; // Skip the '.' and '..' directory entries.
        }
        
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", base_path, find_data.cFileName);
        file_path_list_add(fileList, full_path);
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
}

// Create and initialize a FilePathList
VFilePathList *platform_collect_files_direct(const char *path) {
    VFilePathList *fileList = malloc(sizeof(VFilePathList));
    if (!fileList) {
        perror("Failed to allocate FilePathList");
        return NULL;
    }
    fileList->paths = NULL;
    fileList->count = 0;
    
    collect_files_direct(path, fileList);
    return fileList;
}

// Create and initialize a FilePathList
VFilePathList *platform_collect_files_recursive(const char *path) {
    VFilePathList *fileList = malloc(sizeof(VFilePathList));
    if (!fileList) {
        perror("Failed to allocate FilePathList");
        return NULL;
    }
    fileList->paths = NULL;
    fileList->count = 0;
    
    collect_files_recursive(path, fileList);
    return fileList;
}


char *platform_path(const char *path) {
    if (path == null) return null; // Added null check
    //if it doesn't start with a slash and contains :, it's a windows path so we return it
    if (path[0] != '/' && path[1] == ':') return string_duplicate(path);
    
    u32 len = strlen(path);
    char *platform_path = string_allocate_sized(path, len + 2);
    strcpy(platform_path, path);  // Copy the original path
    
    
    
    // Transform '/' to '\\'
    for (u32 i = 0; i < len; ++i) {
        if (platform_path[i] == '/') {
            platform_path[i] = '\\';
        }
    }
    
    // Handle drive letter
    if (len > 1 && platform_path[0] == '\\' && platform_path[1] != '\\') {
        platform_path[0] = platform_path[1];
        platform_path[1] = ':';
    }
    
    return platform_path;
}

char *platform_get_current_working_directory(void) {
    char *buffer = platform_allocate(MAX_PATH, false);
    u32 result = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (result == 0) {
        platform_free(buffer, false);
        return null;
    }
    return buffer;
}

char *platform_get_current_home_directory(void) {
    //gets the home directory on windows of the current user
    char *buffer = platform_allocate(MAX_PATH, false);
    u32 result = GetEnvironmentVariableA("USERPROFILE", buffer, MAX_PATH);
    if (result == 0) {
        platform_free(buffer, false);
        return null;
    }
    return buffer;
}

char *platform_parent_directory(const char *path) {
    if (path == null) {
        return null;
    }
    // if the path doesn't container \\ or /, it's a file name, so we return the current directory
    if (string_contains(path, "/") == -1 && string_contains(path, "\\") == -1) {
        return platform_get_current_working_directory();
    }
    
    //If we're here, we have a path, so we need to remove the last part of the path
    u32 len = string_length(path);
    char *parent_path = strdup(path);
    for (i32 i = len - 1; i >= 0; i--) {
        if (parent_path[i] == '/' || parent_path[i] == '\\') {
            parent_path[i] = '\0';
            break;
        }
    }
    return parent_path;
    
}

void *platform_reallocate(void *block, u64 size, b8 aligned) {
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, block, size);
}


static void platform_update_watches(void) {
    if (!state_ptr || !state_ptr->watches) {
        return;
    }
    
    u32 count = darray_length(state_ptr->watches);
    for (u32 i = 0; i < count; ++i) {
        win32_file_watch *f = &state_ptr->watches[i];
        if (f->id != INVALID_ID) {
            WIN32_FIND_DATAA data;
            HANDLE file_handle = FindFirstFileA(f->file_path, &data);
            if (file_handle == INVALID_HANDLE_VALUE) {
                // This means the file has been deleted, remove from watch.
                event_context context = {0};
                context.data.u32[0] = f->id;
                event_fire(EVENT_CODE_WATCHED_FILE_DELETED, 0, context);
                vinfo("File watch id %d has been removed.", f->id);
                unregister_watch(f->id);
                continue;
            }
            BOOL result = FindClose(file_handle);
            if (result == 0) {
                continue;
            }
            
            // Check the file time to see if it has been changed and update/notify if so.
            if (CompareFileTime(&data.ftLastWriteTime, &f->last_write_time) != 0) {
                f->last_write_time = data.ftLastWriteTime;
                // Notify listeners.
                event_context context = {0};
                context.data.u32[0] = f->id;
                event_fire(EVENT_CODE_WATCHED_FILE_WRITTEN, 0, context);
            }
        }
    }
}


#endif  // KPLATFORM_WINDOWS