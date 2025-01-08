#define _GNU_SOURCE  // For strcasestr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For strcasestr and strcasecmp
#include <ctype.h>
#include <regex.h>
#include <math.h>
#include "pattern.h"
#include "error.h"

// Maximum pattern length
#define PATTERN_MAX_LENGTH 64
#define PATTERN_MAX_MULTI 8
#define PATTERN_MAX_CHARCLASS 58

// Base58 character set for Bitcoin addresses
static const char *base58_chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const int base58_len = 58;

// Forward declarations
static bool match_wildcard(const struct Pattern *pattern, const char *str);
static bool match_alternation(const struct Pattern *pattern, const char *str);
static double calc_wildcard_probability(const struct Pattern *pattern);
static double calc_alternation_probability(const struct Pattern *pattern);
static struct Pattern *compile_wildcard(const char *pattern, bool case_sensitive);

bool pattern_match(const struct Pattern *pattern, const char *str) {
    if (!pattern || !str) return false;
    
    switch (pattern->type) {
        case PATTERN_TYPE_PREFIX: {
            if (strlen(str) < pattern->str.len) return false;
            return pattern->case_sensitive ?
                strncmp(str, pattern->str.str, pattern->str.len) == 0 :
                strncasecmp(str, pattern->str.str, pattern->str.len) == 0;
        }
            
        case PATTERN_TYPE_SUFFIX: {
            size_t str_len = strlen(str);
            if (str_len < pattern->str.len) return false;
            const char *suffix = str + (str_len - pattern->str.len);
            return pattern->case_sensitive ?
                strcmp(suffix, pattern->str.str) == 0 :
                strcasecmp(suffix, pattern->str.str) == 0;
        }
            
        case PATTERN_TYPE_CONTAINS: {
            if (pattern->case_sensitive) {
                return strstr(str, pattern->str.str) != NULL;
            } else {
                return strcasestr(str, pattern->str.str) != NULL;
            }
        }
            
        case PATTERN_TYPE_EXACT:
            return pattern->case_sensitive ?
                strcmp(str, pattern->str.str) == 0 :
                strcasecmp(str, pattern->str.str) == 0;
            
        case PATTERN_TYPE_REGEX:
            return pattern->str.has_regex &&
                regexec(&pattern->str.regex, str, 0, NULL, 0) == 0;
            
        case PATTERN_TYPE_WILDCARD:
            return match_wildcard(pattern, str);
            
        case PATTERN_TYPE_MULTI: {
            if (pattern->multi.type == PATTERN_COMBINE_AND) {
                for (size_t i = 0; i < pattern->multi.count; i++) {
                    if (!pattern_match(pattern->multi.patterns[i], str))
                        return false;
                }
                return true;
            } else { // OR
                for (size_t i = 0; i < pattern->multi.count; i++) {
                    if (pattern_match(pattern->multi.patterns[i], str))
                        return true;
                }
                return false;
            }
        }
            
        case PATTERN_TYPE_ALTERNATION:
            return match_alternation(pattern, str);
    }
    
    return false;
}

static bool match_wildcard(const struct Pattern *pattern, const char *str) {
    const char *s = str;
    size_t i = 0;
    
    while (i < pattern->wildcard.segment_count) {
        if (pattern->wildcard.segments[i].is_wildcard) {
            // Last wildcard matches everything
            if (i == pattern->wildcard.segment_count - 1) return true;
            
            // Try to match the next non-wildcard segment
            i++;
            const char *next = s;
            while (*next) {
                if (strncmp(next, pattern->wildcard.segments[i].str, pattern->wildcard.segments[i].len) == 0) {
                    s = next + pattern->wildcard.segments[i].len;
                    i++;
                    break;
                }
                next++;
            }
            if (!*next) return false;
        } else {
            if (strncmp(s, pattern->wildcard.segments[i].str, pattern->wildcard.segments[i].len) != 0) {
                return false;
            }
            s += pattern->wildcard.segments[i].len;
            i++;
        }
    }
    
    return *s == '\0' || pattern->wildcard.segments[pattern->wildcard.segment_count - 1].is_wildcard;
}

