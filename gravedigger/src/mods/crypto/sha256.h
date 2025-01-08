/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>

#define SHA256_DIGEST_LENGTH 32

typedef struct {
    unsigned int total[2];
    unsigned int state[8];
    unsigned char buffer[64];
    unsigned int num;
} sha256_context;

void sha256_init(sha256_context *ctx);
void sha256_update(sha256_context *ctx, const unsigned char *input, size_t length);
void sha256_final(sha256_context *ctx, unsigned char *digest);

#endif
