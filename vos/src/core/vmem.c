#include "vmem.h"

#include "core/vlogger.h"
#include "core/vstring.h"
#include "platform/platform.h"
#include "containers/darray.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/vmutex.h"
#include "core/vasserts.h"
#include "memory/dynamic_allocator.h"
#include "memory/linear_allocator.h"
#include "containers/ptrhash.h"

struct memory_stats {
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
    PtrHashTable *allocations;
};

static const char *memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
        "UNKNOWN    ",
        "ASSET      ",
        "VFS        ",
        "KERNEL     ",
        "PROCESS    ",
        "ARRAY      ",
        "LINEAR_ALLOCATOR",
        "DARRAY     ",
        "DICT       ",
        "RING_QUEUE ",
        "BST        ",
        "STRING     ",
        "ENGINE     ",
        "JOB        ",
        "TEXTURE    ",
        "MATERIAL_INSTANCE",
        "RENDERER   ",
        "GAME       ",
        "TRANSFORM  ",
        "ENTITY     ",
        "ENTITY_NODE",
        "SCENE      ",
        "RESOURCE   ",
        "VULKAN     ",
        "VULKAN_EXT ",
        "DIRECT3D   ",
        "OPENGL     ",
        "GPU_LOCAL  ",
        "BITMAP_FONT",
        "SYSTEM_FONT",
        "KEYMAP     ",
        "HASHTABLE  ",
        "UI         ",
        "AUDIO      "};

typedef struct memory_system_state {
    memory_system_configuration config;
    struct memory_stats stats;
    u64 alloc_count;
    u64 allocator_memory_requirement;
    dynamic_allocator allocator;
    void *allocator_block;
    // A mutex for allocations/frees
    kmutex allocation_mutex;
    
} memory_system_state;

// Pointer to system state.
static memory_system_state *state_ptr;