struct Pattern *pattern_compile(const char *pattern, pattern_type_t type, bool case_sensitive) {
    if (!pattern || strlen(pattern) > PATTERN_MAX_LENGTH) {
        error_log("Invalid pattern");
        return NULL;
    }
    
    // Special handling for wildcard and alternation patterns
    if (type == PATTERN_TYPE_WILDCARD) {
        return compile_wildcard(pattern, case_sensitive);
    }
    if (type == PATTERN_TYPE_ALTERNATION) {
        return pattern_compile_alternation(pattern, case_sensitive);
    }
    
    struct Pattern *p = calloc(1, sizeof(struct Pattern));
    if (!p) {
        error_log("Memory allocation failed");
        return NULL;
    }
    
    p->type = type;
    p->case_sensitive = case_sensitive;
    
    // Initialize based on type
    switch (type) {
        case PATTERN_TYPE_PREFIX:
        case PATTERN_TYPE_SUFFIX:
        case PATTERN_TYPE_CONTAINS:
        case PATTERN_TYPE_EXACT:
            p->str.str = strdup(pattern);
            p->str.len = strlen(pattern);
            break;
            
        case PATTERN_TYPE_REGEX: {
            int flags = REG_EXTENDED | REG_NOSUB;
            if (!case_sensitive) flags |= REG_ICASE;
            
            if (regcomp(&p->str.regex, pattern, flags) != 0) {
                error_log("Invalid regular expression");
                pattern_free(p);
                return NULL;
            }
            p->str.has_regex = true;
            break;
        }
            
        default:
            error_log("Invalid pattern type");
            pattern_free(p);
            return NULL;
    }
    
    return p;
}

struct Pattern *pattern_compile_multi(const char **patterns, size_t count,
                                    pattern_combine_t combine_type, bool case_sensitive) {
    if (!patterns || count == 0 || count > PATTERN_MAX_MULTI) {
        error_log("Invalid multi-pattern parameters");
        return NULL;
    }
    
    struct Pattern *p = calloc(1, sizeof(struct Pattern));
    if (!p) {
        error_log("Memory allocation failed");
        return NULL;
    }
    
    p->type = PATTERN_TYPE_MULTI;
    p->case_sensitive = case_sensitive;
    p->multi.type = combine_type;
    
    p->multi.patterns = calloc(count, sizeof(struct Pattern *));
    if (!p->multi.patterns) {
        error_log("Memory allocation failed");
        pattern_free(p);
        return NULL;
    }
    
    for (size_t i = 0; i < count; i++) {
        p->multi.patterns[i] = pattern_compile(patterns[i], PATTERN_TYPE_EXACT, case_sensitive);
        if (!p->multi.patterns[i]) {
            pattern_free(p);
            return NULL;
        }
        p->multi.count++;
    }
    
    return p;
}

static struct Pattern *compile_wildcard(const char *pattern, bool case_sensitive) {
    struct Pattern *p = calloc(1, sizeof(struct Pattern));
    if (!p) {
        error_log("Memory allocation failed");
        return NULL;
    }
    
    p->type = PATTERN_TYPE_WILDCARD;
    p->case_sensitive = case_sensitive;
    
    // Count segments
    size_t count = 1;
    const char *s = pattern;
    while (*s) {
        if (*s == '*') count++;
        s++;
    }
    
    // Allocate segments
    p->wildcard.segments = calloc(count, sizeof(pattern_segment_t));
    if (!p->wildcard.segments) {
        error_log("Memory allocation failed");
        pattern_free(p);
        return NULL;
    }
    
    // Parse segments
    size_t idx = 0;
    const char *start = pattern;
    s = pattern;
    
    while (*s) {
        if (*s == '*') {
            if (s > start) {
                size_t len = s - start;
                p->wildcard.segments[idx].str = strndup(start, len);
                p->wildcard.segments[idx].len = len;
                p->wildcard.segments[idx].is_wildcard = false;
                idx++;
            }
            p->wildcard.segments[idx].is_wildcard = true;
            idx++;
            start = s + 1;
        }
        s++;
    }
    
