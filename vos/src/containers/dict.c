#include "dict.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "core/vstring.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
#define DEFAULT_DICT_SIZE 25

static u64 dict_default_hash(const char *key) {
    u64 hash = FNV_OFFSET;
    for (const char *p = key; *p; p++) {
        hash ^= (u64) (unsigned char) (*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

static u64 has_table_index(Dict *dict, const char *key);

static u64 has_table_index(Dict *dict, const char *key) {
    u64 result = dict->hash_func(key) % dict->size;
    return result;
}

Dict *dict_create(u64 size, hash_function *hash_func) {
    Dict *table = kallocate(sizeof(Dict), MEMORY_TAG_DICT);
    table->size = size;
    table->hash_func = hash_func;
    table->elements = kallocate(sizeof(entry *) * size, MEMORY_TAG_DICT);
    for (u32 i = 0; i < size; i++) {
        table->elements[i] = NULL;
    }
    return table;
}

/**
 * Creates a new dictionary table with the default size and hash function
 * @return a new dictionary table
 */
Dict *dict_create_default() {
    return dict_create(DEFAULT_DICT_SIZE, dict_default_hash);
}


Dict *dict_create_sized(u64 size) {
    return dict_create(size, dict_default_hash);
}

void dict_destroy(Dict *table) {
    //Delete all keys
    for (u32 i = 0; i < table->size; i++) {
        entry *e = table->elements[i];
        while (e != NULL) {
            entry *next = e->next;
            kfree(e->key, string_length(e->key) + 1, MEMORY_TAG_STRING);
            kfree(e, sizeof(entry), MEMORY_TAG_DICT);
            e = next;
        }
    }
    kfree(table->elements, sizeof(entry *) * table->size, MEMORY_TAG_DICT);
    kfree(table, sizeof(Dict), MEMORY_TAG_DICT);
}

char *dict_to_string(Dict *table) {
    char *result = string_duplicate("{");
    for (u32 i = 0; i < table->size; i++) {
        entry *e = table->elements[i];
        while (e != NULL) {
            u64 hash_key = table->hash_func(e->key);
            result = string_format("%s\n\t0x%x: {\n\t\tKey: %s,\n\t\tValue Pointer: 0x%4p\n\t}\n",
                                   result,
                                   hash_key,
                                   e->key,
                                   e->value);
            if (e->next != NULL)
                result = string_concat(result, ",");
            e = e->next;
        }
    }
    result = string_concat(result, "\n");
    result[string_length(result) - 1] = '}';
    return result;
}

b8 dict_set(Dict *table, const char *key, void *value) {
    if (key == NULL || value == NULL)
        return false;
    u64 index = has_table_index(table, key);
    
    if (dict_get(table, key) != NULL)
        return false;
    
    entry *e = kallocate(sizeof(entry), MEMORY_TAG_DICT);
    e->key = string_duplicate(key);
    e->value = value;
    
    e->next = table->elements[index];
    table->elements[index] = e;
    return true;
}

void *dict_get(Dict *table, const char *key) {
    if (key == NULL || table == NULL)
        return NULL;
    u64 index = has_table_index(table, key);
    
    entry *temp = table->elements[index];
    while (temp != NULL && !strings_equal(temp->key, key)) {
        temp = temp->next;
    }
    if (temp == NULL)
        return NULL;
    return temp->value;
}

void *dict_remove(Dict *table, const char *key) {
    if (key == NULL || table == NULL)
        return NULL;
    u64 index = has_table_index(table, key);
    entry *temp = table->elements[index];
    entry *prev = NULL;
    while (temp != NULL && !strings_equal(temp->key, key)) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL)
        return NULL;
    if (prev == NULL) {
        //Deleting from the head of the list
        table->elements[index] = temp->next;
    } else {
        //Deleting from the middle of the list
        prev->next = temp->next;
    }
    void *result = temp->value;
    kfree(temp->key, string_length(temp->key) + 1, MEMORY_TAG_STRING);
    kfree(temp, sizeof(entry), MEMORY_TAG_DICT);
    return result;
}

void dict_clear(Dict *table) {
    for (u32 i = 0; i < table->size; i++) {
        entry *e = table->elements[i];
        while (e != NULL) {
            entry *next = e->next;
            kfree(e->key, string_length(e->key) + 1, MEMORY_TAG_STRING);
            kfree(e, sizeof(entry), MEMORY_TAG_DICT);
            e = next;
        }
        table->elements[i] = NULL;
    }
}

/**
 * @brief creates a new iterator for the given table
 */
DictIterator dict_iterator(Dict *table) {
    if (table == NULL)
        return (DictIterator) {0};
    DictIterator it;
    it.table = table;
    it.index = 0;
    it.entry = NULL;
    return it;
}

/**
 * @brief moves the iterator to the next element
 * @param it the iterator to move
 * @return true if the iterator was moved, false if the iterator is at the end
 */
b8 dict_next(DictIterator *it) {
    if (it->table == NULL)
        return false;
    while (it->index < it->table->size) {
        it->entry = it->table->elements[it->index++];
        if (it->entry != NULL) { // Found a valid entry
            return true;
        }
    }
    return false; // No more entries left
}

b8 dict_contains(Dict *table, const char *key) {
    return dict_get(table, key) != NULL;
}

u32 dict_size(Dict *table) {
    //Counts the number of elements in the table
    u32 count = 0;
    for (u32 i = 0; i < table->size; i++) {
        entry *e = table->elements[i];
        while (e != NULL) {
            count++;
            e = e->next;
        }
    }
    return count;
}

