#pragma once

#include "defines.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1

// Disable debug and trace logging for release builds.


typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5,
    LOG_LEVEL_NONE = 6
} log_level;

 void log_output(log_level level, const char *call_location, const char *message, ...);

// Helper macros for stringifying __LINE__ and formatting the file and line prefix
#define __STRINGIFY_LINE_NUM(x) #x
#define __LINE_NUM_STRING(x) __STRINGIFY_LINE_NUM(x)

#define __LOG_CALL_LOCATION "[" __FILE__ ":" __LINE_NUM_STRING(__LINE__) "] - "

// Updated logging macros
#define vfatal(message, ...) log_output(LOG_LEVEL_FATAL, __LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#define verror(message, ...) log_output(LOG_LEVEL_ERROR, __LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#define vwarn(message, ...) log_output(LOG_LEVEL_WARN, __LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#define vinfo(message, ...) log_output(LOG_LEVEL_INFO, __LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#define vdebug(message, ...) log_output(LOG_LEVEL_DEBUG, __LOG_CALL_LOCATION, message, ##__VA_ARGS__);
#define vtrace(message, ...) log_output(LOG_LEVEL_TRACE, __LOG_CALL_LOCATION, message, ##__VA_ARGS__);
