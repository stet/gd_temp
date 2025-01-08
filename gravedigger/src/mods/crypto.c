/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "crypto/rmd160.h"
#include "crypto/sha256.h"
#include "crypto.h"
#include "error.h"

int crypto_get_sha256(unsigned char *output, unsigned char *input, size_t input_len)
{
	sha256_context ctx;
	assert(output);
	assert(input);

	sha256_init(&ctx);
	sha256_update(&ctx, input, input_len);
	sha256_final(&ctx, output);
	
	return 1;
}

int crypto_get_rmd160(unsigned char *output, unsigned char *input, size_t input_len)
{
	rmd160_context ctx;
	assert(output);
	assert(input);
	assert(input_len);

	rmd160_init(&ctx);
	rmd160_update(&ctx, input, input_len);
	rmd160_final(&ctx, output);

	return 1;
}

int crypto_get_checksum(uint32_t *output, unsigned char *data, size_t len)
{
	int r;
	unsigned char *sha1, *sha2;

	assert(output);
	assert(data);

	sha1 = malloc(32);
	if (sha1 == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}
	sha2 = malloc(32);
	if (sha2 == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	r = crypto_get_sha256(sha1, data, len);
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}
	r = crypto_get_sha256(sha2, sha1, 32);
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}

	*output <<= 8;
	*output += sha2[0];
	*output <<= 8;
	*output += sha2[1];
	*output <<= 8;
	*output += sha2[2];
	*output <<= 8;
	*output += sha2[3];
	
	free(sha1);
	free(sha2);
	
	return 1;
}
