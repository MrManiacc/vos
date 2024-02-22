/**
 * Created by jraynor on 2/12/2024.
 */
#include "ptrhash.h"

#include <stdlib.h>
#include <stdint.h>

#include "platform/platform.h"
#include "lib/vmem.h"


// Simple hash function for pointers
static size_t ptr_hash(void *ptr, u32 capacity) {
    u32 val = (uintptr_t) ptr;
    return val % capacity;
}

// Create a new hash table
PtrHashTable *ptr_hash_table_create(u32 capacity) {
    PtrHashTable *table = (PtrHashTable *) malloc(sizeof(PtrHashTable));
    table->buckets = (PtrHashTableEntry **) calloc(capacity, sizeof(PtrHashTableEntry *));
    table->capacity = capacity;
    return table;
}

// Insert or update a value in the hash table
void ptr_hash_table_set(PtrHashTable *table, void *key, void *value) {
    size_t index = ptr_hash(key, table->capacity);
    PtrHashTableEntry *entry = table->buckets[index];
    
    // Search for an existing key
    while (entry) {
        if (entry->key == key) {
            entry->value = value; // Update existing entry
            return;
        }
        entry = entry->next;
    }
    
    // Create a new entry if key not found
    PtrHashTableEntry *newEntry = (PtrHashTableEntry *) malloc(sizeof(PtrHashTableEntry));
    newEntry->key = key;
    newEntry->value = value;
    newEntry->next = table->buckets[index];
    table->buckets[index] = newEntry;
}

// Retrieve a value from the hash table
void *ptr_hash_table_get(PtrHashTable *table, void *key) {
    size_t index = ptr_hash(key, table->capacity);
    PtrHashTableEntry *entry = table->buckets[index];
    if (!entry)
        return null;
    while (entry) {
        if (entry->key == key) {
            return entry->value; // Key found
        }
        entry = entry->next;
    }
    
    return null; // Key not found
}

// Remove an entry from the hash table
void ptr_hash_table_remove(PtrHashTable *table, void *key) {
    size_t index = ptr_hash(key, table->capacity);
    PtrHashTableEntry **entry = &table->buckets[index];
    
    while (*entry) {
        if ((*entry)->key == key) {
            PtrHashTableEntry *temp = *entry;
            *entry = (*entry)->next;
            free(temp); // Free the removed entry
            return;
        }
        entry = &(*entry)->next;
    }
}

// Free the hash table
void ptr_hash_table_destroy(PtrHashTable *table) {
    //I suppose we'll assume the user will free the values themselves
    //    for (size_t i = 0; i < table->capacity; ++i) {
//        PtrHashTableEntry *entry = table->buckets[i];
//        while (entry) {
//            PtrHashTableEntry *temp = entry;
//            entry = entry->next;
//            free(temp);
//        }
//    }
    kfree(table->buckets, table->capacity * sizeof(PtrHashTableEntry *), MEMORY_TAG_HASHTABLE);
    kfree(table, sizeof(PtrHashTable), MEMORY_TAG_HASHTABLE);
}

PtrHashTableIterator ptr_hash_table_iterator_create(PtrHashTable *table) {
    PtrHashTableIterator iterator;
    iterator.table = table;
    iterator.bucketIndex = 0;
    iterator.entry = NULL;
    return iterator;
}

b8 ptr_hash_table_iterator_has_next(PtrHashTableIterator *iterator) {
    // If the current entry has a next, return true
    if (iterator->entry && iterator->entry->next) {
        return true;
    }
    
    // Otherwise, look for the next non-empty bucket
    for (size_t i = iterator->bucketIndex; i < iterator->table->capacity; i++) {
        if (iterator->table->buckets[i]) {
            return true;
        }
    }
    
    return false; // No more elements
}

void ptr_hash_table_iterator_next(PtrHashTableIterator *iterator, void **key, void **value) {
    // Move to the next entry if available
    if (iterator->entry && iterator->entry->next) {
        iterator->entry = iterator->entry->next;
    } else {
        // Otherwise, find the next non-empty bucket
        iterator->entry = NULL;
        for (; iterator->bucketIndex < iterator->table->capacity; iterator->bucketIndex++) {
            if (iterator->table->buckets[iterator->bucketIndex]) {
                iterator->entry = iterator->table->buckets[iterator->bucketIndex];
                // Move to the next bucket for future calls
                iterator->bucketIndex++;
                break;
            }
        }
    }
    
    if (iterator->entry) {
        *key = iterator->entry->key;
        *value = iterator->entry->value;
    }
}

b8 ptr_hash_table_contains(PtrHashTable *table, void *key) {
    return ptr_hash_table_get(table, key) != null;
}
