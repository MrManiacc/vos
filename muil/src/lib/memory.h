/**
 * Created by jraynor on 2/19/2024.
 */
#pragma once

#include "defines.h"

/** @brief Tags to indicate the usage of memory allocations made in this system. */
typedef enum memory_tag {
    // For temporary use. Should be assigned one of the below or have a new tag created.
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ASSET,
    MEMORY_TAG_VFS,
    MEMORY_TAG_KERNEL,
    MEMORY_TAG_PROCESS,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_ENGINE,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,
    MEMORY_TAG_RESOURCE,
    MEMORY_TAG_VULKAN,
    // "External" vulkan allocations, for reporting purposes only.
    MEMORY_TAG_VULKAN_EXT,
    MEMORY_TAG_DIRECT3D,
    MEMORY_TAG_OPENGL,
    // Representation of GPU-local/vram
    MEMORY_TAG_GPU_LOCAL,
    MEMORY_TAG_BITMAP_FONT,
    MEMORY_TAG_SYSTEM_FONT,
    MEMORY_TAG_KEYMAP,
    MEMORY_TAG_HASHTABLE,
    MEMORY_TAG_UI,
    MEMORY_TAG_AUDIO,
    
    MEMORY_TAG_MAX_TAGS
} memory_tag;

/** @brief The configuration for the memory system. */
typedef struct memory_system_configuration {
    /** @brief The total memory size in byes used by the internal allocator for this system. */
    u64 heap_size;
} memory_system_configuration;

// Modified function signatures with _ prefix, line, and file parameters
b8 _memory_system_initialize(memory_system_configuration config, int line, const char *file);

void _memory_system_shutdown(int line, const char *file);

VAPI void *_kallocate(u64 size, memory_tag tag, int line, const char *file);

VAPI void *_kallocate_aligned(u64 size, u16 alignment, memory_tag tag, int line, const char *file);

VAPI void _kallocate_report(u64 size, memory_tag tag, int line, const char *file);

VAPI void _kfree(void *block, u64 size, memory_tag tag, int line, const char *file);

VAPI void _kfree_aligned(void *block, u64 size, u16 alignment, memory_tag tag, int line, const char *file);

VAPI void _kfree_report(u64 size, memory_tag tag, int line, const char *file);

VAPI b8 _kmemory_get_size_alignment(void *block, u64 *out_size, u16 *out_alignment, int line, const char *file);

VAPI void *_kzero_memory(void *block, u64 size, int line, const char *file);

VAPI void *_kcopy_memory(void *dest, const void *source, u64 size, int line, const char *file);

VAPI void *_kset_memory(void *dest, i32 value, u64 size, int line, const char *file);

VAPI char *_get_memory_usage_str(int line, const char *file);

VAPI u64 _get_memory_alloc_count(int line, const char *file);

VAPI b8 _kis_free(void *block, int line, const char *file);

// Macro definitions to automatically pass __LINE__ and __FILE__
#define memory_system_initialize(config) _memory_system_initialize(config, __LINE__, __FILE__)
#define memory_system_shutdown() _memory_system_shutdown(__LINE__, __FILE__)
#define vallocate(size, tag) _kallocate(size, tag, __LINE__, __FILE__)
#define vgimme(size) _kallocate(size, MEMORY_TAG_ENGINE, __LINE__, __FILE__)
#define vbye(block, size) _kfree(block, size, MEMORY_TAG_ENGINE, __LINE__, __FILE__)
#define vallocate_aligned(size, alignment, tag) _kallocate_aligned(size, alignment, tag, __LINE__, __FILE__)
#define vallocate_report(size, tag) _kallocate_report(size, tag, __LINE__, __FILE__)
#define vfree(block, size, tag) _kfree(block, size, tag, __LINE__, __FILE__)
#define vfree_aligned(block, size, alignment, tag) _kfree_aligned(block, size, alignment, tag, __LINE__, __FILE__)
#define vfree_report(size, tag) _kfree_report(size, tag, __LINE__, __FILE__)
#define vmemory_get_size_alignment(block, out_size, out_alignment) _kmemory_get_size_alignment(block, out_size, out_alignment, __LINE__, __FILE__)
#define vzero_memory(block, size) _kzero_memory(block, size, __LINE__, __FILE__)
#define vcopy_memory(dest, source, size) _kcopy_memory(dest, source, size, __LINE__, __FILE__)
#define vset_memory(dest, value, size) _kset_memory(dest, value, size, __LINE__, __FILE__)
#define vmem_usage_str() _get_memory_usage_str(__LINE__, __FILE__)
#define vmem_alloc_count() _get_memory_alloc_count(__LINE__, __FILE__)
#define vmem_is_free(block) _kis_free(block, __LINE__, __FILE__)
#define vnew(type) (type *) vallocate(sizeof(type), MEMORY_TAG_ENGINE)
#define vdelete(block) vfree(block, sizeof(*block), MEMORY_TAG_ENGINE)

