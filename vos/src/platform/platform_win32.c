#include "platform/platform.h"
// Windows platform layer.
#if KPLATFORM_WINDOWS

#include "core/logger.h"
#include "core/event.h"

#include "containers/darray.h"

#include <windows.h>
#include <windowsx.h>  // param input extraction
#include <stdlib.h>


typedef struct internal_state {
} internal_state;

// Clock
static f64 clock_frequency;
static LARGE_INTEGER start_time;


b8 platform_startup(
    platform_state *plat_state,
    const char *application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {

    // Clock setup
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    return TRUE;
}

void platform_shutdown(platform_state *plat_state) {
    // Simply cold-cast to the known type.
    internal_state *state = (internal_state *)plat_state->internal_state;


}

b8 platform_pump_messages(platform_state *plat_state) {
    return TRUE;
}

void *platform_allocate(u64 size, b8 aligned) {
    return malloc(size);
}

void platform_free(void *block, b8 aligned) {
    free(block);
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

void platform_console_write(const char* str, u8 color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    //FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 levels[7] = {132, 140, 142, 143, 139, 141, 0};
    SetConsoleTextAttribute(hConsole, levels[color]);
    u64 length = strlen(str);
    LPDWORD written = 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), str, (DWORD) length, written, NULL);
}

void platform_console_write_error(const char* str, u8 color) {
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    //FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 levels[7] = {132, 140, 142, 143, 139, 141, 0};
    SetConsoleTextAttribute(hConsole, levels[color]);
    u64 length = strlen(str);
    LPDWORD written = 0;
    WriteConsole(GetStdHandle(STD_ERROR_HANDLE), str, (DWORD) length, written, NULL);
}

f64 platform_get_absolute_time() {
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms) {
    Sleep(ms);
}

#endif  // KPLATFORM_WINDOWS
