#pragma once

#include "defines.h"

/**
 * @brief A simple stack container. Elements may be pushed on or popped
 * off of the stack only.
 */
typedef struct Stack {
    /** @brief The element size in bytes.*/
    u32 element_size;
    /** @brief The current element count. */
    u32 element_count;
    /** @brief The total amount of currently-allocated memory.*/
    u32 allocated;
    /** @brief The allocated memory block. */
    void *memory;
} Stack;

/**
 * @brief Creates a new stack.
 *
 * @param out_stack A pointer to hold the newly-created stack.
 * @param element_size The size of each element in the stack.
 * @return True on success; otherwise false.
 */
VAPI b8 stack_create(Stack *out_stack, u32 element_size);
/**
 * @brief Destroys the given stack.
 *
 * @param s A pointer to the stack to be destroyed.
 */
VAPI void stack_destroy(Stack *s);

/**
 * @brief Pushes an element (a copy of the element data) onto the stack.
 *
 * @param s A pointer to the stack to push to.
 * @param element_data The element data to be pushed. Required.
 * @return True on succcess; otherwise false.
 */
VAPI b8 stack_push(Stack *s, void *element_data);

/**
 * @brief Attempts to peek an element (writing out a copy of the
 * element data on success) from the stack. If the stack is empty,
 * nothing is done and false is returned. The stack memory is not modified.
 *
 * @param s A pointer to the stack to peek from.
 * @param element_data A pointer to write the element data to. Required.
 * @return True on succcess; otherwise false.
 */
VAPI b8 stack_peek(const Stack *s, void *out_element_data);

/**
 * @brief Attempts to pop an element (writing out a copy of the
 * element data on success) from the stack. If the stack is empty,
 * nothing is done and false is returned.
 *
 * @param s A pointer to the stack to pop from.
 * @param element_data A pointer to write the element data to. Required.
 * @return True on succcess; otherwise false.
 */
VAPI b8 stack_pop(Stack *s, void *out_element_data);
