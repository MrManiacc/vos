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
    u64 total_alloc_size;
} memory_system_configuration;
//
///**
// * @brief Initializes the memory system.
// * @param config The configuration for this system.
// */
//b8 memory_system_initialize(memory_system_configuration config);
//
///**
// * @brief Shuts down the memory system.
// */
//void memory_system_shutdown();
//
///**
// * @brief Performs a memory allocation from the host of the given size. The allocation
// * is tracked for the provided tag.
// * @param size The size of the allocation.
// * @param tag Indicates the use of the allocated block.
// * @returns If successful, a pointer to a block of allocated memory; otherwise 0.
// */
//VAPI void *kallocate(u64 size, memory_tag tag);
//
///**
// * @brief Performs an aligned memory allocation from the host of the given size and alignment.
// * The allocation is tracked for the provided tag. NOTE: Memory allocated this way must be freed
// * using kfree_aligned.
// * @param size The size of the allocation.
// * @param alignment The alignment in bytes.
// * @param tag Indicates the use of the allocated block.
// * @returns If successful, a pointer to a block of allocated memory; otherwise 0.
// */
//VAPI void *kallocate_aligned(u64 size, u16 alignment, memory_tag tag);
//
///**
// * @brief Reports an allocation associated with the application, but made externally.
// * This can be done for items allocated within 3rd party libraries, for example, to
// * track allocations but not perform them.
// *
// * @param size The size of the allocation.
// * @param tag Indicates the use of the allocated block.
// */
//VAPI void kallocate_report(u64 size, memory_tag tag);
//
///**
// * @brief Frees the given block, and untracks its size from the given tag.
// * @param block A pointer to the block of memory to be freed.
// * @param size The size of the block to be freed.
// * @param tag The tag indicating the block's use.
// */
//VAPI void kfree(void *block, u64 size, memory_tag tag);
//
///**
// * @brief Frees the given block, and untracks its size from the given tag.
// * @param block A pointer to the block of memory to be freed.
// * @param size The size of the block to be freed.
// * @param tag The tag indicating the block's use.
// */
//VAPI void kfree_aligned(void *block, u64 size, u16 alignment, memory_tag tag);
//
///**
// * @brief Reports a free associated with the application, but made externally.
// * This can be done for items allocated within 3rd party libraries, for example, to
// * track frees but not perform them.
// *
// * @param size The size in bytes.
// * @param tag The tag indicating the block's use.
// */
//VAPI void kfree_report(u64 size, memory_tag tag);
//
///**
// * @brief Returns the size and alignment of the given block of memory.
// * NOTE: A failure result from this method most likely indicates heap corruption.
// *
// * @param block The memory block.
// * @param out_size A pointer to hold the size of the block.
// * @param out_alignment A pointer to hold the alignment of the block.
// * @return True on success; otherwise false.
// */
//VAPI b8 kmemory_get_size_alignment(void *block, u64 *out_size, u16 *out_alignment);
//
///**
// * @brief Zeroes out the provided memory block.
// * @param block A pointer to the block of memory to be zeroed out.
// * @param size The size in bytes to zero out.
// * @param A pointer to the zeroed out block of memory.
// */
//VAPI void *kzero_memory(void *block, u64 size);
//
///**
// * @brief Performs a copy of the memory at source to dest of the given size.
// * @param dest A pointer to the destination block of memory to copy to.
// * @param source A pointer to the source block of memory to copy from.
// * @param size The amount of memory in bytes to be copied over.
// * @returns A pointer to the block of memory copied to.
// */
//VAPI void *kcopy_memory(void *dest, const void *source, u64 size);
//
///**
// * @brief Sets the bytes of memory located at dest to value over the given size.
// * @param dest A pointer to the destination block of memory to be set.
// * @param value The value to be set.
// * @param size The size in bytes to copy over to.
// * @returns A pointer to the destination block of memory.
// */
//VAPI void *kset_memory(void *dest, i32 value, u64 size);
//
///**
// * @brief Obtains a string containing a "printout" of memory usage, categorized by
// * memory tag. The memory should be freed by the caller.
// * @deprecated This function should be discontinued in favour of something more robust in the future.
// * @returns A pointer to a character array containing the string representation of memory usage.
// */
//VAPI char *get_memory_usage_str(void);
//
///**
// * @brief Obtains the number of times kallocate was called since the memory system was initialized.
// * @returns The total count of allocations since the system's initialization.
// */
//VAPI u64 get_memory_alloc_count(void);


// Modified function signatures with _ prefix, line, and file parameters
b8 _memory_system_initialize(memory_system_configuration config, int line, const char* file);
void _memory_system_shutdown(int line, const char* file);
VAPI void* _kallocate(u64 size, memory_tag tag, int line, const char* file);
VAPI void* _kallocate_aligned(u64 size, u16 alignment, memory_tag tag, int line, const char* file);
VAPI void _kallocate_report(u64 size, memory_tag tag, int line, const char* file);
VAPI void _kfree(void* block, u64 size, memory_tag tag, int line, const char* file);
VAPI void _kfree_aligned(void* block, u64 size, u16 alignment, memory_tag tag, int line, const char* file);
VAPI void _kfree_report(u64 size, memory_tag tag, int line, const char* file);
VAPI b8 _kmemory_get_size_alignment(void* block, u64* out_size, u16* out_alignment, int line, const char* file);
VAPI void* _kzero_memory(void* block, u64 size, int line, const char* file);
VAPI void* _kcopy_memory(void* dest, const void* source, u64 size, int line, const char* file);
VAPI void* _kset_memory(void* dest, i32 value, u64 size, int line, const char* file);
VAPI char* _get_memory_usage_str(int line, const char* file);
VAPI u64 _get_memory_alloc_count(int line, const char* file);

// Macro definitions to automatically pass __LINE__ and __FILE__
#define memory_system_initialize(config) _memory_system_initialize(config, __LINE__, __FILE__)
#define memory_system_shutdown() _memory_system_shutdown(__LINE__, __FILE__)
#define kallocate(size, tag) _kallocate(size, tag, __LINE__, __FILE__)
#define kallocate_aligned(size, alignment, tag) _kallocate_aligned(size, alignment, tag, __LINE__, __FILE__)
#define kallocate_report(size, tag) _kallocate_report(size, tag, __LINE__, __FILE__)
#define kfree(block, size, tag) _kfree(block, size, tag, __LINE__, __FILE__)
#define kfree_aligned(block, size, alignment, tag) _kfree_aligned(block, size, alignment, tag, __LINE__, __FILE__)
#define kfree_report(size, tag) _kfree_report(size, tag, __LINE__, __FILE__)
#define kmemory_get_size_alignment(block, out_size, out_alignment) _kmemory_get_size_alignment(block, out_size, out_alignment, __LINE__, __FILE__)
#define kzero_memory(block, size) _kzero_memory(block, size, __LINE__, __FILE__)
#define kcopy_memory(dest, source, size) _kcopy_memory(dest, source, size, __LINE__, __FILE__)
#define kset_memory(dest, value, size) _kset_memory(dest, value, size, __LINE__, __FILE__)
#define get_memory_usage_str() _get_memory_usage_str(__LINE__, __FILE__)
#define get_memory_alloc_count() _get_memory_alloc_count(__LINE__, __FILE__)
