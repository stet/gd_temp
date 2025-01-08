#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include "pattern.h"
#include "vanity.h"

// Result structure for benchmark runs
typedef struct {
    uint32_t thread_count;
    uint64_t total_keys;
    uint64_t keys_per_second;
    double elapsed_seconds;
    double cpu_usage;
    size_t memory_bytes;
} benchmark_result_t;

/**
 * Run a benchmark test
 * @param pattern Pattern to search for
 * @param duration_seconds How long to run the test
 * @param thread_count Number of threads to use
 * @param result Output benchmark results
 * @return 0 on success, -1 on error
 */
int benchmark_run(const struct Pattern *pattern, uint32_t duration_seconds,
                 uint32_t thread_count, benchmark_result_t *result);

/**
 * Estimate time to find a match
 * @param pattern Pattern to search for
 * @param thread_count Number of threads to use
 * @param keys_per_second Speed from benchmark
 * @return Estimated seconds to find match
 */
double benchmark_estimate_time(const struct Pattern *pattern, uint32_t thread_count,
                             uint64_t keys_per_second);

/**
 * Print benchmark results
 * @param result Benchmark results to print
 * @param pattern Pattern that was searched for
 */
void benchmark_print_results(const benchmark_result_t *result, const struct Pattern *pattern);

#endif // BENCHMARK_H
