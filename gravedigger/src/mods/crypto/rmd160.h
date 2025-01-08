/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#ifndef RMD160_H
#define RMD160_H

#include <stddef.h>

#define RIPEMD160_DIGEST_LENGTH 20

typedef struct {
    unsigned int total[2];
    unsigned int state[5];
    unsigned char buffer[64];
    unsigned int num;
} rmd160_context;

void rmd160_init(rmd160_context *ctx);
void rmd160_update(rmd160_context *ctx, const unsigned char *input, size_t length);
void rmd160_final(rmd160_context *ctx, unsigned char *digest);

#endif
