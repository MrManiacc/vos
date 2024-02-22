/**
 * Created by jraynor on 2/19/2024.
 */
#pragma once


#include "defines.h"

/**
 * @brief Performs platform-specific memory allocation of the given size.
 *
 * @param size The size of the allocation in bytes.
 * @param aligned Indicates if the allocation should be aligned.
 * @return A pointer to a block of allocated memory.
 */
void *platform_allocate(u64 size, b8 aligned);

/**
 * @brief Performs platform-specific memory reallocation of the given block of memory.
 *
 * @param block The block of memory to be reallocated.
 * @param size The new size of the allocation in bytes.
 * @param aligned Indicates if the allocation should be aligned.
 * @return A pointer to the reallocated block of memory.
 */
void *platform_reallocate(void *block, u64 size, b8 aligned);

/**
 * @brief Frees the given block of memory.
 *
 * @param block The block to be freed.
 * @param aligned Indicates if the block of memory is aligned.
 */
void platform_free(void *block, b8 aligned);

/**
 * @brief Performs platform-specific zeroing out of the given block of memory.
 *
 * @param block The block to be zeroed out.
 * @param size The size of data to zero out.
 * @return A pointer to the zeroed out block of memory.
 */
void *platform_zero_memory(void *block, u64 size);

/**
 * @brief Copies the bytes of memory in source to dest, of the given size.
 *
 * @param dest The destination memory block.
 * @param source The source memory block.
 * @param size The size of data to be copied.
 * @return A pointer to the destination block of memory.
 */
void *platform_copy_memory(void *dest, const void *source, u64 size);

/**
 * @brief Sets the bytes of memory to the given value.
 *
 * @param dest The destination block of memory.
 * @param value The value to be set.
 * @param size The size of data to set.
 * @return A pointer to the set block of memory.
 */
void *platform_set_memory(void *dest, i32 value, u64 size);

/**
 * @brief Performs platform-specific printing to the console of the given
 * message and colour code (if supported).
 *
 * @param message The message to be printed.
 * @param colour The colour to print the text in (if supported).
 */
void platform_console_write(const char *message, u8 colour);

/**
 * @brief Performs platform-specific printing to the error console of the given
 * message and colour code (if supported).
 *
 * @param message The message to be printed.
 * @param colour The colour to print the text in (if supported).
 */
void platform_console_write_error(const char *message, u8 colour);