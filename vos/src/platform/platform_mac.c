#ifdef __APPLE__
#include "platform.h"
#include "core/vsemaphore.h"
#include "core/vmutex.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/syslimits.h>
#include <semaphore.h>
#include <fcntl.h> // For O_CREAT, O_EXEC
#include <time.h>
#include <errno.h>

b8 vsemaphore_wait_with_timeout(sem_t *semaphore, u64 timeout_ms) {
    if (!semaphore) {
        return false;
    }

    struct timespec ts;
    ts.tv_sec = timeout_ms / 1000; // Convert milliseconds to seconds
    ts.tv_nsec = (timeout_ms % 1000) * 1000000; // Convert remainder to nanoseconds

    struct timespec rem;
    while (timeout_ms > 0) {
        if (sem_trywait(semaphore) == 0) {
            return true; // Successfully acquired the semaphore
        }

        // If the semaphore is not available, we sleep for a short interval
        // Here, we use a short, hard-coded sleep time (e.g., 10ms)
        struct timespec req = {0, 10 * 1000000}; // 10ms
        if (nanosleep(&req, &rem) == -1 && errno == EINTR) {
            // Adjust the timeout based on the remaining time if interrupted
            ts.tv_sec = rem.tv_sec;
            ts.tv_nsec = rem.tv_nsec;
            timeout_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        } else {
            // Decrease the timeout by the sleep time
            timeout_ms -= 10;
        }

        if (timeout_ms <= 0) {
            // If we've exhausted the timeout, return false
            return false;
        }
    }

    return false; // Timeout occurred
}

typedef struct platform_state {
    b8 quit_flagged;
} platform_state;

static platform_state *state_ptr;

typedef enum {
    KEY_NUMPAD0,
    KEY_NUMPAD1,
    KEY_NUMPAD2,
    KEY_NUMPAD3,
    KEY_NUMPAD4,
    KEY_NUMPAD5,
    KEY_NUMPAD6,
    KEY_NUMPAD7,
    KEY_NUMPAD8,
    KEY_NUMPAD9,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_COMMA,
    KEY_GRAVE,
    KEY_MINUS,
    KEY_PERIOD,
    KEY_SEMICOLON,
    KEY_SLASH,
    KEY_BACKSPACE,
    KEY_CAPITAL,
    KEY_DELETE,
    KEY_DOWN,
    KEY_END,
    KEY_ENTER,
    KEY_ESCAPE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_PRINT,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_HOME,
    KEY_INSERT,
    KEY_LEFT,
    KEY_LALT,
    KEY_LCONTROL,
    KEY_LSHIFT,
    KEY_LWIN,
    KEY_NUMLOCK,
    KEY_PAGE_DOWN,
    KEY_PAGE_UP,
    KEY_RIGHT,
    KEY_RALT,
    KEY_RCONTROL,
    KEY_RSHIFT,
    KEY_RWIN,
    KEY_SPACE,
    KEY_TAB,
    KEY_UP,
    KEY_ADD,
    KEY_DECIMAL,
    KEY_DIVIDE,
    KEY_NUMPAD_EQUAL,
    KEY_MULTIPLY,
    KEY_SUBTRACT,
    KEYS_MAX_KEYS
} keys;

keys translate_keycode(unsigned int ns_keycode);

typedef struct {
    void *instance;
    void *allocator;
    void *surface;
} vulkan_context;

void platform_get_required_extension_names(const char ***names_darray);

f64 platform_get_absolute_time(void){
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (f64)time.tv_sec + (f64)time.tv_nsec / 1000000000.0;
}

b8 platform_system_startup(u64 *memory_requirement, void *state, void *config) {
    *memory_requirement = sizeof(platform_state);
    if (state == 0) {
        return true;
    }
    state_ptr = state;
    state_ptr->quit_flagged = false;
    return true;
}

void platform_system_shutdown(void *platform_state) {
    state_ptr = 0;
}

