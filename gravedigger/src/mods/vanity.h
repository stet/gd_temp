/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#ifndef VANITY_H
#define VANITY_H

#include <stdint.h>
#include <stdbool.h>
#include "privkey.h"
#include "pubkey.h"
#include "pattern.h"

// Forward declarations
typedef struct VanitySearch VanitySearch;

// Progress callback type
typedef void (*vanity_progress_cb)(uint64_t attempts, double rate, void *user_data);

/**
 * Initialize vanity address search
 * 
 * @param search Pointer to search context pointer
 * @param pattern Pattern to search for
 * @param case_sensitive Whether pattern matching is case sensitive
 * @param num_threads Number of threads to use
 * @return 0 on success, -1 on error
 */
int vanity_init(VanitySearch **search, const char *pattern, bool case_sensitive, int num_threads);

/**
 * Start the vanity address search
 * 
 * @param search Search context
 * @return 0 on success, -1 on error
 */
int vanity_start(VanitySearch *search);

/**
 * Stop the vanity address search
 * 
 * @param search Search context
 */
void vanity_stop(VanitySearch *search);

/**
 * Check if a match was found
 * 
 * @param search Search context
 * @return true if found, false otherwise
 */
bool vanity_found(VanitySearch *search);

/**
 * Check if search has been stopped
 * 
 * @param search Search context
 * @return true if stopped, false otherwise
 */
bool vanity_is_stopped(VanitySearch *search);

/**
 * Get total number of attempts made
 * 
 * @param search Search context
 * @return Number of attempts
 */
uint64_t vanity_get_attempts(VanitySearch *search);

/**
 * Get elapsed time in milliseconds
 * 
 * @param search Search context
 * @return Elapsed milliseconds
 */
uint64_t vanity_get_elapsed(VanitySearch *search);

/**
 * Get WIF format of found private key
 * 
 * @param search Search context
 * @param wif Buffer to store WIF string
 * @param wif_size Size of WIF buffer
 * @return 0 on success, -1 on error
 */
int vanity_get_wif(VanitySearch *search, char *wif, size_t wif_size);

/**
 * Get found Bitcoin address
 * 
 * @param search Search context
 * @param address Buffer to store address
 * @param address_size Size of address buffer
 * @return 0 on success, -1 on error
 */
int vanity_get_address(VanitySearch *search, char *address, size_t address_size);

/**
 * Set progress callback
 * 
 * @param search Search context
 * @param callback Progress callback function
 * @param user_data User data passed to callback
 * @param interval_ms Minimum interval between callbacks in milliseconds
 * @return 0 on success, -1 on error
 */
int vanity_set_progress_callback(VanitySearch *search, vanity_progress_cb callback,
                               void *user_data, int interval_ms);

/**
 * Clean up search context
 * 
 * @param search Search context to clean up
 */
void vanity_cleanup(VanitySearch *search);

#endif // VANITY_H