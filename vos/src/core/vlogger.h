#pragma once

#include "defines.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// Disable debug and trace logging for release builds.
#ifndef USE_DEBUG_LOG
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5,
    LOG_LEVEL_NONE = 6
} log_level;

b8 initialize_logging();

void shutdown_logging();

VAPI void log_output(log_level level, const char *call_location, const char *message, ...);

// Helper macros for stringifying __LINE__ and formatting the file and line prefix
#define STRINGIFY_LINE_NUM(x) #x
#define LINE_NUM_STRING(x) STRINGIFY_LINE_NUM(x)

#define LOG_CALL_LOCATION "[" __FILE__ ":" LINE_NUM_STRING(__LINE__) "] - "

// Updated logging macros
#define vfatal(message, ...) log_output(LOG_LEVEL_FATAL, LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#ifndef verror
#define verror(message, ...) log_output(LOG_LEVEL_ERROR, LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#endif
#ifndef error
  #define error(message, call_location, ...) log_output(LOG_LEVEL_ERROR, call_location, message, ##__VA_ARGS__);
#endif
#if LOG_WARN_ENABLED == 1
#define vwarn(message, ...) log_output(LOG_LEVEL_WARN, LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#else
#define vwarn(message, ...)
#endif
#if LOG_INFO_ENABLED == 1
#define vinfo(message, ...) log_output(LOG_LEVEL_INFO, LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#else
#define vinfo(message, ...)
#endif
#if LOG_DEBUG_ENABLED == 1
#define vdebug(message, ...) log_output(LOG_LEVEL_DEBUG, LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#else
#define vdebug(message, ...)
#endif
#if LOG_TRACE_ENABLED == 1
#define vtrace(message, ...) log_output(LOG_LEVEL_TRACE, LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#else
#define vtrace(message, ...)
#endif