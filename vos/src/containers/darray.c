#include <string.h>
#include "containers/darray.h"

#include "core/vmem.h"
#include "core/vlogger.h"

void *_darray_create(u64 length, u64 stride) {
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 array_size = length * stride;
    u64 *new_array = kallocate(header_size + array_size, MEMORY_TAG_DARRAY);
    kset_memory(new_array, 0, header_size + array_size);
    new_array[DARRAY_CAPACITY] = length;
    new_array[DARRAY_LENGTH] = 0;
    new_array[DARRAY_STRIDE] = stride;
//    vdebug("Darray created: %p, Capacity: %llu, Stride: %llu", (void *)(new_array + DARRAY_FIELD_LENGTH), length, stride);
    return (void *) (new_array + DARRAY_FIELD_LENGTH);
}

void _darray_destroy(void *array) {
    // Retrieve the darray header
    u64 *header = (u64 *) array - DARRAY_FIELD_LENGTH;
    u64 length = header[DARRAY_LENGTH];
//    vdebug("Destroying darray: %p, Length: %llu, Capacity: %llu", array, header[DARRAY_LENGTH], header[DARRAY_CAPACITY]);
    u64 stride = header[DARRAY_STRIDE];
    u64 total_size = (DARRAY_FIELD_LENGTH * sizeof(u64)) + (header[DARRAY_CAPACITY] * stride);
    kfree(header, total_size, MEMORY_TAG_DARRAY);
    
}

u64 _darray_field_get(void *array, u64 field) {
    u64 *header = (u64 *) array - DARRAY_FIELD_LENGTH;
    return header[field];
}

void _darray_field_set(void *array, u64 field, u64 value) {
    u64 *header = (u64 *) array - DARRAY_FIELD_LENGTH;
    header[field] = value;
}

void *_darray_resize(void *array) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    void *temp = _darray_create(
            (DARRAY_RESIZE_FACTOR * darray_capacity(array)),
            stride);
    kcopy_memory(temp, array, length * stride);
    
    _darray_field_set(temp, DARRAY_LENGTH, length);
    _darray_destroy(array);
    return temp;
}

void *_darray_push(void *array, const void *value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (length >= darray_capacity(array)) {
        array = _darray_resize(array);
    }
    
    u64 addr = (u64) array;
    addr += (length * stride);
    kcopy_memory((void *) addr, value_ptr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length + 1);
    return array;
}

void _darray_pop(void *array, void *dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u64 addr = (u64) array;
    addr += ((length - 1) * stride);
    kcopy_memory(dest, (void *) addr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length - 1);
}

void *_darray_get(void *array, u64 index) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        verror("Index outside the bounds of this array! Length: %i, index: %index", length, index);
        return 0;
    }
    u64 addr = (u64) array;
    addr += (index * stride);
    return (void *) addr;
}

void *_darray_pop_at(void *array, u64 index, void *dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        verror("Index outside the bounds of this array! Length: %i, index: %index", length, index);
        return array;
    }
    
    u64 addr = (u64) array;
    kcopy_memory(dest, (void *) (addr + (index * stride)), stride);
    
    // If not on the last element, snip out the entry and copy the rest inward.
    if (index != length - 1) {
        kcopy_memory(
                (void *) (addr + (index * stride)),
                (void *) (addr + ((index + 1) * stride)),
                stride * (length - index));
    }
    
    _darray_field_set(array, DARRAY_LENGTH, length - 1);
    return array;
}

void *_darray_insert_at(void *array, u64 index, void *value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        verror("Index outside the bounds of this array! Length: %i, index: %index", length, index);
        return array;
    }
    if (length >= darray_capacity(array)) {
        array = _darray_resize(array);
    }
    
    u64 addr = (u64) array;
    
    // If not on the last element, copy the rest outward.
    if (index != length - 1) {
        kcopy_memory(
                (void *) (addr + ((index + 1) * stride)),
                (void *) (addr + (index * stride)),
                stride * (length - index));
    }
    
    // Set the value at the index
    kcopy_memory((void *) (addr + (index * stride)), value_ptr, stride);
    
    _darray_field_set(array, DARRAY_LENGTH, length + 1);
    return array;
}

u64 _darray_find(void *array, void *value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u8 *addr = (u8 *) array;  // use byte pointer for address arithmetic
    for (u64 i = 0; i < length; ++i) {
        if (memcmp(addr, value_ptr, stride) == 0) {
            return i;
        }
        addr += stride;
    }
    return -1;  // returning -1 to signify not found, consider using ULLONG_MAX from limits.h
}

u64 _darray_remove(void *array, void *value_ptr) {
    u64 index = _darray_find(array, value_ptr);
    if (index == -1) {
        return -1;
    }
    _darray_pop_at(array, index, value_ptr);
    return index;
}