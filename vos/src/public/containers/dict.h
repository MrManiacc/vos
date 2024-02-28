/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once

#include "defines.h"

typedef u64 (hash_function)(const char *);


typedef struct Entry {
    char *key;
    void *value;
    struct Entry *next;
} Entry;

typedef struct Dict {
    u32 size;
    hash_function *hash_func;
    Entry **elements;
} Dict;

typedef struct DictIter {
    Dict *table;
    Entry *entry;
    u32 index;
} DictIter;

/**
 * @brief creates a new dictionary table with the given size and hash function
 * @param size the size of the table
 * @param hash_func the hash function to use
 * @return a new dictionary table
 */
Dict *dict_create(u64 size, hash_function *hash_func);

/**
 * Creates a new dictionary table with the default size and hash function
 * @return a new dictionary table
 */
Dict *dict_new();

/**
 * @brief Creates a new dictionary with specified size.
 *
 * This function creates a new dictionary with the specified size. The dictionary
 * uses a hashing function to compute the index of each element. The size parameter
 * determines the number of elements the dictionary can hold.
 *
 * @param size The size of the dictionary.
 * @return A pointer to the newly created dictionary.
 */
Dict *dict_create_sized(u64 size);

/**
 * @brief destroys the given dictionary table
 * @param table  the table to destroy
 */
void dict_delete(Dict *table);

/**
 * @brief inserts the given key and value into the table
 * @param table the table to insert into
 * @param key the key to insert
 * @param value the value to insert
 * @return true if the key was inserted, false if the key already exists
 */
b8 dict_set(Dict *table, const char *key, void *value);

/**
 * @brief looks up the given key in the table
 * @param table the table to look in
 * @param key the key to look for
 * @return the value associated with the key, or null if the key does not exist
 */
void *dict_get(Dict *table, const char *key);

/**
 * @brief Checks if a key is present in the dictionary.
 *
 * This function checks if a given key is present in the dictionary.
 *
 * @param table A pointer to the dictionary.
 * @param key The key to search for.
 * @return True if the key is found, false otherwise.
 */
b8 dict_contains(Dict *table, const char *key);

/**
 * @brief removes the given key from the table
 * @param table the table to remove from
 * @param key the key to remove
 */
void *dict_remove(Dict *table, const char *key);


/**
 * @brief Get the size of the dictionary.
 *
 * This function returns the size of the provided dictionary.
 *
 * @param table A pointer to the dictionary.
 * @return The size of the dictionary.
 */
u32 dict_size(Dict *table);

/**
 * @brief converts the given table to a string
 * @param table
 * @return
 */
char *dict_to_string(Dict *table);

/**
 * @brief clears the given table
 * @param table  the table to check
 */
void dict_clear(Dict *table);

/**
 * @brief creates a new iterator for the given table
 */
DictIter dict_iterator(Dict *table);

/**
 * @brief moves the iterator to the next element
 * @param it the iterator to move
 * @return true if the iterator was moved, false if the iterator is at the end
 */
b8 dict_next(DictIter *it);

#define dict_for(table, type, varName) \
for (Entry *varName##_entry = NULL; \
varName##_entry == NULL; ) \
for (type varName = NULL; \
varName == NULL; ) \
for (DictIter iter = dict_iterator(table); \
dict_next(&iter) && \
(varName##_entry = iter.entry, varName = (type)iter.entry->value, true); )


#define dict_for_each(table, type, callback) \
    {                                        \
    DictIter iterator = dict_iterator(table); \
    while (dict_next(&iterator)) { \
        Entry *entry = iterator.entry; \
        char* key = entry->key;               \
        type *value = (type *)entry->value; \
        callback; \
    }                                         \
    }
