/**
 * Created by jraynor on 2/19/2024.
 */
#ifdef _WIN32

#include <windows.h>
#include "logger.h"
#include "platform.h"
#include "mutex.h"

// NOTE: Begin mutexes
b8 vmutex_create(vmutex *out_mutex) {
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

void vmutex_destroy(vmutex *mutex) {
    if (mutex && mutex->internal_data) {
        CloseHandle(mutex->internal_data);
        // KTRACE("Destroyed mutex.");
        mutex->internal_data = 0;
    }
}

b8 vmutex_lock(vmutex *mutex) {
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

b8 vmutex_unlock(vmutex *mutex) {
    if (!mutex || !mutex->internal_data) {
        return false;
    }
    i32 result = ReleaseMutex(mutex->internal_data);
    // KTRACE("Mutex unlocked.");
    return result != 0;  // 0 is a failure
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

void *platform_reallocate(void *block, u64 size, b8 aligned) {
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, block, size);
}


void platform_console_write(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(console_handle, &csbi);
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
    GetConsoleScreenBufferInfo(console_handle, &csbi);
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[7] = {64, 4, 6, 2, 1, 8, 15};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    DWORD number_written = 0;
    WriteConsoleA(console_handle, message, (DWORD) length, &number_written, 0);
    
    SetConsoleTextAttribute(console_handle, csbi.wAttributes);
}


#ifdef __APPLE__

void *platform_reallocate(void *block, u64 size, b8 aligned) {
    return realloc(block, size);
}

void *platform_allocate(unsigned long long int size, b8 aligned) {
    return malloc(size);
}

void *platform_reallocate(void *block, unsigned long long int size, b8 aligned) {
    return realloc(block, size);
}

void platform_free(void *block, b8 aligned) {
    free(block);
}

void *platform_zero_memory(void *block, unsigned long long int size) {
    return memset(block, 0, size);
}

void *platform_copy_memory(void *dest, const void *source, unsigned long long int size) {
    return memcpy(dest, source, size);
}

void *platform_set_memory(void *dest, int value, unsigned long long int size) {
    return memset(dest, value, size);
}
void platform_console_write(const char *message, u8 colour) {
    // ANSI Color Codes
    static const char *colour_strings[] = {
            "\033[0;41m", // Red background for fatal
            "\033[1;31m", // Bright Red for errors
            "\033[1;33m", // Bright Yellow for warnings
            "\033[1;32m", // Bright Green for info
            "\033[1;34m", // Bright Blue for debug
            "\033[0m",    // Reset to default for trace and general messages
    };

    // Check if colour is within range to avoid accessing out of bounds
    const char *colour_code = (colour < sizeof(colour_strings) / sizeof(colour_strings[0]))
                              ? colour_strings[colour]
                              : "\033[0m"; // Default to reset if out of bounds

    printf("%s%s\033[0m", colour_code, message); // Reset color at the end
}
#endif

#endif

#ifdef __linux__

#endif
