/**
 * Created by jraynor on 2/19/2024.
 */
#pragma once

#include "lib/defines.h"
#include "lib/memory.h"

//============================================================
// Arena - a simple memory allocator
//============================================================
typedef struct Region Region;

struct Region {
    Region *next;
    u64 count;
    u64 capacity;
    u32 data[];
};

typedef struct {
    Region *begin, *end;
} Arena;

#define REGION_DEFAULT_CAPACITY (8*1024)

Region *new_region(u64 capacity);
void free_region(Region *r);

// TODO: snapshot/rewind capability for the arena
// - Snapshot should be combination of a->end and a->end->count.
// - Rewinding should be restoring a->end and a->end->count from the snapshot and
// setting count-s of all the Region-s after the remembered a->end to 0.
void *arena_alloc(Arena *a, u64 size_bytes);
void *arena_realloc(Arena *a, void *oldptr, u64 oldsz, u64 newsz);

void arena_reset(Arena *a);
void arena_free(Arena *a);
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#define INV_HANDLE(x)       (((x) == NULL) || ((x) == INVALID_HANDLE_VALUE))

Region *new_region(size_t capacity) {
    SIZE_T size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
    Region *r = VirtualAllocEx(
            GetCurrentProcess(),      /* Allocate in current process address space */
            NULL,                     /* Unknown position */
            size_bytes,               /* Bytes to allocate */
            MEM_COMMIT | MEM_RESERVE, /* Reserve and commit allocated page */
            PAGE_READWRITE            /* Permissions ( Read/Write )*/
    );
    if (INV_HANDLE(r)) VASSERT(0 && "VirtualAllocEx() failed.");
    
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void free_region(Region *r) {
    if (INV_HANDLE(r))
        return;
    
    BOOL free_result = VirtualFreeEx(
            GetCurrentProcess(),        /* Deallocate from current process address space */
            (LPVOID) r,                  /* Address to deallocate */
            0,                          /* Bytes to deallocate ( Unknown, deallocate entire page ) */
            MEM_RELEASE                 /* Release the page ( And implicitly decommit it ) */
    );
    
    if (FALSE == free_result)
        VASSERT(0 && "VirtualFreeEx() failed.");
}


// TODO: add debug statistic collection mode for arena
// Should collect things like:
// - How many times new_region was called
// - How many times existing region was skipped
// - How many times allocation exceeded REGION_DEFAULT_CAPACITY

void *arena_alloc(Arena *a, size_t size_bytes) {
    size_t size = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
    
    if (a->end == NULL) {
        VASSERT(a->begin == NULL);
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->end = new_region(capacity);
        a->begin = a->end;
    }
    
    while (a->end->count + size > a->end->capacity && a->end->next != NULL) {
        a->end = a->end->next;
    }
    
    if (a->end->count + size > a->end->capacity) {
        VASSERT(a->end->next == NULL);
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->end->next = new_region(capacity);
        a->end = a->end->next;
    }
    
    void *result = &a->end->data[a->end->count];
    a->end->count += size;
    return result;
}

void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz) {
    if (newsz <= oldsz) return oldptr;
    void *newptr = arena_alloc(a, newsz);
    char *newptr_char = newptr;
    char *oldptr_char = oldptr;
    for (size_t i = 0; i < oldsz; ++i) {
        newptr_char[i] = oldptr_char[i];
    }
    return newptr;
}

void arena_reset(Arena *a) {
    for (Region *r = a->begin; r != NULL; r = r->next) {
        r->count = 0;
    }
    
    a->end = a->begin;
}

void arena_free(Arena *a) {
    Region *r = a->begin;
    while (r) {
        Region *r0 = r;
        r = r->next;
        free_region(r0);
    }
    a->begin = NULL;
    a->end = NULL;
}
