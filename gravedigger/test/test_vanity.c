#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include "../src/mods/gd_vanity.h"
#include "../src/mods/privkey.h"
#include "../src/mods/pubkey.h"
#include "../src/mods/address.h"
#include "../src/mods/debug.h"

// Known test vectors from https://en.bitcoin.it/wiki/Technical_background_of_version_1_Bitcoin_addresses
static const unsigned char test_private_key[] = {
    0x18, 0xE1, 0x4A, 0x7B, 0x6A, 0x30, 0x7F, 0x42,
    0x6A, 0x94, 0xF8, 0x11, 0x47, 0x01, 0xE7, 0xC8,
    0xE7, 0x74, 0xE7, 0xF9, 0xA4, 0x7E, 0x2C, 0x20,
    0x35, 0xDB, 0x29, 0xA2, 0x06, 0x32, 0x17, 0x25
};

static const char test_address[] = "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM";

// Test with known key pair
int test_known_key_pair(void)
{
    struct PrivKey priv;
    struct PubKey pub;
    char address[ADDRESS_LENGTH];

    // Initialize private key from test vector
    memcpy(priv.data, test_private_key, PRIVKEY_LENGTH);
    priv.compressed = false;

    // Generate public key
    if (!pubkey_get(&pub, &priv)) {
        printf("Failed to get public key\n");
        return 1;
    }

    // Generate address
    if (!address_get(address, &pub)) {
        printf("Failed to get address\n");
        return 1;
    }

    // Verify address matches test vector
    if (strcmp(address, test_address) != 0) {
        printf("Address mismatch:\n");
        printf("  Expected: %s\n", test_address);
        printf("  Got:      %s\n", address);
        return 1;
    }

    return 0;
}

// Test vanity address generation
int test_vanity_search(void)
{
    gd_vanity_stats_t stats = {0};
    pthread_mutex_init(&stats.lock, NULL);
    stats.start_time = time(NULL);

    gd_vanity_thread_ctx_t ctx = {
        .thread_id = 0,
        .pattern = "1test",
        .case_sensitive = true,
        .running = true,
        .stats = &stats
    };

    // Run search in current thread
    gd_vanity_search_worker(&ctx);

    // Verify we found a match
    pthread_mutex_lock(&stats.lock);
    bool found = stats.matches > 0;
    pthread_mutex_unlock(&stats.lock);

    pthread_mutex_destroy(&stats.lock);

    return found ? 0 : 1;
}

int main()
{
    int result = 0;

    printf("Running tests...\n");

    printf("Test 1: Known key pair... ");
    if (test_known_key_pair() == 0) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        result = 1;
    }

    printf("Test 2: Vanity search... ");
    if (test_vanity_search() == 0) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        result = 1;
    }

    return result;
}