    // Add final segment if any
    if (*start) {
        size_t len = s - start;
        p->wildcard.segments[idx].str = strndup(start, len);
        p->wildcard.segments[idx].len = len;
        p->wildcard.segments[idx].is_wildcard = false;
        idx++;
    }
    
    p->wildcard.segment_count = idx;
    return p;
}

struct Pattern *pattern_compile_alternation(const char *pattern, bool case_sensitive) {
    if (!pattern) return NULL;
    
    struct Pattern *p = calloc(1, sizeof(struct Pattern));
    if (!p) return NULL;
    
    p->type = PATTERN_TYPE_ALTERNATION;
    p->case_sensitive = case_sensitive;
    
    // Count character classes
    size_t class_count = 0;
    const char *ptr = pattern;
    while (*ptr) {
        if (*ptr == '[') {
            class_count++;
            ptr = strchr(ptr, ']');
            if (!ptr) {
                pattern_free(p);
                return NULL;
            }
        }
        ptr++;
    }
    
    if (class_count == 0) {
        pattern_free(p);
        return NULL;
    }
    
    // Allocate character classes
    p->alt.classes = calloc(class_count, sizeof(pattern_charclass_t));
    if (!p->alt.classes) {
        pattern_free(p);
        return NULL;
    }
    p->alt.count = class_count;
    
    // Parse character classes
    ptr = pattern;
    size_t class_idx = 0;
    while (*ptr && class_idx < class_count) {
        if (*ptr == '[') {
            ptr++;
            size_t char_idx = 0;
            while (*ptr && *ptr != ']' && char_idx < PATTERN_MAX_CHARCLASS) {
                p->alt.classes[class_idx].chars[char_idx++] = *ptr++;
            }
            if (*ptr != ']') {
                pattern_free(p);
                return NULL;
            }
            p->alt.classes[class_idx].count = char_idx;
            class_idx++;
        }
        ptr++;
    }
    
    p->probability = calc_alternation_probability(p);
    return p;
}

static bool match_alternation(const struct Pattern *pattern, const char *str) {
    const char *s = str;
    
    // Match each position against its character class
    for (size_t i = 0; i < pattern->alt.count; i++) {
        if (!*s) return false;
        
        bool found = false;
        for (size_t j = 0; j < pattern->alt.classes[i].count; j++) {
            char c = pattern->case_sensitive ? *s :
                tolower(*s);
            char p = pattern->case_sensitive ? pattern->alt.classes[i].chars[j] :
                tolower(pattern->alt.classes[i].chars[j]);
            
            if (c == p) {
                found = true;
                break;
            }
        }
        
        if (!found) return false;
        s++;
    }
    
    return *s == '\0';
}

static double calc_wildcard_probability(const struct Pattern *pattern) {
    // Count fixed characters
    size_t fixed_chars = 0;
    for (size_t i = 0; i < pattern->wildcard.segment_count; i++) {
        if (!pattern->wildcard.segments[i].is_wildcard) {
            fixed_chars += pattern->wildcard.segments[i].len;
        }
    }
    
    // Base probability is 1/58^fixed_chars for fixed characters
    double prob = pow(1.0/base58_len, fixed_chars);
    
    // Adjust for wildcards - each wildcard roughly halves probability
    for (size_t i = 0; i < pattern->wildcard.segment_count; i++) {
        if (pattern->wildcard.segments[i].is_wildcard) {
            prob *= 0.5;
        }
    }
    
    return prob;
}

static double calc_alternation_probability(const struct Pattern *pattern) {
    double prob = 1.0;
    
    for (size_t i = 0; i < pattern->alt.count; i++) {
        prob *= (double)pattern->alt.classes[i].count / base58_len;
    }
    
    return prob;
}

