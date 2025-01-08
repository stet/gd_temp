/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <stdatomic.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>

#include "btk_vanity.h"
#include "mods/debug.h"
#include "mods/gd_vanity.h"
#include "mods/output.h"
#include "mods/input.h"
#include "mods/opts.h"

// ANSI color codes
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

// Unicode emojis
#define EMOJI_PICKAXE    "⛏️ "
#define EMOJI_SPARKLES   "✨ "
#define EMOJI_ROCKET     "🚀 "
#define EMOJI_CHECK      "✅ "
#define EMOJI_BITCOIN    "₿ "
#define EMOJI_KEY        "🔑 "

// Forward declarations
static void progress_callback(const vanity_stats_t *stats);

// Main function for vanity address generation
int btk_vanity_main(output_item *output, opts_p opts, unsigned char *input, size_t input_len)
{
    (void)input;
    (void)input_len;
    
    // Initialize output if needed
    if (!*output) {
        *output = output_new(NULL, 0);
        if (!*output) {
            fprintf(stderr, "%sError: Failed to create output item%s\n", ANSI_RED, ANSI_RESET);
            return -1;
        }
    }
    
    // Get number of threads from options (default to 4)
    uint32_t num_threads = opts->threads > 0 ? opts->threads : 4;
    
    // Get case sensitivity from options (default to case sensitive)
    bool case_sensitive = !opts->case_insensitive;
    
    // Get pattern from input
    if (opts->input_count < 1) {
        output_printf(*output, "%sError: Pattern is required%s\n", ANSI_RED, ANSI_RESET);
        return -1;
    }
    const char *pattern = opts->input[0];
    
    // Initialize debug module
    debug_init(DEBUG_TRACE);
    debug_info("Starting vanity address search for pattern '%s'", pattern);
    
    // Initialize vanity search module
    if (gd_vanity_init(num_threads) < 0) {
        output_printf(*output, "%sError: Failed to initialize vanity search module%s\n", ANSI_RED, ANSI_RESET);
        return -1;
    }
    
    // Set progress callback
    gd_vanity_set_progress_callback(progress_callback);
    
    // Start search
    if (gd_vanity_start(pattern, case_sensitive) < 0) {
        output_printf(*output, "%sError: Failed to start vanity search%s\n", ANSI_RED, ANSI_RESET);
        gd_vanity_cleanup();
        return -1;
    }
    
    // Print search info
    output_printf(*output, "%sStarting vanity address search...%s\n", ANSI_BOLD, ANSI_RESET);
    output_printf(*output, "Pattern: %s%s%s\n", ANSI_BOLD, pattern, ANSI_RESET);
    output_printf(*output, "Case %ssensitive%s\n", case_sensitive ? "" : "in", ANSI_RESET);
    output_printf(*output, "Using %u thread%s\n\n", num_threads, num_threads > 1 ? "s" : "");
    
    // Wait for result or termination
    char wif[53] = {0};
    char address[35] = {0};
    bool found = false;
    bool interrupted = false;
    
    // Set up signal handler
    struct sigaction sa = {0};
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
    
    while (!found && !interrupted) {
        if (gd_vanity_get_result(wif, address)) {
            found = true;
            break;
        }
        
        // Check if search was interrupted
        if (access("/tmp/vanity_stop", F_OK) == 0) {
            interrupted = true;
            break;
        }
        
        usleep(100000);  // Sleep for 100ms
    }
    
    // Stop search and clean up
    gd_vanity_stop();
    gd_vanity_cleanup();
    
    // Remove stop file if it exists
    unlink("/tmp/vanity_stop");
    
    if (found) {
        // Output result
        output_printf(*output, "\n%sFound matching address!%s\n", ANSI_BOLD, ANSI_RESET);
        output_printf(*output, "Private key (WIF): %s%s%s\n", ANSI_BOLD, wif, ANSI_RESET);
        output_printf(*output, "Address: %s%s%s\n", ANSI_BOLD, address, ANSI_RESET);
        return 0;
    } else if (interrupted) {
        output_printf(*output, "\n%sSearch interrupted by user%s\n", ANSI_YELLOW, ANSI_RESET);
        return 1;
    } else {
        output_printf(*output, "%sSearch terminated without finding a match%s\n", ANSI_RED, ANSI_RESET);
        return -1;
    }
}

// Progress callback function
static void progress_callback(const vanity_stats_t *stats)
{
    if (!stats) {
        return;
    }
    
    // Calculate rate
    double rate = stats->attempts / stats->elapsed_time;
    
    // Format progress message
    char msg[256];
    snprintf(msg, sizeof(msg), "%sSearching...%s %zu attempts (%.2fK/s)", 
             ANSI_BOLD, ANSI_RESET, stats->attempts, rate / 1000.0);
    
    // Print progress
    printf("\r%s", msg);
    fflush(stdout);
}

// Help function
int btk_vanity_help(output_item *output)
{
    // Initialize output if needed
    if (!*output) {
        *output = output_new(NULL, 0);
        if (!*output) {
            fprintf(stderr, "%sError: Failed to create output item%s\n", ANSI_RED, ANSI_RESET);
            return -1;
        }
    }
    
    output_printf(*output, "%s%s vanity - Generate a Bitcoin vanity address%s\n\n", ANSI_BOLD, EMOJI_BITCOIN, ANSI_RESET);
    output_printf(*output, "Usage: %sbtk vanity [options] <pattern>%s\n\n", ANSI_BOLD, ANSI_RESET);
    output_printf(*output, "Options:\n");
    output_printf(*output, "  -t, --threads <n>       Number of threads to use (default: 4)\n");
    output_printf(*output, "  -i, --case-insensitive  Case insensitive pattern matching\n");
    output_printf(*output, "\n");
    output_printf(*output, "Example:\n");
    output_printf(*output, "  btk vanity 1abc        Generate address starting with '1abc'\n");
    output_printf(*output, "  btk vanity -i 1ABC     Generate address starting with '1abc' (case insensitive)\n");
    output_printf(*output, "\n");
    return 0;
}