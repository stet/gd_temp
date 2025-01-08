#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// Debug levels
#define DEBUG_NONE 0
#define DEBUG_ERROR 1
#define DEBUG_WARN 2
#define DEBUG_INFO 3
#define DEBUG_TRACE 4

// Global debug level
extern int debug_level;

// Initialize debug module
void debug_init(int level);

// Debug logging functions
void debug_error(const char *fmt, ...);
void debug_warn(const char *fmt, ...);
void debug_info(const char *fmt, ...);
void debug_trace(const char *fmt, ...);

// Hex dump utility for debugging byte arrays
void debug_hex_dump(const char *prefix, const unsigned char *data, size_t len);

#endif /* DEBUG_H */