#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static u64 point_hash(const char *key) {
    u64 hash = FNV_OFFSET;
    for (const char *p = key; *p; p++) {
        hash ^= (u64) (unsigned char) (*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

b8 _memory_system_initialize(memory_system_configuration config, int line, const char *file) {
    vdebug("%s:%d memory_system_initialize called.", file, line);
    
    
    // The amount needed by the system state.
    u64 state_memory_requirement = sizeof(memory_system_state);
    
    // Figure out how much space the dynamic allocator needs.
    u64 alloc_requirement = 0;
    dynamic_allocator_create(config.total_alloc_size, &alloc_requirement, 0, 0);
    
    // Call the platform allocator to get the memory for the whole system, including the state.
    void *block = platform_allocate(state_memory_requirement + alloc_requirement, true);
    if (!block) {
        vfatal("Memory system allocation failed and the system cannot continue.");
        return false;
    }
    
    
    state_ptr = (memory_system_state *) block;
    //create the allocation dict
    state_ptr->config = config;
    state_ptr->alloc_count = 0;
    state_ptr->allocator_memory_requirement = alloc_requirement;
    platform_zero_memory(&state_ptr->stats, sizeof(state_ptr->stats));
    state_ptr->allocator_block = ((char *) block + state_memory_requirement);
    state_ptr->stats.allocations = ptr_hash_table_create(100);
    if (!dynamic_allocator_create(
            config.total_alloc_size,
            &state_ptr->allocator_memory_requirement,
            state_ptr->allocator_block,
            &state_ptr->allocator)) {
        vfatal("Memory system is unable to setup internal allocator. Application cannot continue.");
        return false;
    }
    
    if (!kmutex_create(&state_ptr->allocation_mutex)) {
        vfatal("Unable to create allocation mutex!");
        return false;
    }
    
    vdebug("%s:%d Memory system successfully allocated %llu bytes.", file, line, config.total_alloc_size);
    return true;
}

static void report_memory_leaks();

void _memory_system_shutdown(int line, const char *file) {
    vdebug("%s:%d memory_system_shutdown called.", file, line);
    if (state_ptr) {
        report_memory_leaks();
        kmutex_destroy(&state_ptr->allocation_mutex);
        
        dynamic_allocator_destroy(&state_ptr->allocator);
        //TODO print all memory leaks
        ptr_hash_table_destroy(state_ptr->stats.allocations);
        platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(memory_system_state));
    }
    state_ptr = 0;
}


typedef struct AllocationMetadata {
    const char *file;
    int line;
    u64 size;
    memory_tag tag;
} AllocationMetadata;



// Modify dict to support void* keys
// This requires adjustments in dict's implementation to directly handle void* keys without converting them to strings.

void kallocate_record(void *ptr, u64 size, const char *file, int line, memory_tag tag) {
    if (!ptr) return;
//    vdebug("%s:%d kallocate_aligned successfully allocated %llu bytes.", file, line, size);
    AllocationMetadata *metadata = (AllocationMetadata *) platform_allocate(sizeof(AllocationMetadata), false);
    metadata->file = file;
    metadata->line = line;
    metadata->size = size;
    metadata->tag = tag;
    
    // Lock the mutex before modifying the dictionary
    kmutex_lock(&state_ptr->allocation_mutex);
    ptr_hash_table_set(state_ptr->stats.allocations, ptr, metadata);
    kmutex_unlock(&state_ptr->allocation_mutex);
}

void kfree_record(void *ptr) {
    if (!ptr) return;
    
    // Lock the mutex before modifying the dictionary
    kmutex_lock(&state_ptr->allocation_mutex);
    // Remove the allocation from the dictionary
    AllocationMetadata *metadata = (AllocationMetadata *) ptr_hash_table_get(state_ptr->stats.allocations, ptr);
    if (metadata) {
        ptr_hash_table_remove(state_ptr->stats.allocations, ptr);
//        vdebug("Freed memory: %llu bytes allocated at %s:%d", metadata->size, metadata->file, metadata->line)
    }
    kmutex_unlock(&state_ptr->allocation_mutex);
    if (metadata) {
        // Optionally log metadata for debug purposes
        platform_free(metadata, false);
    }
}

// Modify _kallocate_aligned and _kfree_aligned to call _kallocate_record and _kfree_record respectively.



void *_kallocate(u64 size, memory_tag tag, int line, const char *file) {
    return _kallocate_aligned(size, 1, tag, line, file);
}

void *_kallocate_aligned(u64 size, u16 alignment, memory_tag tag, int line, const char *file) {
//    vdebug("%s:%d kallocate_aligned called with size: %llu, alignment: %u, tag: %d", file, line, size, alignment, tag);
    if (tag == MEMORY_TAG_UNKNOWN) {
        vwarn("%s:%d kallocate_aligned called using MEMORY_TAG_UNKNOWN. Re-class this allocation.", file, line);
    }
    
    void *block = 0;
    if (state_ptr) {
        if (!kmutex_lock(&state_ptr->allocation_mutex)) {
            vfatal("%s:%d Error obtaining mutex lock during allocation.", file, line);
            return 0;
        }
        
        state_ptr->stats.total_allocated += size;
        state_ptr->stats.tagged_allocations[tag] += size;
        state_ptr->alloc_count++;
        block = dynamic_allocator_allocate_aligned(&state_ptr->allocator, size, alignment);
        kmutex_unlock(&state_ptr->allocation_mutex);
    } else {
        block = platform_allocate(size, false);
    }
    
    if (block) {
        platform_zero_memory(block, size);
        // Add the allocation to the dict.
        kallocate_record(block, size, file, line, tag);
        return block;
    }
    
    vfatal("%s:%d kallocate_aligned failed to allocate successfully.", file, line);
    return 0;
}

void _kallocate_report(u64 size, memory_tag tag, int line, const char *file) {
//    vdebug("%s:%d kallocate_report called with size: %llu, tag: %d", file, line, size, tag);
    if (!kmutex_lock(&state_ptr->allocation_mutex)) {
        vfatal("%s:%d Error obtaining mutex lock during allocation reporting.", file, line);
        return;
    }
    state_ptr->stats.total_allocated += size;
    state_ptr->stats.tagged_allocations[tag] += size;
    state_ptr->alloc_count++;
    kmutex_unlock(&state_ptr->allocation_mutex);
}

void _kfree(void *block, u64 size, memory_tag tag, int line, const char *file) {
    _kfree_aligned(block, size, 1, tag, line, file);
}

void _kfree_aligned(void *block, u64 size, u16 alignment, memory_tag tag, int line, const char *file) {
//    vdebug("%s:%d kfree_aligned called with size: %llu, alignment: %u, tag: %d", file, line, size, alignment, tag);
    if (tag == MEMORY_TAG_UNKNOWN) {
        vwarn("%s:%d kfree_aligned called using MEMORY_TAG_UNKNOWN. Re-class this allocation.", file, line);
    }
    //If we already have freed the block, we should not free it again.
    if (_kis_free(block, line, file)) {
//        vdebug("%s:%d Attempted to free a block that has already been freed. This is likely a double free.", file, line);
        kfree_record(block);
        block = null;
        return;
    }
    
    if (state_ptr) {
        
        if (!kmutex_lock(&state_ptr->allocation_mutex)) {
            vfatal("%s:%d Unable to obtain mutex lock for free operation. Heap corruption is likely.", file, line);
            return;
        }
        //Check if the block has already been freed
        
        
        state_ptr->stats.total_allocated -= size;
        state_ptr->stats.tagged_allocations[tag] -= size;
        state_ptr->alloc_count--;
        b8 result = dynamic_allocator_free_aligned(&state_ptr->allocator, block);
        
        kmutex_unlock(&state_ptr->allocation_mutex);
        
        if (!result) {
            platform_free(block, false);
        }
    } else {
        platform_free(block, false);
    }
    kfree_record(block);
    block = null;// Set the block to null to prevent double frees.
}

void _kfree_report(u64 size, memory_tag tag, int line, const char *file) {
//    vdebug("%s:%d kfree_report called with size: %llu, tag: %d", file, line, size, tag);
    if (!kmutex_lock(&state_ptr->allocation_mutex)) {
        vfatal("%s:%d Error obtaining mutex lock during allocation reporting.", file, line);
        return;
    }
    state_ptr->stats.total_allocated -= size;
    state_ptr->stats.tagged_allocations[tag] -= size;
    state_ptr->alloc_count--;
    kmutex_unlock(&state_ptr->allocation_mutex);
}

b8 _kmemory_get_size_alignment(void *block, u64 *out_size, u16 *out_alignment, int line, const char *file) {
//    vdebug("%s:%d kmemory_get_size_alignment called.", file, line);
    if (!kmutex_lock(&state_ptr->allocation_mutex)) {
        vfatal("%s:%d Error obtaining mutex lock during kmemory_get_size_alignment.", file, line);
        return false;
    }
    b8 result = dynamic_allocator_get_size_alignment(block, out_size, out_alignment);
    kmutex_unlock(&state_ptr->allocation_mutex);
    return result;
}

void *_kzero_memory(void *block, u64 size, int line, const char *file) {
//    vdebug("%s:%d kzero_memory called with size: %llu", file, line, size);
    return platform_zero_memory(block, size);
}

void *_kcopy_memory(void *dest, const void *source, u64 size, int line, const char *file) {
//    vdebug("%s:%d kcopy_memory called with size: %llu", file, line, size);
    return platform_copy_memory(dest, source, size);
}

void *_kset_memory(void *dest, i32 value, u64 size, int line, const char *file) {
//    vdebug("%s:%d kset_memory called with value: %d, size: %llu", file, line, value, size);
    return platform_set_memory(dest, value, size);
}


u64 _get_memory_alloc_count(int line, const char *file) {
//    vdebug("%s:%d get_memory_alloc_count called.", file, line);
    if (state_ptr) {
        return state_ptr->alloc_count;
    }
    return 0;
}

// Ensure to replace the function calls in the original implementation with their macro versions
// as defined in the header to automatically pass __LINE__ and __FILE__ parameters.

const char *get_unit_for_size(u64 size_bytes, f32 *out_amount) {
    if (size_bytes >= GIBIBYTES(1)) {
        *out_amount = (f64) size_bytes / GIBIBYTES(1);
        return "GiB";
    } else if (size_bytes >= MEBIBYTES(1)) {
        *out_amount = (f64) size_bytes / MEBIBYTES(1);
        return "MiB";
    } else if (size_bytes >= KIBIBYTES(1)) {
        *out_amount = (f64) size_bytes / KIBIBYTES(1);
        return "KiB";
    } else {
        *out_amount = (f32) size_bytes;
        return "B";
    }
}

char *_get_memory_usage_str(int line, const char *file) {
    vdebug("%s:%d get_memory_usage_str called.", file, line);
    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        f32 amount = 1.0f;
        const char *unit = get_unit_for_size(state_ptr->stats.tagged_allocations[i], &amount);
        
        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }
    {
        // Compute total usage.
        u64 total_space = dynamic_allocator_total_space(&state_ptr->allocator);
        u64 free_space = dynamic_allocator_free_space(&state_ptr->allocator);
        u64 used_space = total_space - free_space;
        
        f32 used_amount = 1.0f;
        const char *used_unit = get_unit_for_size(used_space, &used_amount);
        
        f32 total_amount = 1.0f;
        const char *total_unit = get_unit_for_size(total_space, &total_amount);
        
        f64 percent_used = (f64) (used_space) / total_space;
        
        i32 length = snprintf(buffer + offset, 8000, "Total memory usage: %.2f%s of %.2f%s (%.2f%%)\n", used_amount,
                              used_unit, total_amount, total_unit, percent_used);
        offset += length;
    }
    
    char *out_string = strdup(buffer);
    return out_string;
}

