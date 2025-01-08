/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "vanity.h"
#include "random.h"
#include "privkey.h"
#include "pubkey.h"
#include "address.h"
#include "pattern.h"
#include "error.h"

#define VANITY_MAX_PATTERN 16
#define VANITY_MAX_THREADS 64
#define VANITY_BATCH_SIZE 16

// Forward declaration
typedef struct ThreadContext ThreadContext;

struct ThreadContext {
    struct VanitySearch *search;
    int thread_id;
    bool running;
    pthread_t thread;
};

struct VanitySearch {
    struct Pattern pattern;           // Compiled pattern
    bool case_sensitive;        // Case sensitivity flag
    int num_threads;           // Number of threads to use
    volatile bool found;       // Whether a match was found
    volatile bool stopped;     // Whether search was stopped
    volatile uint64_t attempts; // Total attempts made
    struct timespec start_time; // Search start time
    pthread_mutex_t mutex;     // Thread synchronization
    PrivKey found_privkey;    // Found private key
    PubKey found_pubkey;      // Found public key
    char found_address[35];   // Found address
    // Progress tracking
    vanity_progress_cb progress_callback;
    void *progress_user_data;
    int progress_interval_ms;
    struct timeval last_progress;
    // Thread contexts
    ThreadContext contexts[VANITY_MAX_THREADS];
};

// Thread-local key pool
struct KeyPool {
    PrivKey privkeys[VANITY_BATCH_SIZE];
    PubKey pubkeys[VANITY_BATCH_SIZE];
    char addresses[VANITY_BATCH_SIZE][35];
    bool initialized;
};

static void init_key_pool(struct KeyPool *pool) {
    if (pool->initialized) return;
    
    for (int i = 0; i < VANITY_BATCH_SIZE; i++) {
        if (privkey_new(&pool->privkeys[i]) < 0 ||
            pubkey_new(&pool->pubkeys[i]) < 0) {
            error_log("Failed to initialize key pool");
            return;
        }
    }
    pool->initialized = true;
}

static void cleanup_key_pool(struct KeyPool *pool) {
    if (!pool->initialized) return;
    
    for (int i = 0; i < VANITY_BATCH_SIZE; i++) {
        privkey_free(pool->privkeys[i]);
        pubkey_free(pool->pubkeys[i]);
    }
    pool->initialized = false;
}

static void *search_thread(void *arg) {
    ThreadContext *ctx = (ThreadContext *)arg;
    VanitySearch *search = ctx->search;
    struct timeval now;
    struct KeyPool pool = {0};
    unsigned char key_data[32 * VANITY_BATCH_SIZE];
    
    ctx->running = true;
    printf("Thread %d started\n", ctx->thread_id);
    
    // Initialize key pool
    init_key_pool(&pool);
    if (!pool.initialized) {
        printf("Thread %d failed to initialize key pool\n", ctx->thread_id);
        goto cleanup;
    }
    
    printf("Thread %d initialized key pool\n", ctx->thread_id);
    
    while (!search->found && !search->stopped) {
        // Generate batch of random private keys
        if (random_get(key_data, sizeof(key_data)) != 0) {
            printf("Thread %d failed to generate random keys\n", ctx->thread_id);
            goto cleanup;
        }
        
        // Process batch
        for (int i = 0; i < VANITY_BATCH_SIZE && !search->found && !search->stopped; i++) {
            // Import private key
            if (privkey_import(pool.privkeys[i], key_data + (i * 32), 32) != 0) {
                printf("Thread %d failed to import private key\n", ctx->thread_id);
                continue;
            }
            pool.privkeys[i]->cflag = PRIVKEY_COMPRESSED_FLAG;
            
            // Get public key
            if (pubkey_get(pool.pubkeys[i], pool.privkeys[i]) != 0) {
                printf("Thread %d failed to get public key\n", ctx->thread_id);
                continue;
            }
            
            // Get address
            if (address_get_p2pkh(pool.addresses[i], pool.pubkeys[i]) != 0) {
                printf("Thread %d failed to get address\n", ctx->thread_id);
                continue;
            }
            
            // Update attempt counter atomically
            __atomic_add_fetch(&search->attempts, 1, __ATOMIC_SEQ_CST);
            
            // Check if address matches pattern
            if (pattern_match(&search->pattern, pool.addresses[i] + 1)) { // Skip version byte
                pthread_mutex_lock(&search->mutex);
                if (!search->found) {
                    search->found = true;
                    memcpy(search->found_privkey->data, pool.privkeys[i]->data, PRIVKEY_LENGTH);
                    search->found_privkey->cflag = pool.privkeys[i]->cflag;
                    memcpy(search->found_pubkey, pool.pubkeys[i], sizeof(*pool.pubkeys[i]));
                    strcpy(search->found_address, pool.addresses[i]);
                }
                pthread_mutex_unlock(&search->mutex);
                break;
            }
        }
        
        // Handle progress callback
        if (search->progress_callback) {
            gettimeofday(&now, NULL);
            long elapsed_ms = (now.tv_sec - search->last_progress.tv_sec) * 1000 + 
                            (now.tv_usec - search->last_progress.tv_usec) / 1000;
                            
            if (elapsed_ms >= search->progress_interval_ms) {
                pthread_mutex_lock(&search->mutex);
                uint64_t current_attempts = search->attempts;
                double rate = current_attempts * 1000.0 / elapsed_ms;
                search->progress_callback(current_attempts, rate, search->progress_user_data);
                search->last_progress = now;
                pthread_mutex_unlock(&search->mutex);
            }
        }
    }
    
cleanup:
    printf("Thread %d cleaning up\n", ctx->thread_id);
    cleanup_key_pool(&pool);
    ctx->running = false;
    return NULL;
}

