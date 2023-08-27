/**
 * Created by jraynor on 8/27/2023.
 */
#pragma once

#include "defines.h"

typedef u64 (hash_function)(const char *);

typedef struct dict dict;

typedef struct entry {
  char *key;
  void *object;
  struct entry *next;
} entry;

typedef struct dict {
  u32 size;
  hash_function *hash_func;
  entry **elements;
} dict;

typedef struct _dict_iterator {
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
b8 dict_insert(dict *table, const char *key, void *value);
/**
 * @brief looks up the given key in the table
 * @param table the table to look in
 * @param key the key to look for
 * @return the value associated with the key, or null if the key does not exist
 */
void *dict_lookup(dict *table, const char *key);
/**
 * @brief removes the given key from the table
 * @param table the table to remove from
 * @param key the key to remove
 */
void *dict_remove(dict *table, const char *key);

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

