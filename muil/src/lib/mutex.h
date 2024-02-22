/**
 * Created by jraynor on 2/19/2024.
 */
#pragma once

#include "defines.h"

/**
 * A mutex to be used for synchronization purposes. A mutex (or
 * mutual exclusion) is used to limit access to a resource when
 * there are multiple threads of execution around that resource.
 */
typedef struct kmutex {
    void *internal_data;
} vmutex;

/**
 * Creates a mutex.
 * @param out_mutex A pointer to hold the created mutex.
 * @returns True if created successfully; otherwise false.
 */
VAPI b8 vmutex_create(vmutex *out_mutex);

/**
 * @brief Destroys the provided mutex.
 *
 * @param mutex A pointer to the mutex to be destroyed.
 */
VAPI void vmutex_destroy(vmutex *mutex);

/**
 * Creates a mutex lock.
 * @param mutex A pointer to the mutex.
 * @returns True if locked successfully; otherwise false.
 */
VAPI b8 vmutex_lock(vmutex *mutex);

/**
 * Unlocks the given mutex.
 * @param mutex The mutex to unlock.
 * @returns True if unlocked successfully; otherwise false.
 */
VAPI b8 vmutex_unlock(vmutex *mutex);