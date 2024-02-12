#pragma once

#include "defines.h"

// Returns the length of the given string.
VAPI u64 string_length(const char *str);

VAPI char *string_duplicate(const char *str);

// Case-sensitive string comparison. True if the same, otherwise false.
VAPI b8 strings_equal(const char *str0, const char *str1);

// Case-insensitive string comparison. True if the same, otherwise false.
VAPI b8 string_contains(const char *str, const char *substr);

// Splits the given string into an array of strings using the given delimiter.
// The returned array must be freed using darray_free.
VAPI char *string_split(const char *str, const char *delimiter);

VAPI b8 string_starts_with(const char *str, const char *substr);

VAPI b8 string_ends_with(const char *str, const char *substr);

// Combines two strings into a new string. and returns a new one.
// The returned string must be freed using kfree.
// The original strings are freed
VAPI char *string_concat(const char *str0, const char *str1);

VAPI char *string_replace(const char *str, const char *substr, const char *replacement);
// Returns the index of the first occurrence of the given character in the given string.
// Returns -1 if the character is not found.
VAPI i64 string_index_of(const char *str, char c);

VAPI i32 string_split_count(const char *str, const char *delimiter);

VAPI char *string_split_at(const char *str, const char *delimiter, u64 index);

VAPI char *string_trim(const char *str);

VAPI char *string_format(const char *str, ...);

VAPI char *string_to_lower(const char *input);

VAPI char *string_append(const char *str, const char *append);