b8 platform_pump_messages(void) {
    if (state_ptr) {
        return !state_ptr->quit_flagged;
    }
    return true;
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


// Linux version https://stackoverflow.com/questions/3596781/how-to-detect-if-the-console-is-a-terminal-in-unix-in-c
// Mac version https://stackoverflow.com/questions/2200277/detecting-debugger-on-mac-os-x
b8 platform_is_debugger_attached(void) {
    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.

    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.

    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.

    return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}

void platform_console_write_error(const char *message, u8 colour) {
    platform_console_write(message, colour);//just for now ?
}


keys translate_keycode(unsigned int ns_keycode) {
    switch (ns_keycode) {
        case 0x1D:
            return KEY_NUMPAD0;
        case 0x12:
            return KEY_NUMPAD1;
        case 0x13:
            return KEY_NUMPAD2;
        case 0x14:
            return KEY_NUMPAD3;
        case 0x15:
            return KEY_NUMPAD4;
        case 0x17:
            return KEY_NUMPAD5;
        case 0x16:
            return KEY_NUMPAD6;
        case 0x1A:
            return KEY_NUMPAD7;
        case 0x1C:
            return KEY_NUMPAD8;
        case 0x19:
            return KEY_NUMPAD9;
        case 0x00:
            return KEY_A;
        case 0x0B:
            return KEY_B;
        case 0x08:
            return KEY_C;
        case 0x02:
            return KEY_D;
        case 0x0E:
            return KEY_E;
        case 0x03:
            return KEY_F;
        case 0x05:
            return KEY_G;
        case 0x04:
            return KEY_H;
        case 0x22:
            return KEY_I;
        case 0x26:
            return KEY_J;
        case 0x28:
            return KEY_K;
        case 0x25:
            return KEY_L;
        case 0x2E:
            return KEY_M;
        case 0x2D:
            return KEY_N;
        case 0x1F:
            return KEY_O;
        case 0x23:
            return KEY_P;
        case 0x0C:
            return KEY_Q;
        case 0x0F:
            return KEY_R;
        case 0x01:
            return KEY_S;
        case 0x11:
            return KEY_T;
        case 0x20:
            return KEY_U;
        case 0x09:
            return KEY_V;
        case 0x0D:
            return KEY_W;
        case 0x07:
            return KEY_X;
        case 0x10:
            return KEY_Y;
        case 0x06:
            return KEY_Z;
        case 0x27:
            return KEYS_MAX_KEYS; // Apostrophe
        case 0x2A:
            return KEYS_MAX_KEYS; // Backslash
        case 0x2B:
            return KEY_COMMA;
        case 0x18:
            return KEYS_MAX_KEYS; // Equal
        case 0x32:
            return KEY_GRAVE;
        case 0x21:
            return KEYS_MAX_KEYS; // Left bracket
        case 0x1B:
            return KEY_MINUS;
        case 0x2F:
            return KEY_PERIOD;
        case 0x1E:
            return KEYS_MAX_KEYS; // Right bracket
        case 0x29:
            return KEY_SEMICOLON;
        case 0x2C:
            return KEY_SLASH;
        case 0x0A:
            return KEYS_MAX_KEYS; // ?
        case 0x33:
            return KEY_BACKSPACE;
        case 0x39:
            return KEY_CAPITAL;
        case 0x75:
            return KEY_DELETE;
        case 0x7D:
            return KEY_DOWN;
        case 0x77:
            return KEY_END;
        case 0x24:
            return KEY_ENTER;
        case 0x35:
            return KEY_ESCAPE;
        case 0x7A:
            return KEY_F1;
        case 0x78:
            return KEY_F2;
        case 0x63:
            return KEY_F3;
        case 0x76:
            return KEY_F4;
        case 0x60:
            return KEY_F5;
        case 0x61:
            return KEY_F6;
        case 0x62:
            return KEY_F7;
        case 0x64:
            return KEY_F8;
        case 0x65:
            return KEY_F9;
        case 0x6D:
            return KEY_F10;
        case 0x67:
            return KEY_F11;
        case 0x6F:
            return KEY_F12;
        case 0x69:
            return KEY_PRINT;
        case 0x6B:
            return KEY_F14;
        case 0x71:
            return KEY_F15;
        case 0x6A:
            return KEY_F16;
        case 0x40:
            return KEY_F17;
        case 0x4F:
            return KEY_F18;
        case 0x50:
            return KEY_F19;
        case 0x5A:
            return KEY_F20;
        case 0x73:
            return KEY_HOME;
        case 0x72:
            return KEY_INSERT;
        case 0x7B:
            return KEY_LEFT;
        case 0x3A:
            return KEY_LALT;
        case 0x3B:
            return KEY_LCONTROL;
        case 0x38:
            return KEY_LSHIFT;
        case 0x37:
            return KEY_LWIN;
        case 0x6E:
            return KEYS_MAX_KEYS; // Menu
        case 0x47:
            return KEY_NUMLOCK;
        case 0x79:
            return KEYS_MAX_KEYS; // Page down
        case 0x74:
            return KEYS_MAX_KEYS; // Page up
        case 0x7C:
            return KEY_RIGHT;
        case 0x3D:
            return KEY_RALT;
        case 0x3E:
            return KEY_RCONTROL;
        case 0x3C:
            return KEY_RSHIFT;
        case 0x36:
            return KEY_RWIN;
        case 0x31:
            return KEY_SPACE;
        case 0x30:
            return KEY_TAB;
        case 0x7E:
            return KEY_UP;
        case 0x52:
            return KEY_NUMPAD0;
        case 0x53:
            return KEY_NUMPAD1;
        case 0x54:
            return KEY_NUMPAD2;
        case 0x55:
            return KEY_NUMPAD3;
        case 0x56:
            return KEY_NUMPAD4;
        case 0x57:
            return KEY_NUMPAD5;
        case 0x58:
            return KEY_NUMPAD6;
        case 0x59:
            return KEY_NUMPAD7;
        case 0x5B:
            return KEY_NUMPAD8;
        case 0x5C:
            return KEY_NUMPAD9;
        case 0x45:
            return KEY_ADD;
        case 0x41:
            return KEY_DECIMAL;
        case 0x4B:
            return KEY_DIVIDE;
        case 0x4C:
            return KEY_ENTER;
        case 0x51:
            return KEY_NUMPAD_EQUAL;
        case 0x43:
            return KEY_MULTIPLY;
        case 0x4E:
            return KEY_SUBTRACT;
        default:
            return KEYS_MAX_KEYS;
    }
}


b8 platform_is_directory(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}


b8 platform_is_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

b8 platform_file_exists(const char *path) {
    return access(path, F_OK) != -1;
}


u32 platform_file_size(const char *path) {
    struct stat stat_buf;
    if (stat(path, &stat_buf) == 0) {
        return (u32) stat_buf.st_size;
    }
    return 0;
}

void *platform_read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    void *data = malloc(file_size);
    if (!data) {
        fclose(file);
        return NULL;
    }

    fread(data, 1, file_size, file);
    fclose(file);

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


// Function to add a file path to the FilePathList
void file_path_list_add(VFilePathList *fileList, const char *path) {
    fileList->paths = realloc(fileList->paths, (fileList->count + 1) * sizeof(char *));
    if (!fileList->paths) {
        perror("Failed to realloc filePaths");
        exit(1); // Or handle error as appropriate
    }
    fileList->paths[fileList->count] = strdup(path); // Copy the path
    fileList->count++;
}

// macOS version of recursively collecting files into the FilePathList
void collect_files_recursive(const char *base_path, VFilePathList *fileList) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(base_path)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", base_path, entry->d_name);

        // Add to list
        file_path_list_add(fileList, full_path);

        // Check if it's a directory and recurse
        struct stat path_stat;
        stat(full_path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
            collect_files_recursive(full_path, fileList);
        }
    }

    closedir(dir);
}

