/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once

#include "defines.h"

typedef u64 (hash_function)(const char *);


typedef struct entry {
    char *key;
    void *value;
    struct entry *next;
} entry;

typedef struct dict {
    u32 size;
    hash_function *hash_func;
    entry **elements;
} dict;

typedef struct idict {
    dict *table;
    entry *entry;
    u32 index;
} idict;

/**
 * @brief creates a new dictionary table with the given size and hash function
 * @param size the size of the table
 * @param hash_func the hash function to use
 * @return a new dictionary table
 */
dict *dict_create(u64 size, hash_function *hash_func);

/**
 * Creates a new dictionary table with the default size and hash function
 * @return a new dictionary table
 */
dict *dict_create_default();

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
dict *dict_create_sized(u64 size);

/**
 * @brief destroys the given dictionary table
 * @param table  the table to destroy
 */
void dict_destroy(dict *table);

/**
 * @brief inserts the given key and value into the table
 * @param table the table to insert into
 * @param key the key to insert
 * @param value the value to insert
 * @return true if the key was inserted, false if the key already exists
 */
b8 dict_set(dict *table, const char *key, void *value);

/**
 * @brief looks up the given key in the table
 * @param table the table to look in
 * @param key the key to look for
 * @return the value associated with the key, or null if the key does not exist
 */
void *dict_get(dict *table, const char *key);

/**
 * @brief Checks if a key is present in the dictionary.
 *
 * This function checks if a given key is present in the dictionary.
 *
 * @param table A pointer to the dictionary.
 * @param key The key to search for.
 * @return True if the key is found, false otherwise.
 */
b8 dict_contains(dict *table, const char *key);

/**
 * @brief removes the given key from the table
 * @param table the table to remove from
 * @param key the key to remove
 */
void *dict_remove(dict *table, const char *key);


/**
 * @brief Get the size of the dictionary.
 *
 * This function returns the size of the provided dictionary.
 *
 * @param table A pointer to the dictionary.
 * @return The size of the dictionary.
 */
u32 dict_size(dict *table);

/**
 * @brief converts the given table to a string
 * @param table
 * @return
 */
char *dict_to_string(dict *table);

/**
 * @brief clears the given table
 * @param table  the table to check
 */
void dict_clear(dict *table);

/**
 * @brief creates a new iterator for the given table
 */
idict dict_iterator(dict *table);

/**
 * @brief moves the iterator to the next element
 * @param it the iterator to move
 * @return true if the iterator was moved, false if the iterator is at the end
 */
b8 dict_next(idict *it);

