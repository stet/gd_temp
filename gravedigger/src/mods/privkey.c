/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include "mods/crypto/rmd160.h"
#include "mods/crypto/sha256.h"
#include "mods/crypto.h"
#include "mods/base58check.h"
#include "mods/privkey.h"
#include "mods/pubkey.h"
#include "mods/network.h"
#include "mods/random.h"
#include "mods/hex.h"
#include "mods/error.h"
#include "mods/GMP/mini-gmp.h"

#define MAINNET_PREFIX      0x80
#define TESTNET_PREFIX      0xEF

int privkey_new(PrivKey key)
{
	int r;

	assert(key);

	r = random_get(key->data, PRIVKEY_LENGTH);
	if (r < 0)
	{
		error_log("Could not get random data for new private key.");
		return -1;
	}
	
	key->cflag = PRIVKEY_COMPRESSED_FLAG;
	
	return 1;
}

int privkey_compress(PrivKey key)
{
	assert(key);
	
	key->cflag = PRIVKEY_COMPRESSED_FLAG;
	
	return 1;
}

int privkey_uncompress(PrivKey key)
{
	assert(key);

	key->cflag = PRIVKEY_UNCOMPRESSED_FLAG;

	return 1;
}

int privkey_to_hex(char *str, PrivKey key, int cflag)
{
	int i;
	
	assert(key);
	assert(str);
	
	for (i = 0; i < PRIVKEY_LENGTH; ++i)
	{
		sprintf(str + (i * 2), "%02x", key->data[i]);
	}

	if (cflag)
	{
		sprintf(str + (i * 2), "%02x", key->cflag);
		++i;
	}

	str[i * 2] = '\0';
	
	return 1;
}

int privkey_to_raw(unsigned char *raw, PrivKey key, int cflag)
{
	int len = PRIVKEY_LENGTH;

	assert(key);
	assert(raw);

	memcpy(raw, key->data, PRIVKEY_LENGTH);
	
	if (cflag)
	{
		raw[PRIVKEY_LENGTH] = key->cflag;
		len += 1;
	}

	return len;
}

int privkey_to_dec(char *str, PrivKey key)
{
	int r;
	char privkey_hex[(PRIVKEY_LENGTH * 2) + 1];
	mpz_t d;

	mpz_init(d);

	r = privkey_to_hex(privkey_hex, key, 0);
	if (r < 0)
	{
		error_log("Could not convert private key to hex string.");
		return -1;
	}

	privkey_hex[PRIVKEY_LENGTH * 2] = '\0';
	mpz_set_str(d, privkey_hex, 16);

	mpz_get_str(str, 10, d);

	return 1;
}

int privkey_to_wif(char *str, PrivKey key)
{
	int r, len;
	unsigned char p[PRIVKEY_LENGTH + 2];
	char *base58check;

	assert(str);
	assert(key);

	len = PRIVKEY_LENGTH + 1;

	if (network_is_main())
	{
		p[0] = MAINNET_PREFIX;
	}
	else if (network_is_test())
	{
		p[0] = TESTNET_PREFIX;
	}
	memcpy(p+1, key->data, PRIVKEY_LENGTH);
	if (privkey_is_compressed(key))
	{
		p[PRIVKEY_LENGTH + 1] = PRIVKEY_COMPRESSED_FLAG;
		len += 1;
	}

	// Assume the base58 string will never be longer
	// than twice the input string
	base58check = malloc(len * 2);
	if (base58check == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}
	r = base58check_encode(base58check, p, len);
	if (r < 0)
	{
		error_log("Could not encode private key to WIF format.");
		return -1;
	}

	strcpy(str, base58check);

	free(base58check);
	
	return 1;
}