int vanity_init(VanitySearch **search, const char *pattern, bool case_sensitive, int num_threads) {
    if (!search || !pattern) return -1;
    
    // Validate pattern length
    if (strlen(pattern) > VANITY_MAX_PATTERN) {
        error_log("Pattern too long (max %d characters)", VANITY_MAX_PATTERN);
        return -1;
    }
    
    // Validate thread count
    if (num_threads < 1 || num_threads > VANITY_MAX_THREADS) {
        error_log("Invalid thread count (must be between 1 and %d)", VANITY_MAX_THREADS);
        return -1;
    }
    
    // Allocate search context
    VanitySearch *s = calloc(1, sizeof(VanitySearch));
    if (!s) {
        error_log("Could not allocate search context");
        return -1;
    }
    
    // Initialize pattern
    struct Pattern *compiled_pattern = pattern_compile(pattern, PATTERN_TYPE_PREFIX, case_sensitive);
    if (!compiled_pattern) {
        error_log("Could not compile pattern");
        free(s);
        return -1;
    }
    memcpy(&s->pattern, compiled_pattern, sizeof(struct Pattern));
    pattern_free(compiled_pattern);
    
    // Initialize other fields
    s->case_sensitive = case_sensitive;
    s->num_threads = num_threads;
    s->found = false;
    s->stopped = false;
    s->attempts = 0;
    
    // Initialize mutex
    if (pthread_mutex_init(&s->mutex, NULL) != 0) {
        error_log("Could not initialize mutex");
        pattern_free(&s->pattern);
        free(s);
        return -1;
    }
    
    // Initialize private key for storing result
    if (privkey_new(&s->found_privkey) < 0) {
        error_log("Failed to initialize result private key");
        pattern_free(&s->pattern);
        pthread_mutex_destroy(&s->mutex);
        free(s);
        return -1;
    }
    
    *search = s;
    return 0;
}

int vanity_start(VanitySearch *search) {
    if (!search) {
        error_log("Invalid search context");
        return -1;
    }
    
    // Record start time
    clock_gettime(CLOCK_MONOTONIC, &search->start_time);
    gettimeofday(&search->last_progress, NULL);
    
    // Start worker threads
    for (int i = 0; i < search->num_threads; i++) {
        search->contexts[i].search = search;
        search->contexts[i].thread_id = i;
        search->contexts[i].running = false;
        
        if (pthread_create(&search->contexts[i].thread, NULL, search_thread, &search->contexts[i]) != 0) {
            error_log("Failed to create worker thread");
            vanity_stop(search);
            return -1;
        }
    }
    
    return 0;
}

void vanity_stop(VanitySearch *search) {
    if (!search) return;
    
    // Signal threads to stop
    search->stopped = true;
    
    // Wait for threads to finish
    for (int i = 0; i < search->num_threads; i++) {
        if (search->contexts[i].running) {
            pthread_join(search->contexts[i].thread, NULL);
        }
    }
}

bool vanity_found(VanitySearch *search) {
    return search ? search->found : false;
}

bool vanity_is_stopped(VanitySearch *search) {
    return search ? search->stopped : true;
}

uint64_t vanity_get_attempts(VanitySearch *search) {
    return search ? search->attempts : 0;
}

uint64_t vanity_get_elapsed(VanitySearch *search) {
    if (!search) return 0;
    
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    return (now.tv_sec - search->start_time.tv_sec) * 1000 +
           (now.tv_nsec - search->start_time.tv_nsec) / 1000000;
}

int vanity_get_wif(VanitySearch *search, char *wif, size_t wif_size) {
    if (!search || !wif || wif_size < PRIVKEY_WIF_LENGTH_MIN || !search->found) {
        error_log("Invalid parameters for WIF export");
        return -1;
    }
    
    return privkey_to_wif(wif, search->found_privkey);
}

int vanity_get_address(VanitySearch *search, char *address, size_t address_size) {
    if (!search || !address || address_size < 35 || !search->found) {
        error_log("Invalid parameters for address export");
        return -1;
    }
    
    strcpy(address, search->found_address);
    return 0;
}

int vanity_set_progress_callback(VanitySearch *search, vanity_progress_cb callback,
                               void *user_data, int interval_ms) {
    if (!search || !callback || interval_ms < 0) {
        error_log("Invalid progress callback parameters");
        return -1;
    }
    
    search->progress_callback = callback;
    search->progress_user_data = user_data;
    search->progress_interval_ms = interval_ms;
    return 0;
}

void vanity_cleanup(VanitySearch *search) {
    if (!search) return;
    
    vanity_stop(search);
    pattern_free(&search->pattern);
    privkey_free(search->found_privkey);
    pthread_mutex_destroy(&search->mutex);
    free(search);
}