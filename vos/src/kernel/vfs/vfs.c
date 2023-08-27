#include "vfs.h"
#include "core/mem.h"
#include "containers/dict.h"
#include "core/logger.h"
#include "vfswatch.h"
#include "core/timer.h"

static Vfs *vfs;
static b8 *vfs_initialized;

KernelResult vfs_initialize(char *root_path) {

//    vfs = kallocate(sizeof(Vfs), MEMORY_TAG_VFS);
    vfs_initialized = kallocate(sizeof(b8), MEMORY_TAG_VFS);
    *vfs_initialized = true;
//    vfs->r = kallocate(strlen(root_path) + 1, MEMORY_TAG_VFS);

    vdebug("Initializing with path: %s", root_path)
    watcher_initialize(root_path, vfs_initialized);
//    create thread to poll on
//    pthread_create(&watcher_thread, NULL, watcher_thread_func, watcher);

    return (KernelResult) {KERNEL_SUCCESS, null};
}

KernelResult vfs_shutdown() {
//    should_run = false;
    watcher_shutdown();
//    kfree(vfs_initialized, sizeof(b8), MEMORY_TAG_VFS);
//    kfree(vfs, sizeof(Vfs), MEMORY_TAG_VFS);

    return (KernelResult) {KERNEL_SUCCESS, null};
}