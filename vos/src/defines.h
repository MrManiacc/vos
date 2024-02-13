#pragma once
// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef char b8;

/** @brief A range, typically of memory */
typedef struct range {
    /** @brief The offset in bytes. */
    u64 offset;
    /** @brief The size in bytes. */
    u64 size;
} range;

/** @brief A range, typically of memory */
typedef struct range32 {
    /** @brief The offset in bytes. */
    i32 offset;
    /** @brief The size in bytes. */
    i32 size;
} range32;


// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

#define true 1
#define false 0
#define null ((void*) 0)
/**
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID_U64 18446744073709551615UL
#define INVALID_ID 4294967295U
#define INVALID_ID_U16 65535U
#define INVALID_ID_U8 255U

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define KPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define KPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define KPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define KPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define KPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define KPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define KPLATFORM_IOS 1
#define KPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define KPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef KEXPORT
// Exports
#ifdef _MSC_VER
#define VAPI __declspec(dllexport)
#else
#define VAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define VAPI __declspec(dllimport)
#else
#define VAPI
#endif
#endif

/**
 * @brief Clamps value to a range of min and max (inclusive).
 * @param value The value to be clamped.
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @returns The clamped value.
 */
#define KCLAMP(value, min, max) ((value <= min) ? min : (value >= max) ? max \
                                                                       : value)

// Inlining
#if defined(__clang__) || defined(__gcc__)
/** @brief Inline qualifier */
#define KINLINE __attribute__((always_inline)) inline

/** @brief No-inline qualifier */
#define KNOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)

/** @brief Inline qualifier */
#define KINLINE __forceinline

/** @brief No-inline qualifier */
#define KNOINLINE __declspec(noinline)
#else

/** @brief Inline qualifier */
#define KINLINE static inline

/** @brief No-inline qualifier */
#define KNOINLINE
#endif

/** @brief Gets the number of bytes from amount of gibibytes (GiB) (1024*1024*1024) */
#define GIBIBYTES(amount) ((amount)*1024ULL * 1024ULL * 1024ULL)
/** @brief Gets the number of bytes from amount of mebibytes (MiB) (1024*1024) */
#define MEBIBYTES(amount) ((amount)*1024ULL * 1024ULL)
/** @brief Gets the number of bytes from amount of kibibytes (KiB) (1024) */
#define KIBIBYTES(amount) ((amount)*1024ULL)

/** @brief Gets the number of bytes from amount of gigabytes (GB) (1000*1000*1000) */
#define GIGABYTES(amount) ((amount)*1000ULL * 1000ULL * 1000ULL)
/** @brief Gets the number of bytes from amount of megabytes (MB) (1000*1000) */
#define MEGABYTES(amount) ((amount)*1000ULL * 1000ULL)
/** @brief Gets the number of bytes from amount of kilobytes (KB) (1000) */
#define KILOBYTES(amount) ((amount)*1000ULL)

KINLINE u64 get_aligned(u64 operand, u64 granularity) {
    return ((operand + (granularity - 1)) & ~(granularity - 1));
}

KINLINE range get_aligned_range(u64 offset, u64 size, u64 granularity) {
    return (range) {get_aligned(offset, granularity), get_aligned(size, granularity)};
}

#define KMIN(x, y) (x < y ? x : y)
#define KMAX(x, y) (x > y ? x : y)
                                                                      