#pragma once

#include "defines.h"

typedef struct vsemaphore {
    void *internal_data;
} vsemaphore;

VAPI b8 vsemaphore_create(vsemaphore *out_semaphore, u32 max_count, u32 start_count);

VAPI void vsemaphore_destroy(vsemaphore *semaphore);

VAPI b8 vsemaphore_signal(vsemaphore *semaphore);

/**
 * Decreases the semaphore count by 1. If the count reaches 0, the
 * semaphore is considered unsignaled and this call blocks until the
 * semaphore is signaled by vsemaphore_signal.
 */
VAPI b8 vsemaphore_wait(vsemaphore *semaphore, u64 timeout_ms);