b8 _kis_free(void *block, int line, const char *file) {
    if (state_ptr) {
        if (!kmutex_lock(&state_ptr->allocation_mutex)) {
            vfatal("%s:%d Error obtaining mutex lock during kis_free.", file, line);
            return false;
        }
        // Check if the block is free.
        b8 result = ptr_hash_table_contains(state_ptr->stats.allocations, block);
        
        kmutex_unlock(&state_ptr->allocation_mutex);
        return !result;
    }
    return false;
}


typedef struct AllocationReport {
    const char *file;
    int line;
    u64 totalSize;
    void *sizes; // Dynamic array to store individual allocation sizes
} AllocationReport;

// Helper function to format sizes in human-readable form
void format_size(u64 sizeBytes, char *outStr, size_t outStrSize) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = sizeBytes;
    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        ++unitIndex;
    }
    snprintf(outStr, outStrSize, "%.2f %s", size, units[unitIndex]);
}

static void report_memory_leaks() {
    if (!state_ptr) {
        return;
    }
    
    kmutex_lock(&state_ptr->allocation_mutex);
    
    printf("Memory Leak Report:\n");
    printf("%-50s %-10s %-15s\n", "Location", "Tag", "Size");
    
    PtrHashTableIterator it = ptr_hash_table_iterator_create(state_ptr->stats.allocations);
    void *key = NULL;
    AllocationMetadata *metadata = NULL;
    while (ptr_hash_table_iterator_has_next(&it)) {
        ptr_hash_table_iterator_next(&it, &key, (void **) &metadata);
        if (metadata) {
            char formattedSize[32];
            format_size(metadata->size, formattedSize, sizeof(formattedSize));
            char location[1024]; // Ensure this is large enough for your paths
            snprintf(location, sizeof(location), "%s:%d", metadata->file, metadata->line);
//
            printf("%-50s %-10s %-15s\n",
                   location,
                   memory_tag_strings[metadata->tag],
                   formattedSize);
            
            // Free the metadata if needed
            platform_free(metadata, sizeof(AllocationMetadata));
        }
    }
    
    ptr_hash_table_destroy(state_ptr->stats.allocations);
    kmutex_unlock(&state_ptr->allocation_mutex);
}