double pattern_probability(const struct Pattern *pattern) {
    if (!pattern) return 0.0;
    
    switch (pattern->type) {
        case PATTERN_TYPE_PREFIX:
            return pow(1.0/base58_len, pattern->str.len);
            
        case PATTERN_TYPE_SUFFIX:
            return pow(1.0/base58_len, pattern->str.len);
            
        case PATTERN_TYPE_CONTAINS:
            return pow(1.0/base58_len, pattern->str.len) * 0.1;
            
        case PATTERN_TYPE_EXACT:
            return pow(1.0/base58_len, pattern->str.len);
            
        case PATTERN_TYPE_REGEX:
            return 0.0; // Cannot estimate regex probability
            
        case PATTERN_TYPE_WILDCARD:
            return calc_wildcard_probability(pattern);
            
        case PATTERN_TYPE_MULTI:
            if (pattern->multi.type == PATTERN_COMBINE_AND) {
                double prob = 1.0;
                for (size_t i = 0; i < pattern->multi.count; i++) {
                    prob *= pattern_probability(pattern->multi.patterns[i]);
                }
                return prob;
            } else { // OR
                double prob = 0.0;
                for (size_t i = 0; i < pattern->multi.count; i++) {
                    prob += pattern_probability(pattern->multi.patterns[i]);
                }
                return prob;
            }
            
        case PATTERN_TYPE_ALTERNATION:
            return calc_alternation_probability(pattern);
    }
    
    return 0.0;
}

int pattern_describe(const struct Pattern *pattern, char *buf, size_t size) {
    if (!pattern || !buf || size == 0) return -1;
    
    switch (pattern->type) {
        case PATTERN_TYPE_PREFIX:
            snprintf(buf, size, "Prefix: %s", pattern->str.str);
            break;
            
        case PATTERN_TYPE_SUFFIX:
            snprintf(buf, size, "Suffix: %s", pattern->str.str);
            break;
            
        case PATTERN_TYPE_CONTAINS:
            snprintf(buf, size, "Contains: %s", pattern->str.str);
            break;
            
        case PATTERN_TYPE_EXACT:
            snprintf(buf, size, "Exact: %s", pattern->str.str);
            break;
            
        case PATTERN_TYPE_REGEX:
            snprintf(buf, size, "Regex pattern");
            break;
            
        case PATTERN_TYPE_WILDCARD:
            snprintf(buf, size, "Wildcard pattern");
            break;
            
        case PATTERN_TYPE_MULTI:
            snprintf(buf, size, "Multi-pattern (%s)",
                    pattern->multi.type == PATTERN_COMBINE_AND ? "AND" : "OR");
            break;
            
        case PATTERN_TYPE_ALTERNATION:
            snprintf(buf, size, "Alternation pattern");
            break;
            
        default:
            snprintf(buf, size, "Unknown pattern type");
            break;
    }
    
    return 0;
}

void pattern_free(struct Pattern *pattern) {
    if (pattern) {
        switch (pattern->type) {
            case PATTERN_TYPE_PREFIX:
            case PATTERN_TYPE_SUFFIX:
            case PATTERN_TYPE_CONTAINS:
            case PATTERN_TYPE_EXACT:
                free(pattern->str.str);
                break;
                
            case PATTERN_TYPE_REGEX:
                if (pattern->str.has_regex) {
                    regfree(&pattern->str.regex);
                }
                break;
                
            case PATTERN_TYPE_WILDCARD:
                if (pattern->wildcard.segments) {
                    for (size_t i = 0; i < pattern->wildcard.segment_count; i++) {
                        free(pattern->wildcard.segments[i].str);
                    }
                    free(pattern->wildcard.segments);
                }
                break;
                
            case PATTERN_TYPE_MULTI:
                if (pattern->multi.patterns) {
                    for (size_t i = 0; i < pattern->multi.count; i++) {
                        pattern_free(pattern->multi.patterns[i]);
                    }
                    free(pattern->multi.patterns);
                }
                break;
                
            case PATTERN_TYPE_ALTERNATION:
                free(pattern->alt.classes);
                break;
        }
        free(pattern);
    }
}
