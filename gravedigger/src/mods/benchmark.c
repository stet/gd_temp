#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "benchmark.h"
#include "error.h"

// Get current timestamp in seconds with microsecond precision
static double get_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// Get current process CPU usage
static double get_cpu_usage(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }
    
    // Convert to seconds
    double user = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);
    double sys = usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1000000.0);
    
    return user + sys;
}

// Get current process memory usage
static size_t get_memory_usage(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
    
    // ru_maxrss is in kilobytes
    return usage.ru_maxrss * 1024;
}

// Benchmark progress callback
static void benchmark_progress(uint64_t attempts, double rate, void *user_data) {
    benchmark_result_t *result = (benchmark_result_t *)user_data;
    result->total_keys = attempts;
    result->keys_per_second = (uint64_t)rate;
}

int benchmark_run(const struct Pattern *pattern, uint32_t duration_seconds,
                 uint32_t thread_count, benchmark_result_t *result) {
    if (!pattern || !result || duration_seconds == 0) {
        error_log("Invalid benchmark parameters");
        return -1;
    }
    
    // Initialize result structure
    memset(result, 0, sizeof(benchmark_result_t));
    result->thread_count = thread_count;
    
    // Create vanity search context
    VanitySearch *search = NULL;
    if (vanity_init(&search, pattern->str.str, pattern->case_sensitive, thread_count) < 0) {
        error_log("Failed to initialize vanity search");
        return -1;
    }
    
    // Set up progress callback
    vanity_set_progress_callback(search, benchmark_progress, result, 1000);
    
    // Record start state
    double start_time = get_timestamp();
    double start_cpu = get_cpu_usage();
    
    // Run the search
    if (vanity_start(search) < 0) {
        error_log("Failed to start vanity search");
        vanity_cleanup(search);
        return -1;
    }
    
    // Wait for specified duration
    sleep(duration_seconds);
    
    // Stop the search
    vanity_stop(search);
    
    // Record end state
    double end_time = get_timestamp();
    double end_cpu = get_cpu_usage();
    
    // Calculate results
    result->elapsed_seconds = end_time - start_time;
    result->cpu_usage = ((end_cpu - start_cpu) / result->elapsed_seconds) * 100.0;
    result->memory_bytes = get_memory_usage();
    
    // Clean up
    vanity_cleanup(search);
    
    return 0;
}

double benchmark_estimate_time(const struct Pattern *pattern, uint32_t thread_count,
                             uint64_t keys_per_second) {
    if (!pattern || keys_per_second == 0) return 0.0;
    
    // Get pattern probability
    double prob = pattern_probability(pattern);
    if (prob <= 0.0) return 0.0;
    
    // Expected attempts needed = 1/probability
    double attempts_needed = 1.0 / prob;
    
    // Time = attempts / (keys_per_second * thread_count)
    return attempts_needed / (keys_per_second * thread_count);
}

void benchmark_print_results(const benchmark_result_t *result, const struct Pattern *pattern) {
    if (!result || !pattern) return;
    
    printf("\nBenchmark Results:\n");
    printf("----------------\n");
    printf("Pattern: %s\n", pattern->str.str);
    printf("Threads: %u\n", result->thread_count);
    printf("Performance: %lu keys/second\n", result->keys_per_second);
    printf("CPU Usage: %.1f%%\n", result->cpu_usage);
    printf("Memory Usage: %.1f MB\n", result->memory_bytes / (1024.0 * 1024.0));
    
    // Estimate time to find match
    double est_time = benchmark_estimate_time(pattern, result->thread_count,
                                            result->keys_per_second);
    
    if (est_time > 0.0) {
        if (est_time < 60) {
            printf("Estimated time to match: %.1f seconds\n", est_time);
        } else if (est_time < 3600) {
            printf("Estimated time to match: %.1f minutes\n", est_time / 60);
        } else if (est_time < 86400) {
            printf("Estimated time to match: %.1f hours\n", est_time / 3600);
        } else {
            printf("Estimated time to match: %.1f days\n", est_time / 86400);
        }
    }
    
    printf("\n");
}
