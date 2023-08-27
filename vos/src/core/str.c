#include "core/str.h"
#include "core/mem.h"
#include "logger.h"

#include <string.h>

inline u64 string_length(const char *str) {
    return strlen(str);
}

inline char *string_duplicate(const char *str) {
    u64 length = string_length(str);
    char *copy = kallocate(length + 1, MEMORY_TAG_STRING);
    kcopy_memory(copy, str, length + 1);
    return copy;
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
    u64 str0_len = string_length(str0);
    u64 str1_len = string_length(str1);
    char *result = kallocate(str0_len + str1_len + 1, MEMORY_TAG_STRING);
    kcopy_memory(result, str0, str0_len);
    kcopy_memory(result + str0_len, str1, str1_len);
    result[str0_len + str1_len] = '\0';
    return result;
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
            vdebug("string_split_at: %s", output);
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
/**
 * removes the first occurrence of the given character in the given string.
 * @return
 */
u32 string_dealloc_all() {

}
