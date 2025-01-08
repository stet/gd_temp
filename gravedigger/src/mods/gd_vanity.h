#ifndef GD_VANITY_H
#define GD_VANITY_H

#include <stdbool.h>
#include <stddef.h>

// Statistics structure
typedef struct {
    size_t attempts;
    double elapsed_time;
} vanity_stats_t;

// Progress callback type
typedef void (*progress_callback_t)(const vanity_stats_t *stats);

// Function declarations
void gd_vanity_set_progress_callback(progress_callback_t callback);
int gd_vanity_init(uint32_t thread_count);
void gd_vanity_cleanup(void);
int gd_vanity_start(const char *pattern, bool case_sensitive);
void gd_vanity_stop(void);
bool gd_vanity_get_result(char *privkey_wif, char *address);

#endif // GD_VANITY_H
