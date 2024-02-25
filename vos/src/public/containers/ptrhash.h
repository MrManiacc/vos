/**
 * Created by jraynor on 2/12/2024.
 */
#pragma once

#include "defines.h"

#include <stdlib.h>

// Represents an entry in the hash table for storing key-value pairs.
typedef struct PtrHashTableEntry {
    void *key;                      // Pointer used as a key.
    void *value;                    // Associated value with the key.
    struct PtrHashTableEntry *next; // Next entry in the chain for handling collisions.
} PtrHashTableEntry;

// Hash table structure for storing pointer key-value pairs.
typedef struct {
    PtrHashTableEntry **buckets; // Array of bucket pointers for the hash table entries.
    u32 capacity;             // Number of buckets in the hash table.
} PtrHashTable;

typedef struct PtrHashTableIterator {
    PtrHashTable *table;
    size_t bucketIndex;
    PtrHashTableEntry *entry;
} PtrHashTableIterator;

/**
 * Creates a new hash table iterator.
 *
 * @param table The hash table to iterate over.
 * @return A new hash table iterator.
 */
PtrHashTableIterator ptr_hash_table_iterator_create(PtrHashTable *table);

/**
 * Moves the iterator to the next entry in the hash table.
 *
 * @param iterator
 * @return
 */
b8 ptr_hash_table_iterator_has_next(PtrHashTableIterator *iterator);

/**
 * Retrieves the next key-value pair from the hash table.
 *
 * @param iterator The hash table iterator.
 * @param key Pointer to store the key.
 * @param value Pointer to store the value.
 */
void ptr_hash_table_iterator_next(PtrHashTableIterator *iterator, void **key, void **value);

/**
 * Creates a new hash table.
 *
 * @param capacity The initial capacity of the hash table (number of buckets).
 * @return A pointer to the newly created hash table.
 */
PtrHashTable *ptr_hash_table_create(u32 capacity);

/**
 * Inserts or updates a value in the hash table.
 *
 * @param table The hash table in which to set the value.
 * @param key Pointer used as the key in the hash table.
 * @param value The value to associate with the key.
 */
void ptr_hash_table_set(PtrHashTable *table, void *key, void *value);

/**
 * Retrieves a value from the hash table.
 *
 * @param table The hash table from which to retrieve the value.
 * @param key The pointer key for which to look up the value.
 * @return The value associated with the key, or NULL if the key is not found.
 */
void *ptr_hash_table_get(PtrHashTable *table, void *key);

/**
 * Removes an entry from the hash table.
 *
 * @param table The hash table from which to remove the entry.
 * @param key The pointer key of the entry to remove.
 */
void ptr_hash_table_remove(PtrHashTable *table, void *key);

/**
 * Destroys the hash table and frees all associated memory.
 *
 * @param table The hash table to destroy.
 */
void ptr_hash_table_destroy(PtrHashTable *table);

/**
 * Checks if the hash table contains the given key.
 * @param table The hash table to check.
 * @param key The key to check for.
 * @return True if the key is in the hash table, false otherwise.
 */
b8 ptr_hash_table_contains(PtrHashTable *table, void *key);