int privkey_from_wif(PrivKey key, char *wif)
{
	unsigned char *p;
	int l;

	assert(key);
	assert(wif);

	// Assume that the length of the decoded data will always
	// be less than the encoded data
	p = malloc(strlen(wif) * 2);
	if (p == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	l = base58check_decode(p, wif, BASE58CHECK_TYPE_NA);
	if (l < 0)
	{
		error_log("Could not parse input string.");
		return -1;
	}

	if (l != PRIVKEY_LENGTH + 1 && l != PRIVKEY_LENGTH + 2)
	{
		error_log("Decoded input contains %i bytes. Valid byte length must be %i or %i.", l, PRIVKEY_LENGTH + 1, PRIVKEY_LENGTH + 2);
		return -1;
	}

	switch (p[0])
	{
		case MAINNET_PREFIX:
			network_set_main();
			break;
		case TESTNET_PREFIX:
			network_set_test();
			break;
		default:
			error_log("Input contains invalid network prefix.");
			return -1;
			break;
	}
	
	if (l == PRIVKEY_LENGTH + 2)
	{
		if (p[l - 1] == PRIVKEY_COMPRESSED_FLAG)
		{
			privkey_compress(key);
		}
		else
		{
			error_log("Input contains invalid compression flag.");
			return -1;
		}
	}
	else
	{
		privkey_uncompress(key);
	}

	memcpy(key->data, p+1, PRIVKEY_LENGTH);

	free(p);
	
	return 1;
}

int privkey_from_hex(PrivKey key, char *input)
{
	int r;
	char *hex;
	size_t len;

	assert(key);
	assert(input);

	len = strlen(input);
	if (len != PRIVKEY_LENGTH * 2 && len != (PRIVKEY_LENGTH + 1) * 2)
	{
		error_log("Input string must be %i or %i characters in length.", PRIVKEY_LENGTH * 2, (PRIVKEY_LENGTH + 1) * 2);
		return -1;
	}

	hex = malloc(len + 1);
	if (hex == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	r = hex_str_to_raw((unsigned char *)hex, input);
	if (r < 0)
	{
		error_log("Could not decode hex string.");
		return -1;
	}

	memcpy(key->data, hex, PRIVKEY_LENGTH);

	if (len == (PRIVKEY_LENGTH + 1) * 2)
	{
		if (hex[PRIVKEY_LENGTH] == PRIVKEY_COMPRESSED_FLAG)
		{
			privkey_compress(key);
		}
		else
		{
			error_log("Input contains invalid compression flag.");
			return -1;
		}
	}
	else
	{
		privkey_uncompress(key);
	}

	free(hex);
	
	return 1;
}

int privkey_from_dec(PrivKey key, char *data)
{
	int r;
	char *hex;
	mpz_t d;

	assert(key);
	assert(data);

	mpz_init(d);

	r = mpz_set_str(d, data, 10);
	if (r < 0)
	{
		error_log("Could not parse input string.");
		return -1;
	}

	hex = malloc((PRIVKEY_LENGTH * 2) + 1);
	if (hex == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	mpz_get_str(hex, 16, d);

	r = privkey_from_hex(key, hex);
	if (r < 0)
	{
		error_log("Could not parse input string.");
		return -1;
	}

	free(hex);
	
	return 1;
}

int privkey_from_sbd(PrivKey key, char *data)
{
	int r;
	char *hex;
	mpz_t d;

	assert(key);
	assert(data);

	mpz_init(d);

	r = mpz_set_str(d, data, 10);
	if (r < 0)
	{
		error_log("Could not parse input string.");
		return -1;
	}

	hex = malloc((PRIVKEY_LENGTH * 2) + 1);
	if (hex == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	mpz_get_str(hex, 16, d);

	r = privkey_from_hex(key, hex);
	if (r < 0)
	{
		error_log("Could not parse input string.");
		return -1;
	}

	free(hex);
	
	return 1;
}

int privkey_from_str(PrivKey key, char *data)
{
	int r;
	unsigned char *hash;

	assert(key);
	assert(data);

	hash = malloc(32);
	if (hash == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	r = crypto_get_sha256(hash, (unsigned char *)data, strlen(data));
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}

	memcpy(key->data, hash, PRIVKEY_LENGTH);

	free(hash);
	
	return 1;
}

int privkey_from_raw(PrivKey key, unsigned char *raw, size_t l)
{
	assert(key);
	assert(raw);

	if (l != PRIVKEY_LENGTH && l != PRIVKEY_LENGTH + 1)
	{
		error_log("Input data must be %i or %i bytes in length.", PRIVKEY_LENGTH, PRIVKEY_LENGTH + 1);
		return -1;
	}

	memcpy(key->data, raw, PRIVKEY_LENGTH);

	if (l == PRIVKEY_LENGTH + 1)
	{
		if (raw[PRIVKEY_LENGTH] == PRIVKEY_COMPRESSED_FLAG)
		{
			privkey_compress(key);
		}
		else
		{
			error_log("Input contains invalid compression flag.");
			return -1;
		}
	}
	else
	{
		privkey_uncompress(key);
	}
	
	return 1;
}

int privkey_from_blob(PrivKey key, unsigned char *data, size_t data_len)
{
	int r;
	unsigned char *hash;

	assert(key);
	assert(data);

	hash = malloc(32);
	if (hash == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	r = crypto_get_sha256(hash, data, data_len);
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}

	memcpy(key->data, hash, PRIVKEY_LENGTH);

	free(hash);
	
	return 1;
}

int privkey_from_guess(PrivKey key, unsigned char *data, size_t data_len)
{
	int r;
	char *data_str;

	assert(key);
	assert(data);

	data_str = malloc(data_len + 1);
	if (data_str == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	memcpy(data_str, data, data_len);
	data_str[data_len] = '\0';

	r = privkey_from_wif(key, data_str);
	if (r < 0)
	{
		r = privkey_from_hex(key, data_str);
		if (r < 0)
		{
			r = privkey_from_dec(key, data_str);
			if (r < 0)
			{
				r = privkey_from_str(key, data_str);
				if (r < 0)
				{
					r = privkey_from_blob(key, data, data_len);
					if (r < 0)
					{
						error_log("Could not parse input data.");
						return -1;
					}
				}
			}
		}
	}

	free(data_str);
	
	return 1;
}

int privkey_is_compressed(PrivKey key)
{
	assert(key);

	return key->cflag == PRIVKEY_COMPRESSED_FLAG;
}

int privkey_is_zero(PrivKey key)
{
	int i;

	assert(key);

	for (i = 0; i < PRIVKEY_LENGTH; ++i)
	{
		if (key->data[i] != 0)
		{
			return 0;
		}
	}

	return 1;
}

size_t privkey_sizeof(void)
{
	return sizeof(struct PrivKey);
}

int privkey_rehash(PrivKey key)
{
	int r;
	unsigned char *hash;

	assert(key);

	hash = malloc(32);
	if (hash == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	r = crypto_get_sha256(hash, key->data, PRIVKEY_LENGTH);
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}

	memcpy(key->data, hash, PRIVKEY_LENGTH);

	free(hash);
	
	return 1;
}
