#include "queue.h"

#include "lib/memory.h"
#include "lib/logger.h"

static void queue_ensure_allocated(queue *s, u32 count) {
    if (s->allocated < s->element_size * count) {
        void *temp = vallocate(count * s->element_size, MEMORY_TAG_ARRAY);
        if (s->memory) {
            vcopy_memory(temp, s->memory, s->allocated);
            vfree(s->memory, s->allocated, MEMORY_TAG_ARRAY);
        }
        s->memory = temp;
        s->allocated = count * s->element_size;
    }
}

b8 queue_create(queue *out_queue, u32 element_size) {
    if (!out_queue) {
        verror("queue_create requires a pointer to a valid queue.");
        return false;
    }
    
    vzero_memory(out_queue, sizeof(queue));
    out_queue->element_size = element_size;
    out_queue->element_count = 0;
    queue_ensure_allocated(out_queue, 1);
    return true;
}

void queue_destroy(queue *s) {
    if (s) {
        if (s->memory) {
            vfree(s->memory, s->allocated, MEMORY_TAG_ARRAY);
        }
        vzero_memory(s, sizeof(queue));
    }
}

b8 queue_push(queue *s, void *element_data) {
    if (!s) {
        verror("queue_push requires a pointer to a valid queue.");
        return false;
    }
    
    queue_ensure_allocated(s, s->element_count + 1);
    vcopy_memory((void *) ((u64) s->memory + (s->element_count * s->element_size)), element_data, s->element_size);
    s->element_count++;
    return true;
}

b8 queue_peek(const queue *s, void *out_element_data) {
    if (!s || !out_element_data) {
        verror("queue_peek requires a pointer to a valid queue and to hold element data output.");
        return false;
    }
    
    if (s->element_count < 1) {
        vwarn("Cannot peek from an empty queue.");
        return false;
    }
    
    // Copy the front entry to out_element_data
    vcopy_memory(out_element_data, s->memory, s->element_size);
    
    return true;
}

b8 queue_pop(queue *s, void *out_element_data) {
    if (!s || !out_element_data) {
        verror("queue_pop requires a pointer to a valid queue and to hold element data output.");
        return false;
    }
    
    if (s->element_count < 1) {
        vwarn("Cannot pop from an empty queue.");
        return false;
    }
    
    // Copy the front entry to out_element_data
    vcopy_memory(out_element_data, s->memory, s->element_size);
    
    // Move everything "forward".
    vcopy_memory(s->memory, (void *) (((u64) s->memory) + s->element_size), s->element_size * (s->element_count - 1));
    
    s->element_count--;
    
    return true;
}
