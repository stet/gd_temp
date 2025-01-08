/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "../mods/vanity.h"
#include "../mods/pattern.h"
#include "../mods/error.h"
#include "../mods/benchmark.h"

static VanitySearch *g_ctx = NULL;

static void handle_signal(int signo) {
    (void)signo;
    if (g_ctx) {
        vanity_stop(g_ctx);
    }
}

static void print_usage(void) {
    printf("Usage: btk vanity [options] <pattern>\n");
    printf("\n");
    printf("Options:\n");
    printf("  -i        Case insensitive search (default: case sensitive)\n");
    printf("  -t <num>  Number of threads (default: number of CPU cores)\n");
    printf("  -p <type> Pattern type:\n");
    printf("           prefix    - Match at start (default)\n");
    printf("           suffix    - Match at end\n");
    printf("           contains  - Match anywhere\n");
    printf("           exact     - Exact match\n");
    printf("           regex     - Regular expression\n");
    printf("           wildcard  - Wildcard pattern (use * for wildcards)\n");
    printf("           alt       - Alternation pattern (e.g., 1[AB][12])\n");
    printf("  -m <op>   Multi-pattern operator (for multiple patterns):\n");
    printf("           and - All patterns must match\n");
    printf("           or  - Any pattern must match\n");
    printf("  -b        Run benchmark before starting\n");
    printf("\n");
    printf("Examples:\n");
    printf("  btk vanity -p prefix 1ABC      # Address starting with 1ABC\n");
    printf("  btk vanity -p suffix XYZ       # Address ending with XYZ\n");
    printf("  btk vanity -p wildcard 1*COOL*Z # Address with wildcards\n");
    printf("  btk vanity -p alt 1[AB][12]    # Address matching alternation\n");
    printf("  btk vanity -m and ABC XYZ      # Address containing both ABC and XYZ\n");
    printf("\n");
}

static void progress_callback(uint64_t attempts, double rate, void *user_data) {
    (void)user_data;
    printf("\rAttempts: %" PRIu64 " (%.2f/s)", attempts, rate);
    fflush(stdout);
}

int gd_handle_vanity(int argc, char *argv[]) {
    int r, opt;
    bool case_sensitive = true;
    bool run_benchmark = false;
    int threads = sysconf(_SC_NPROCESSORS_ONLN);
    pattern_type_t pattern_type = PATTERN_TYPE_PREFIX;
    pattern_combine_t combine_type = PATTERN_COMBINE_AND;
    VanitySearch *ctx = NULL;
    struct Pattern *pattern = NULL;
    
    // Parse command line options
    while ((opt = getopt(argc, argv, "it:p:m:b")) != -1) {
        switch (opt) {
            case 'i':
                case_sensitive = false;
                break;
                
            case 't':
                threads = atoi(optarg);
                if (threads <= 0) {
                    error_log("Invalid number of threads");
                    return -1;
                }
                break;
                
            case 'p':
                if (strcmp(optarg, "prefix") == 0) {
                    pattern_type = PATTERN_TYPE_PREFIX;
                } else if (strcmp(optarg, "suffix") == 0) {
                    pattern_type = PATTERN_TYPE_SUFFIX;
                } else if (strcmp(optarg, "contains") == 0) {
                    pattern_type = PATTERN_TYPE_CONTAINS;
                } else if (strcmp(optarg, "exact") == 0) {
                    pattern_type = PATTERN_TYPE_EXACT;
                } else if (strcmp(optarg, "regex") == 0) {
                    pattern_type = PATTERN_TYPE_REGEX;
                } else if (strcmp(optarg, "wildcard") == 0) {
                    pattern_type = PATTERN_TYPE_WILDCARD;
                } else if (strcmp(optarg, "alt") == 0) {
                    pattern_type = PATTERN_TYPE_ALTERNATION;
                } else {
                    error_log("Invalid pattern type");
                    print_usage();
                    return -1;
                }
                break;
                
            case 'm':
                if (strcmp(optarg, "and") == 0) {
                    combine_type = PATTERN_COMBINE_AND;
                    pattern_type = PATTERN_TYPE_MULTI;
                } else if (strcmp(optarg, "or") == 0) {
                    combine_type = PATTERN_COMBINE_OR;
                    pattern_type = PATTERN_TYPE_MULTI;
                } else {
                    error_log("Invalid multi-pattern operator");
                    print_usage();
                    return -1;
                }
                break;
                
            case 'b':
                run_benchmark = true;
                break;
                
            default:
                print_usage();
                return -1;
        }
    }
    
    // Check for pattern arguments
    if (optind >= argc) {
        error_log("Missing pattern argument");
        print_usage();
        return -1;
    }
    
    // Compile pattern based on type
    if (pattern_type == PATTERN_TYPE_MULTI) {
        // Multiple patterns
        const char **patterns = (const char **)&argv[optind];
        size_t count = argc - optind;
        pattern = pattern_compile_multi(patterns, count, combine_type, case_sensitive);
    } else if (pattern_type == PATTERN_TYPE_ALTERNATION) {
        pattern = pattern_compile_alternation(argv[optind], case_sensitive);
    } else {
        pattern = pattern_compile(argv[optind], pattern_type, case_sensitive);
    }
    
    if (!pattern) {
        error_log("Failed to compile pattern");
        return -1;
    }
    
    // Print pattern description
    char desc[256];
    pattern_describe(pattern, desc, sizeof(desc));
    printf("%s\n\n", desc);
    
    // Run benchmark if requested
    if (run_benchmark) {
        benchmark_result_t result;
        printf("Running benchmark...\n");
        
        if (benchmark_run(pattern, 5, threads, &result) == 0) {
            benchmark_print_results(&result, pattern);
            
            // Ask user if they want to continue
            printf("Continue with search? [Y/n] ");
            char response = getchar();
            if (response == 'n' || response == 'N') {
                pattern_free(pattern);
                return 0;
            }
        }
    }
    
    // Initialize vanity search
    r = vanity_init(&ctx, argv[optind], case_sensitive, threads);
    if (r != 0) {
        error_log("Failed to initialize vanity search");
        pattern_free(pattern);
        return -1;
    }
    
    // Set up signal handler
    g_ctx = ctx;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        error_log("Failed to set up signal handler");
        pattern_free(pattern);
        vanity_cleanup(ctx);
        return -1;
    }
    
    // Set up progress callback
    vanity_set_progress_callback(ctx, progress_callback, NULL, 100);
    
    // Start search
    printf("Starting search...\n");
    printf("Press Ctrl+C to stop\n\n");
    
    r = vanity_start(ctx);
    if (r != 0) {
        error_log("Failed to start vanity search");
        pattern_free(pattern);
        vanity_cleanup(ctx);
        return -1;
    }
    
    // Wait for result
    while (!vanity_found(ctx) && !vanity_is_stopped(ctx)) {
        usleep(100000); // 100ms
    }
    printf("\n");
    
    // Get results
    if (vanity_found(ctx)) {
        char wif[100];
        char address[100];
        
        r = vanity_get_wif(ctx, wif, sizeof(wif));
        if (r != 0) {
            error_log("Failed to get WIF");
            pattern_free(pattern);
            vanity_cleanup(ctx);
            return -1;
        }
        
        r = vanity_get_address(ctx, address, sizeof(address));
        if (r != 0) {
            error_log("Failed to get address");
            pattern_free(pattern);
            vanity_cleanup(ctx);
            return -1;
        }
        
        printf("Found!\n");
        printf("Private key (WIF): %s\n", wif);
        printf("Address: %s\n", address);
    }
    
    pattern_free(pattern);
    vanity_cleanup(ctx);
    return 0;
}