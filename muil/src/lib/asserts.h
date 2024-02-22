#pragma once

#include "defines.h"

// Disable assertions by commenting out the below line.
#define VASSERTIONS_ENABLED

#ifdef VASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

VAPI void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

#define VASSERT(expr)                                                \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }

#define VASSERT_MSG(expr, message)                                        \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            debugBreak();                                                 \
        }                                                                 \
    }

#ifdef _DEBUG
#define VASSERT_DEBUG(expr)                                          \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }
#else
#define VASSERT_DEBUG(expr)  // Does nothing at all
#endif

#else
#define VASSERT(expr)               // Does nothing at all
#define VASSERT_MSG(expr, message)  // Does nothing at all
#define VASSERT_DEBUG(expr)         // Does nothing at all
#endif