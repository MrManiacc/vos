#pragma once

#include "defines.h"

// Returns the length of the given string.
VAPI u64 string_length(const char* str);

VAPI char* string_duplicate(const char* str);

// Case-sensitive string comparison. True if the same, otherwise false.
VAPI b8 strings_equal(const char* str0, const char* str1);
