#include "mem.h"

#include "core/logger.h"
#include "core/str.h"
#include "platform/platform.h"
#include "containers/darray.h"

// TODO: Custom string lib
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct memory_stats {
  u64 total_allocated;
  u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
  void *tagged_allocation_pointers[MEMORY_TAG_MAX_TAGS];
};

static const char *memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "VFS        ",
    "DARRAY     ",
    "KERNEL     ",
    "RING_QUEUE ",
    "PROCESS    ",
    "STRING     ",
    "APPLICATION",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      "};

static struct memory_stats stats;

void initialize_memory() {
    platform_zero_memory(&stats, sizeof(stats));
    // Inside initialize_memory()

}

void shutdown_memory() {
//    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; i++) {
//        for (u32 j = 0; j < darray_length(stats.tagged_allocation_pointers[i]); j++) {
//            void *ptr;
//            darray_pop_at(stats.tagged_allocation_pointers[i], j, &ptr);
////            kfree(ptr, darray_stride(stats.tagged_allocation_pointers[i]), i);
//            if (i == MEMORY_TAG_STRING) {
//                vdebug("[0x%04x] Left over string - %s", ptr, (char *) ptr);
////                platform_free(ptr, false);
//            } else if (i == MEMORY_TAG_DARRAY) {
//                vdebug("[0x%04x] Left over darray size: %d, stride: %d", ptr, darray_length(ptr), darray_stride(ptr));
////                kfree(ptr, darray_stride(stats.tagged_allocation_pointers[i]), i);
////                platform_free(ptr, false);
//
////                platform_free(ptr, false);
//
//            } else if (i == MEMORY_TAG_VFS) {
//                vdebug("[0x%04x] Left over vfs", ptr);
//            }
//        }
//        darray_destroy(stats.tagged_allocation_pointers[i]);
//    }
}

void *kallocate(u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        vwarn("kallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }
    stats.total_allocated += size;
    stats.tagged_allocations[tag] += size;
    // TODO: Memory alignment
    void *block = platform_allocate(size, false);
    platform_zero_memory(block, size);
//    if (tag != MEMORY_TAG_DARRAY) {
//        darray_push(stats.tagged_allocation_pointers[tag], block)
//    }
    return block;
}

void *kreallocate(void *block, u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        vwarn("kreallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    // Adjust the memory statistics before reallocating
    // Assuming you've stored the old size somewhere, you would subtract that from the stats
    // and add the new size to the stats. If you don't store sizes, consider tracking them
    // somehow as this is a common requirement for custom memory management.

    // stats.total_allocated -= old_size; // Adjust with old size
    stats.total_allocated += size;       // Adjust with new size
    // stats.tagged_allocations[tag] -= old_size; // Adjust with old size
    stats.tagged_allocations[tag] += size;       // Adjust with new size

    // Now use realloc to resize the memory block
    void *new_block = realloc(block, size);
    if (!new_block) {
        // handle error, perhaps logging that the reallocation failed
        return null;
    }

    return new_block;
}

void kfree(void *block, u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        vwarn("kfree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    stats.total_allocated -= size;
    stats.tagged_allocations[tag] -= size;
//    darray_remove(stats.tagged_allocation_pointers[tag], block);
    // TODO: Memory alignment
    platform_free(block, false);
}

void *kzero_memory(void *block, u64 size) {
    return platform_zero_memory(block, size);
}

void *kcopy_memory(void *dest, const void *source, u64 size) {
    return platform_copy_memory(dest, source, size);
}

void *kset_memory(void *dest, i32 value, u64 size) {
    return platform_set_memory(dest, value, size);
}

char *get_memory_usage_str() {
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (stats.tagged_allocations[i] >= gib) {
            unit[0] = 'G';
            amount = stats.tagged_allocations[i] / (float) gib;
        } else if (stats.tagged_allocations[i] >= mib) {
            unit[0] = 'M';
            amount = stats.tagged_allocations[i] / (float) mib;
        } else if (stats.tagged_allocations[i] >= kib) {
            unit[0] = 'K';
            amount = stats.tagged_allocations[i] / (float) kib;
        } else {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float) stats.tagged_allocations[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }
    char *out_string = string_duplicate(buffer);
    return out_string;
}