// macOS version of collecting files directly within a given directory, non-recursively.
void collect_files_direct(const char *base_path, VFilePathList *fileList) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(base_path)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", base_path, entry->d_name);

        // Add to list
        file_path_list_add(fileList, full_path);
    }

    closedir(dir);
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
    return strdup(path);
}

char *platform_get_current_home_directory(void) {
    return strdup(getenv("HOME"));
}

// gets the current directory of the executable. equivalent to the GetCurrentDirectoryA function in windows
char *platform_get_current_working_directory(void) {
    char *buffer = malloc(PATH_MAX);
    if (getcwd(buffer, PATH_MAX) == NULL) {
        perror("getcwd");
        return NULL;
    }
    return buffer;
}

char *platform_parent_directory(const char *path) {
    char *parent = strdup(path);
    char *last_slash = strrchr(parent, '/');
    if (last_slash) {
        *last_slash = '\0';
    }
    return parent;
}

b8 vsemaphore_create(vsemaphore *out_semaphore, u32 max_count, u32 start_count) {
    if (!out_semaphore) return false;

    // Use sem_open to create or open a named semaphore
    // Since macOS might not fully support unnamed semaphores via sem_init
    char sem_name[16];
    snprintf(sem_name, sizeof(sem_name), "/sem-%p", (void *) out_semaphore);
    out_semaphore->internal_data = sem_open(sem_name, O_CREAT | O_EXCL, 0644, start_count);
    if (out_semaphore->internal_data == SEM_FAILED) {
        // Handle error
        return false;
    }

    // Attempt to unlink immediately to allow system cleanup upon deletion
    sem_unlink(sem_name);

    return true;
}

void vsemaphore_destroy(vsemaphore *semaphore) {
    if (semaphore && semaphore->internal_data != SEM_FAILED) {
        sem_close(semaphore->internal_data);
    }
}

b8 vsemaphore_signal(vsemaphore *semaphore) {
    if (!semaphore || semaphore->internal_data == SEM_FAILED) {
        return false;
    }
    return sem_post(semaphore->internal_data) == 0;
}

b8 vsemaphore_wait(vsemaphore *semaphore, u64 timeout_ms) {
    if (!semaphore || semaphore->internal_data == SEM_FAILED) {
        return false;
    }

    if (timeout_ms == 0) {
        // Wait indefinitely
        return sem_wait(semaphore->internal_data) == 0;
    } else {
        // For timed wait, macOS supports sem_timedwait
        return vsemaphore_wait_with_timeout(semaphore->internal_data, timeout_ms);
    }
}

b8 kmutex_create(kmutex *out_mutex) {
//    if (!out_mutex) return false;
//
//    // Initialize the mutex with default attributes
//    if (pthread_mutex_init(&out_mutex->internal_data, NULL) != 0) {
//        // Handle error
//        return false;
//    }
    return true;
}

void kmutex_destroy(kmutex *mutex) {
//    if (mutex) {
//        pthread_mutex_destroy(mutex->internal_data);
//    }
}

b8 kmutex_lock(kmutex *mutex) {
    if (!mutex) return false;
//    return pthread_mutex_lock(mutex->internal_data) == 0;
    return true;
}

b8 kmutex_unlock(kmutex *mutex) {
    if (!mutex) return false;
//    return pthread_mutex_unlock(mutex->internal_data) == 0;
    return true;
}

#endif // __APPLE__