//static void report_memory_leaks() {
//    if (!state_ptr) {
//        return;
//    }
//    u64 totalLeakedSize = 0;
//    int totalLeaksCount = 0;
//    char formattedSize[32];
//    kmutex_lock(&state_ptr->allocation_mutex);
//
//    // Temporary storage for aggregation
//    PtrHashTable *reportTable = ptr_hash_table_create(state_ptr->stats.allocations->capacity);
//
//    PtrHashTable *allocations = state_ptr->stats.allocations;
//    for (u32 i = 0; i < allocations->capacity; ++i) {
//        PtrHashTableEntry *entry = allocations->buckets[i];
//        while (entry) {
//            AllocationMetadata *metadata = (AllocationMetadata *) entry->value;
//            u64 key = point_hash(metadata->file) ^ (u64) metadata->line; // Unique key for file-line combination
//
//            AllocationReport *report = (AllocationReport *) ptr_hash_table_get(reportTable, (void *) key);
//            if (!report) {
//                report = (AllocationReport *) platform_allocate(sizeof(AllocationReport), false);
//                report->file = metadata->file;
//                report->line = metadata->line;
//                report->totalSize = 0;
//                report->sizes = darray_create(sizeof(u64));
//                ptr_hash_table_set(reportTable, (void *) key, report);
//            }
//
//            darray_push(report->sizes, metadata->size);
//            report->totalSize += metadata->size;
//
//            entry = entry->next;
//        }
//    }
//
//
//
//    // Print table header
//    printf("%-50s %-15s %s\n", "Location", "Total Size", "Allocations");
//
//    // Iterate through reports and print each row
//    for (u32 i = 0; i < reportTable->capacity; ++i) {
//        PtrHashTableEntry *entry = reportTable->buckets[i];
//        while (entry) {
//            AllocationReport *report = (AllocationReport *) entry->value;
//            format_size(report->totalSize, formattedSize, sizeof(formattedSize));
//
//            // Combine file name and line number into a single string
//            char location[1024]; // Ensure this is large enough for your paths
//            snprintf(location, sizeof(location), "%s:%d", report->file, report->line);
//
//            // Print the location and total size with proper alignment
//            printf("%-50s %-14s", location, formattedSize);
//
//            // Rest of the logic for printing allocations remains unchanged
//
//            u64 remainderSize = 0;
//            printf("[");
//            for (int j = 0; j < darray_length(report->sizes); ++j) {
//                if (j < 4) { // Print the first four allocations
//                    u64 *size = (u64 *) darray_get(report->sizes, j);
//                    char sizeStr[32];
//                    format_size(*size, sizeStr, sizeof(sizeStr));
//                    printf("%s%s", j > 0 ? ", " : "", sizeStr);
//                } else { // Sum up the remaining allocations
//                    remainderSize += *(u64 *) darray_get(report->sizes, j);
//                }
//            }
//
//            // If there are more than four allocations, print the sum of the remainder
//            if (darray_length(report->sizes) > 4) {
//                char remainderSizeStr[32];
//                format_size(remainderSize, remainderSizeStr, sizeof(remainderSizeStr));
//                printf(", + %s more", remainderSizeStr);
//            }
//
//            printf("]\n");
//
//            // Cleanup and stats update remains unchanged
//            entry = entry->next;
//        }
//    }
//
//    format_size(totalLeakedSize, formattedSize, sizeof(formattedSize));
//    printf("\nSummary: %d leaks detected, totaling %s.\n", totalLeaksCount, formattedSize);
//    ptr_hash_table_destroy(reportTable);
//    kmutex_unlock(&state_ptr->allocation_mutex);
//}
