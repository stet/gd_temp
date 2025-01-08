// Debug module implementation

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include "debug.h"

// Global debug level
int debug_level = DEBUG_NONE;

// Initialize debug module
void debug_init(int level) {
    debug_level = level;
}

// Internal helper for timestamp
static void print_timestamp(void) {
    time_t now;
    char timestamp[26];
    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0'; // Remove newline
    fprintf(stderr, "[%s] ", timestamp);
}

// Debug logging functions
void debug_error(const char *fmt, ...) {
    if (debug_level >= DEBUG_ERROR) {
        va_list args;
        print_timestamp();
        fprintf(stderr, "[ERROR] ");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void debug_warn(const char *fmt, ...) {
    if (debug_level >= DEBUG_WARN) {
        va_list args;
        print_timestamp();
        fprintf(stderr, "[WARN] ");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void debug_info(const char *fmt, ...) {
    if (debug_level >= DEBUG_INFO) {
        va_list args;
        print_timestamp();
        fprintf(stderr, "[INFO] ");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void debug_trace(const char *fmt, ...) {
    if (debug_level >= DEBUG_TRACE) {
        va_list args;
        print_timestamp();
        fprintf(stderr, "[TRACE] ");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

// Hex dump utility for debugging byte arrays
void debug_hex_dump(const char *prefix, const unsigned char *data, size_t len) {
    if (debug_level >= DEBUG_TRACE) {
        print_timestamp();
        fprintf(stderr, "[HEXDUMP] %s: ", prefix);
        for (size_t i = 0; i < len; i++) {
            fprintf(stderr, "%02x", data[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}
