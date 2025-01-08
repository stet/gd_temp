#ifndef PATTERN_H
#define PATTERN_H

#define _GNU_SOURCE  // For strcasestr
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>  // For strcasestr and strcasecmp
#include <regex.h>     // For regex_t

// Pattern types
typedef enum {
    PATTERN_TYPE_PREFIX = 1,     // Match at start (e.g., "1ABC")
    PATTERN_TYPE_SUFFIX = 2,     // Match at end (e.g., "XYZ")
    PATTERN_TYPE_CONTAINS = 3,   // Match anywhere (e.g., "COOL")
    PATTERN_TYPE_EXACT = 4,      // Exact match
    PATTERN_TYPE_REGEX = 5,      // Regular expression
    PATTERN_TYPE_WILDCARD = 6,   // Wildcard pattern (e.g., "1*ABC*Z")
    PATTERN_TYPE_MULTI = 7,      // Multiple patterns (AND/OR)
    PATTERN_TYPE_ALTERNATION = 8 // Alternating chars (e.g., "1[AB][12]")
} pattern_type_t;

// Pattern combination type for PATTERN_TYPE_MULTI
typedef enum {
    PATTERN_COMBINE_AND = 1, // All patterns must match
    PATTERN_COMBINE_OR = 2   // Any pattern must match
} pattern_combine_t;

// Character class for PATTERN_TYPE_ALTERNATION
typedef struct {
    char chars[58];  // Base58 alphabet
    size_t count;    // Number of valid chars
} pattern_charclass_t;

// Pattern segment for PATTERN_TYPE_WILDCARD
typedef struct {
    char *str;
    size_t len;
    bool is_wildcard;
} pattern_segment_t;

// Pattern structure
struct Pattern {
    pattern_type_t type;     // Pattern type
    bool case_sensitive;     // Case sensitivity
    
    union {
        struct {  // For string patterns
            char *str;
            size_t len;
            bool has_regex;
            regex_t regex;
        } str;
        
        struct {  // For PATTERN_TYPE_MULTI
            struct Pattern **patterns;
            size_t count;
            pattern_combine_t type;
        } multi;
        
        struct {  // For PATTERN_TYPE_ALTERNATION
            pattern_charclass_t *classes;
            size_t count;
        } alt;
        
        struct {  // For PATTERN_TYPE_WILDCARD
            pattern_segment_t *segments;
            size_t segment_count;
        } wildcard;
    };
    
    double probability;      // Estimated match probability
};

/**
 * Compile a pattern for efficient matching
 * 
 * @param pattern String pattern to compile
 * @param type Type of pattern matching to use
 * @param case_sensitive Whether matching should be case sensitive
 * @return Compiled pattern or NULL on error
 */
struct Pattern *pattern_compile(const char *pattern, pattern_type_t type, bool case_sensitive);

/**
 * Create a multi-pattern matcher
 * 
 * @param patterns Array of patterns to combine
 * @param count Number of patterns
 * @param combine_type How to combine patterns (AND/OR)
 * @param case_sensitive Whether matching should be case sensitive
 * @return Compiled pattern or NULL on error
 */
struct Pattern *pattern_compile_multi(const char **patterns, size_t count,
                             pattern_combine_t combine_type, bool case_sensitive);

/**
 * Create an alternation pattern
 * 
 * @param pattern Pattern with character classes (e.g., "1[AB][12]")
 * @param case_sensitive Whether matching should be case sensitive
 * @return Compiled pattern or NULL on error
 */
struct Pattern *pattern_compile_alternation(const char *pattern, bool case_sensitive);

/**
 * Match a string against a compiled pattern
 * 
 * @param pattern Compiled pattern to match against
 * @param str String to match
 * @return true if matches, false otherwise
 */
bool pattern_match(const struct Pattern *pattern, const char *str);

/**
 * Get the estimated probability of a match (1/keyspace)
 * Used for performance estimation
 * 
 * @param pattern Compiled pattern
 * @return Estimated probability (0.0 to 1.0)
 */
double pattern_probability(const struct Pattern *pattern);

/**
 * Get a human-readable description of the pattern
 * 
 * @param pattern Compiled pattern
 * @param buf Buffer to store description
 * @param size Size of buffer
 * @return 0 on success, -1 on error
 */
int pattern_describe(const struct Pattern *pattern, char *buf, size_t size);

/**
 * Free a compiled pattern
 * 
 * @param pattern Pattern to free
 */
void pattern_free(struct Pattern *pattern);

#endif // PATTERN_H
