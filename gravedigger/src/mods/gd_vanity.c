#define _POSIX_C_SOURCE 199309L
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
#include <strings.h>

#include "gd_vanity.h"
#include "debug.h"
#include "pubkey.h"
#include "privkey.h"
#include "address.h"
#include "random.h"
#include "base58check.h"

// Forward declarations
static void *thread_worker(void *arg);
static void handle_signal(int sig);

#define MAX_PATTERN_LENGTH 128
#define PROGRESS_INTERVAL 10000

// Thread context structure
typedef struct {
    unsigned int thread_num;
    atomic_bool should_exit;
} thread_context_t;

// Global variables
static bool initialized = false;
static uint32_t thread_count = 0;
static pthread_t *threads = NULL;
static thread_context_t *thread_contexts = NULL;
static progress_callback_t progress_callback = NULL;
static bool case_sensitive_match = true;
static char pattern_buf[MAX_PATTERN_LENGTH];
static size_t pattern_len = 0;
static struct timespec start_time;

// Result storage
static char result_wif[53];
static char result_address[35];

// Atomic flags
static atomic_bool found = false;
static atomic_bool stopped = false;
static atomic_size_t total_attempts = 0;

// Mutexes
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Set progress callback
void gd_vanity_set_progress_callback(progress_callback_t callback) {
    progress_callback = callback;
}

// Pattern matching function
static bool pattern_match(const char *address) {
    if (!address || !pattern_buf[0]) {
        return false;
    }
    
    // Skip version byte
    address++;
    
    // Case sensitive match
    if (case_sensitive_match) {
        return strstr(address, pattern_buf) != NULL;
    }
    
    // Case insensitive match
    char addr_lower[35];
    char pattern_lower[MAX_PATTERN_LENGTH];
    
    strncpy(addr_lower, address, sizeof(addr_lower) - 1);
    addr_lower[sizeof(addr_lower) - 1] = '\0';
    
    strncpy(pattern_lower, pattern_buf, sizeof(pattern_lower) - 1);
    pattern_lower[sizeof(pattern_lower) - 1] = '\0';
    
    for (char *p = addr_lower; *p; p++) {
        *p = tolower(*p);
    }
    for (char *p = pattern_lower; *p; p++) {
        *p = tolower(*p);
    }
    
    return strstr(addr_lower, pattern_lower) != NULL;
}

// Thread worker function
static void *thread_worker(void *arg) {
    printf("Thread worker started\n");
    thread_context_t *ctx = (thread_context_t *)arg;
    size_t local_attempts = 0;
    struct timespec last_progress = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &last_progress);
    
    // Allocate key structures
    PrivKey privkey = malloc(sizeof(struct PrivKey));
    if (!privkey) {
        debug_error("Failed to allocate private key");
        return NULL;
    }
    
    PubKey pubkey = malloc(pubkey_sizeof());
    if (!pubkey) {
        debug_error("Failed to allocate public key");
        free(privkey);
        return NULL;
    }
    
    char address[35];
    char wif[53];
    
    printf("Thread %u started\n", ctx->thread_num);
    
    // Main search loop
    while (!atomic_load(&ctx->should_exit) && !atomic_load(&found)) {
        printf("Generating random private key\n");
        // Generate random private key
        if (privkey_new(privkey) < 0) {
            debug_error("Failed to generate private key");
            continue;
        }
        
        printf("Getting public key\n");
        // Get public key
        if (pubkey_get(pubkey, privkey) < 0) {
            debug_error("Failed to get public key");
            continue;
        }
        
        printf("Getting address\n");
        // Get address
        if (address_get_p2pkh(address, pubkey) < 0) {
            debug_error("Failed to get address");
            continue;
        }
        
        printf("Checking for pattern match\n");
        // Check for pattern match
        if (pattern_match(address)) {
            printf("Pattern match found\n");
            // Get WIF format
            if (privkey_to_wif(wif, privkey) < 0) {
                debug_error("Failed to get WIF");
                continue;
            }
            
            // Save result
            pthread_mutex_lock(&result_mutex);
            if (!atomic_load(&found)) {
                strncpy(result_wif, wif, sizeof(result_wif) - 1);
                result_wif[sizeof(result_wif) - 1] = '\0';
                strncpy(result_address, address, sizeof(result_address) - 1);
                result_address[sizeof(result_address) - 1] = '\0';
                atomic_store(&found, true);
                debug_info("Thread %u found match: %s", ctx->thread_num, address);
            }
            pthread_mutex_unlock(&result_mutex);
            break;
        }
        
        // Update counters
        local_attempts++;
        atomic_fetch_add(&total_attempts, 1);
        
        // Update progress
        if (progress_callback && local_attempts % PROGRESS_INTERVAL == 0) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            
            vanity_stats_t stats = {
                .attempts = atomic_load(&total_attempts),
                .elapsed_time = (now.tv_sec - start_time.tv_sec) + 
                               (now.tv_nsec - start_time.tv_nsec) / 1e9
            };
            
            progress_callback(&stats);
        }
    }
    
    // Cleanup
    free(privkey);
    free(pubkey);
    
    printf("Thread %u finished\n", ctx->thread_num);
    return NULL;
}

