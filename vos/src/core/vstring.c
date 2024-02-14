#include "core/vstring.h"
#include "vmem.h"
#include "vlogger.h"
#include "containers/dict.h"
#include "containers/ptrhash.h"
#include "containers/darray.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>


// a tracked allocation of a string
typedef struct StringAllocation {
    char *string;
    u64 length;
    u64 tag;
} StringAllocation;

static void **string_allocations = NULL;

void strings_initialize() {
    string_allocations = darray_create(StringAllocation*);
}

void strings_shutdown() {
    if (!string_allocations) return;
    
    for (u64 i = 0; i < darray_length(string_allocations); ++i) {
        StringAllocation *allocation = *(StringAllocation **) darray_get(string_allocations, i);
        if (allocation) {
            // free the string and the allocation
//            vdebug("Freeing string: %s", allocation->string)
            kfree(allocation->string, allocation->length + 1, allocation->tag); // Free the string
            kfree(allocation, sizeof(StringAllocation), MEMORY_TAG_STRING); // Free the allocation struct
            //remove from the hash table
        }
    }
    
    darray_destroy(string_allocations);
    string_allocations = NULL;
}

// Allocate a string with tracking
char *string_allocate_empty(u64 length) {
    char *copiedString = kallocate(length + 1, MEMORY_TAG_STRING);
    kzero_memory(copiedString, length + 1);
    
    StringAllocation *allocation = kallocate(sizeof(StringAllocation), MEMORY_TAG_STRING);
    allocation->string = copiedString;
    allocation->length = length;
    allocation->tag = MEMORY_TAG_STRING;
    
    darray_push(StringAllocation*, string_allocations, allocation);
    return copiedString;
}


char *string_allocate_sized(const char *input, u64 length) {
    
    char *copiedString = kallocate(length + 1, MEMORY_TAG_STRING);
    kcopy_memory(copiedString, input, length + 1);
    //append the null terminator if it's not there
    if (copiedString[length] != '\0') {
        copiedString[length] = '\0';
    }
    StringAllocation *allocation = kallocate(sizeof(StringAllocation), MEMORY_TAG_STRING);
    allocation->string = copiedString;
    allocation->length = length;
    allocation->tag = MEMORY_TAG_STRING;
    
    darray_push(StringAllocation *, string_allocations, allocation);
    
    return copiedString;
}

char *string_allocate(const char *input) {
    if (!input) return null;
    return string_allocate_sized(input, string_length(input));
}

void string_deallocate(char *str) {
    if (!str) return;
    //Finds the allocation and removes it from the darray
    for (u64 i = 0; i < darray_length(string_allocations); ++i) {
        StringAllocation *allocation = *(StringAllocation **) darray_get(string_allocations, i);
        if (strings_equal(allocation->string, str)) {
            kfree(allocation->string, allocation->length + 1, allocation->tag); // Free the string
            kfree(allocation, sizeof(StringAllocation), MEMORY_TAG_STRING); // Free the allocation struct
            darray_remove(string_allocations, allocation);
            return;
        }
    }
}


inline u64 string_length(const char *str) {
    return strlen(str);
}

inline char *string_duplicate(const char *str) {
    if (!str) return null;
    return string_allocate(str);;
}

// Case-sensitive string comparison. True if the same, otherwise false.
inline b8 strings_equal(const char *str0, const char *str1) {
    return strcmp(str0, str1) == 0;
}

// Case-insensitive string comparison. True if the same, otherwise false.
inline b8 string_contains(const char *str, const char *substr) {
    return strstr(str, substr) != null;
}

// Splits the given string into an array of strings using the given delimiter.
// The returned array must be freed using darray_free.
inline char *string_split(const char *str, const char *delimiter) {
    char *copy = string_duplicate(str);
    char *token = strtok(copy, delimiter);
    kfree(copy, string_length(copy) + 1, MEMORY_TAG_STRING);
    return token;
}

b8 string_starts_with(const char *str, const char *substr) {
    return strncmp(str, substr, string_length(substr)) == 0;
}

