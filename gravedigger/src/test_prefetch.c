#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BATCH_SIZE 512
#define KEY_SIZE 32

// Test prefetch optimization
static inline void prefetch_batch(const void *addr) {
    __builtin_prefetch(addr, 0, 3);  // Read access, high temporal locality
}

int main() {
    // Allocate test buffer
    unsigned char *batch = aligned_alloc(16, BATCH_SIZE * KEY_SIZE);
    if (!batch) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    // Initialize with some data
    memset(batch, 0x42, BATCH_SIZE * KEY_SIZE);

    // Test prefetch
    for (int i = 0; i < BATCH_SIZE; i++) {
        prefetch_batch(batch + ((i + 1) * KEY_SIZE));
        // Simulate some work with current batch
        unsigned char sum = 0;
        for (int j = 0; j < KEY_SIZE; j++) {
            sum += batch[i * KEY_SIZE + j];
        }
    }

    printf("Prefetch test completed successfully\n");
    free(batch);
    return 0;
}