// Signal handler
static void handle_signal(int sig) {
    (void)sig;
    atomic_store(&stopped, true);
    for (uint32_t i = 0; i < thread_count; i++) {
        atomic_store(&thread_contexts[i].should_exit, true);
    }
}

// Initialize vanity search module
int gd_vanity_init(uint32_t num_threads) {
    printf("[INFO] Initializing vanity search module with %u threads\n", num_threads);
    pthread_mutex_lock(&init_mutex);
    
    if (initialized) {
        printf("[WARN] Module already initialized\n");
        pthread_mutex_unlock(&init_mutex);
        return 0;
    }
    
    // Set thread count
    thread_count = num_threads > 0 ? num_threads : 4;
    printf("[INFO] Thread count set to %u\n", thread_count);
    
    // Allocate thread arrays
    threads = calloc(thread_count, sizeof(pthread_t));
    if (!threads) {
        printf("[ERROR] Failed to allocate thread array\n");
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    
    thread_contexts = calloc(thread_count, sizeof(thread_context_t));
    if (!thread_contexts) {
        printf("[ERROR] Failed to allocate thread contexts\n");
        free(threads);
        threads = NULL;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    
    // Initialize pattern_buf before use
    memset(pattern_buf, 0, sizeof(pattern_buf));
    
    // Initialize thread contexts
    for (uint32_t i = 0; i < thread_count; i++) {
        thread_contexts[i].thread_num = i;
        atomic_init(&thread_contexts[i].should_exit, false);
        printf("[INFO] Initialized thread context for thread %u\n", i);
    }
    
    // Initialize atomic flags
    atomic_init(&found, false);
    atomic_init(&stopped, false);
    atomic_init(&total_attempts, 0);
    
    // Set up signal handler
    signal(SIGINT, handle_signal);
    
    initialized = true;
    printf("[INFO] Vanity search module initialized\n");
    pthread_mutex_unlock(&init_mutex);
    return 0;
}

// Start vanity address search
int gd_vanity_start(const char *pattern, bool case_sensitive) {
    printf("[INFO] Starting vanity search for pattern '%s' (case %ssensitive)\n", pattern, case_sensitive ? "" : "in");
    if (!initialized || !pattern) {
        printf("[ERROR] Module not initialized or invalid pattern\n");
        return -1;
    }
    
    // Copy and validate pattern
    pattern_len = strlen(pattern);
    if (pattern_len >= MAX_PATTERN_LENGTH) {
        printf("[ERROR] Pattern too long\n");
        return -1;
    }
    strncpy(pattern_buf, pattern, MAX_PATTERN_LENGTH - 1);
    pattern_buf[MAX_PATTERN_LENGTH - 1] = '\0';
    printf("[INFO] Pattern set to '%s'\n", pattern_buf);
    
    // Set case sensitivity
    case_sensitive_match = case_sensitive;
    
    // Reset state
    atomic_store(&found, false);
    atomic_store(&stopped, false);
    atomic_store(&total_attempts, 0);
    result_wif[0] = '\0';
    result_address[0] = '\0';
    
    // Record start time
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Start worker threads
    for (uint32_t i = 0; i < thread_count; i++) {
        atomic_store(&thread_contexts[i].should_exit, false);
        if (pthread_create(&threads[i], NULL, thread_worker, &thread_contexts[i]) != 0) {
            printf("[ERROR] Failed to create worker thread %u\n", i);
            free(threads);
            free(thread_contexts);
            threads = NULL;
            thread_contexts = NULL;
            pthread_mutex_unlock(&init_mutex);
            return -1;
        }
        printf("[INFO] Started worker thread %u\n", i);
    }
    
    return 0;
}

// Stop vanity address search
void gd_vanity_stop(void) {
    if (!initialized) {
        return;
    }
    
    debug_info("Stopping vanity search");
    
    // Signal threads to stop
    atomic_store(&stopped, true);
    for (uint32_t i = 0; i < thread_count; i++) {
        atomic_store(&thread_contexts[i].should_exit, true);
    }
    
    // Wait for threads to finish
    for (uint32_t i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
        debug_info("Thread %u joined", i);
    }
}

// Get search result
bool gd_vanity_get_result(char *privkey_wif, char *address) {
    if (!initialized || !privkey_wif || !address) {
        return false;
    }
    
    pthread_mutex_lock(&result_mutex);
    if (atomic_load(&found)) {
        strncpy(privkey_wif, result_wif, 52);
        privkey_wif[52] = '\0';
        strncpy(address, result_address, 34);
        address[34] = '\0';
        pthread_mutex_unlock(&result_mutex);
        return true;
    }
    pthread_mutex_unlock(&result_mutex);
    
    privkey_wif[0] = '\0';
    address[0] = '\0';
    return false;
}

// Cleanup vanity search module
void gd_vanity_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    debug_info("Cleaning up vanity search module");
    
    pthread_mutex_lock(&init_mutex);
    
    // Stop any running search
    gd_vanity_stop();
    
    // Free thread arrays
    free(threads);
    threads = NULL;
    free(thread_contexts);
    thread_contexts = NULL;
    
    // Reset state
    thread_count = 0;
    initialized = false;
    progress_callback = NULL;
    
    debug_info("Vanity search module cleaned up");
    pthread_mutex_unlock(&init_mutex);
}

// Main function to handle command-line arguments
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s vanity [mode] [pattern] [options]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *mode = argv[2];
    const char *pattern = argv[3];
    int num_threads = 1;
    bool case_sensitive = true;

    // Parse options
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0) {
            case_sensitive = false;
        }
    }

    // Initialize and start the vanity search
    if (gd_vanity_init(num_threads) != 0) {
        fprintf(stderr, "Failed to initialize vanity search module\n");
        return EXIT_FAILURE;
    }

    if (gd_vanity_start(pattern, case_sensitive) != 0) {
        fprintf(stderr, "Failed to start vanity search\n");
        gd_vanity_cleanup();
        return EXIT_FAILURE;
    }

    // Wait for the search to complete
    while (!atomic_load(&found)) {
        sleep(1);
    }

    // Retrieve and print the result
    char privkey_wif[53];
    char address[35];
    if (gd_vanity_get_result(privkey_wif, address)) {
        printf("Found match!\nPrivate Key (WIF): %s\nAddress: %s\n", privkey_wif, address);
    } else {
        printf("No match found.\n");
    }

    // Clean up resources
    gd_vanity_cleanup();
    return EXIT_SUCCESS;
}