char *string_concat(const char *str0, const char *str1) {
    if (!str0 || !str1) return null;
    u64 str0_len = string_length(str0);
    u64 str1_len = string_length(str1);
    char *result = kallocate(str0_len + str1_len + 1, MEMORY_TAG_STRING); // Allocate once
    kcopy_memory(result, str0, str0_len);
    kcopy_memory(result + str0_len, str1, str1_len + 1); // Include null terminator
    result[str0_len + str1_len] = '\0';
    
    // Track the allocation right after creating it
    StringAllocation *allocation = kallocate(sizeof(StringAllocation), MEMORY_TAG_STRING);
    allocation->string = result;
    allocation->length = str0_len + str1_len;
    allocation->tag = MEMORY_TAG_STRING;
    darray_push(StringAllocation *, string_allocations, allocation);
    return result;
}

// Lowercase conversion with tracking
char *string_to_lower(const char *input) {
    if (input == NULL) return NULL;
    u32 len = strlen(input);
    char *lowercase_str = kallocate(len + 1, MEMORY_TAG_STRING);
    for (u32 i = 0; i < len; ++i) {
        lowercase_str[i] = input[i] >= 'A' && input[i] <= 'Z' ? input[i] + 32 : input[i];
    }
    lowercase_str[len] = '\0';
    string_allocate_sized(lowercase_str, len);
    return lowercase_str;
}

b8 string_ends_with(const char *str, const char *substr) {
    u64 str_len = string_length(str);
    u64 substr_len = string_length(substr);
    if (substr_len > str_len) return false;
    return strncmp(str + str_len - substr_len, substr, substr_len) == 0;
}

// Splits the given string into an array of strings using the given delimiter.
inline char *string_split_at(const char *str, const char *delimiter, u64 index) {
    char *copy = string_duplicate(str);
    char *token = strtok(copy, delimiter);
    u64 i = 0;
    while (token != null) {
        if (i == index) {
            char *output = string_duplicate(token);
            kfree(copy, string_length(copy) + 1, MEMORY_TAG_STRING);
//            vdebug("string_split_at: %s", output);
            return output;
        }
        token = strtok(null, delimiter);
        i++;
    }
    return null;
}

inline i32 string_split_count(const char *str, const char *delimiter) {
    char *copy = string_duplicate(str);
    char *token = strtok(copy, delimiter);
    i32 count = 0;
    while (token != null) {
        count++;
        token = strtok(null, delimiter);
    }
    kfree(copy, string_length(str) + 1, MEMORY_TAG_STRING);
    return count;
}

// Returns the index of the first occurrence of the given character in the given string.
// Returns -1 if the character is not found.
inline i64 string_index_of(const char *str, char c) {
    char *ptr = strchr(str, c);
    if (ptr == null) {
        return -1;
    }
    return ptr - str;
}

char *string_replace(const char *str, const char *substr, const char *replacement) {
    char *copy = string_duplicate(str);
    char *token = strtok(copy, substr);
    char *result = string_duplicate(token);
    token = strtok(null, substr);
    while (token != null) {
        result = string_concat(result, replacement);
        result = string_concat(result, token);
        token = strtok(null, substr);
    }
    kfree(copy, string_length(copy) + 1, MEMORY_TAG_STRING);
    return result;
}

// Format string with tracking
char *string_format(const char *str, ...) {
    va_list args;
            va_start(args, str);
    char formatted[1024]; // Assuming 1024 is enough; adjust as needed
    vsprintf(formatted, str, args);
            va_end(args);
    return string_allocate(formatted);;
}


// Append strings with tracking
char *string_append(const char *str, const char *append) {
    if (!str || !append) return null;
    return string_concat(str, append); // Reuse string_concat for efficiency and tracking
}

char *string_repeat(const char *str, u64 count) {
    if (!str) return null;
    u64 str_len = string_length(str);
    char *result = kallocate(str_len * count + 1, MEMORY_TAG_STRING); // Allocate once
    for (u64 i = 0; i < count; ++i) {
        kcopy_memory(result + i * str_len, str, str_len);
    }
    result[str_len * count] = '\0';
    // Track the allocation right after creating it
    StringAllocation *allocation = kallocate(sizeof(StringAllocation), MEMORY_TAG_STRING);
    allocation->string = result;
    allocation->length = str_len * count;
    allocation->tag = MEMORY_TAG_STRING;
    darray_push(StringAllocation*, string_allocations, allocation);
    return result;
}



