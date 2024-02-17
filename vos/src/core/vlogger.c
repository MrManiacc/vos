#include "vlogger.h"
#include "vasserts.h"
#include "platform/platform.h"
#include "vstring.h"
#include "vmem.h"
#include <stdlib.h> // For dynamic memory allocation

// TODO-temporary
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

b8 initialize_logging() {
    // TODO-create log file.
    return true;
}

void shutdown_logging() {
    // TODO-cleanup logging/write queued entries.
}

void log_output(log_level level, const char *call_location, const char *message, ...) {
#ifdef USE_LINE_NUMBER
    static const char *level_strings[6] = {"[FATAL]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]", "[TRACE]"};
#else
    static const char *level_strings[6] = {"[FATAL] - ", "[ERROR] - ", "[WARN] - ", "[INFO] - ", "[DEBUG] - ", "[TRACE] - "};
#endif
    
    b8 is_error = level < LOG_LEVEL_WARN;
    
    va_list arg_ptr;
            va_start(arg_ptr, message);
    
    // Determine required buffer size
    int required_length = vsnprintf(NULL, 0, message, arg_ptr) + 1;
            va_end(arg_ptr);
    
    // Allocate buffer dynamically based on required length
    char *formatted_message = (char *) platform_allocate(required_length, false);
    if (!formatted_message) {
        // Handle memory allocation failure
        // For simplicity, we just return here, but you might want to handle this more gracefully
        return;
    }
    
    // Actually format the string
            va_start(arg_ptr, message);
    vsnprintf(formatted_message, required_length, message, arg_ptr);
            va_end(arg_ptr);
    
    // Add a newline to the end of the message if it doesn't have one
    if (formatted_message[required_length - 2] != '\n') {
        formatted_message[required_length - 1] = '\n';
        formatted_message[required_length] = '\0';
    }
    
    // Output level with its color
    if (is_error) {
        platform_console_write_error(level_strings[level], level);
    } else {
        platform_console_write(level_strings[level], level);
    }
#ifdef USE_LINE_NUMBER
    // Output call location with the same color as level
    if (is_error) {
        platform_console_write_error(call_location, level);
    } else {
        platform_console_write(call_location, level);
    }
#endif
    // Reset to default color (white) and output the message
    if (is_error) {
        platform_console_write_error(formatted_message, LOG_LEVEL_NONE); // Assuming LOG_LEVEL_INFO is white
    } else {
        platform_console_write(formatted_message, LOG_LEVEL_NONE); // Assuming LOG_LEVEL_INFO is white
    }
    
    // Free the dynamically allocated memory
    platform_free(formatted_message, false);
}

void report_assertion_failure(const char *expression, const char *message, const char *file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Failure-%s, message-'%s', in file-%s, line-%d\n", expression, message,
               file, line);
}