/**
 * Created by jraynor on 2/12/2024.
 */
#pragma once


#include "lib/defines.h"

/**
 * @brief The data structure for a linear allocator.
 */
typedef struct linear_allocator {
    /** @brief The total size of memory in the allocator. */
    u64 total_size;
    /** @brief The amount of memory currently allocated. */
    u64 allocated;
    /** @brief The internal block of memory used by the allocator. */
    void* memory;
    /**
     * @brief Indicates if the allocator owns the memory (meaning it
     * performed the allocation itself) or whether it was provided by an outside source.
     */
    b8 owns_memory;
} linear_allocator;

/**
 * @brief Creates a linear allocator of the given size.
 *
 * @param total_size The total amount in bytes the allocator will hold.
 * @param memory Allocated block of memory matching size above, or 0. If 0, a dynamic allocation is performed
 * and this allocator is considered to own that memory.
 * @param out_allocator A pointer to hold the new allocator.
 */
VAPI void linear_allocator_create(u64 total_size, void* memory, linear_allocator* out_allocator);

/**
 * @brief Destroys the given allocator. If the allocator owns its memory, it is freed at this time.
 *
 * @param allocator A pointer to the allocator to be destroyed.
 */
VAPI void linear_allocator_destroy(linear_allocator* allocator);

/**
 * @brief Allocates the given amount from the allocator.
 *
 * @param allocator A pointer to the allocator to allocate from.
 * @param size The size to be allocated.
 * @return A pointer to a block of memory as allocated. If this fails, 0 is returned.
 */
VAPI void* linear_allocator_allocate(linear_allocator* allocator, u64 size);

/**
 * @brief Frees everything in the allocator, effectively moving its pointer back to the beginning.
 * Does not free internal memory, if owned. Only resets the pointer.
 *
 * @param allocator A pointer to the allocator to free.
 * @param clear Indicates whether or not to clear/zero the memory. Enabling this obviously takes more processing power.
 */
VAPI void linear_allocator_free_all(linear_allocator* allocator, b8 clear);