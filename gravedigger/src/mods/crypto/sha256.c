/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <string.h>
#include "sha256.h"
#include "shaconst.h"

#define DATA_ORDER_IS_BIG_ENDIAN

#if defined(DATA_ORDER_IS_BIG_ENDIAN)

#define HOST_c2l(c,l)  (l =(((unsigned long)(*((c)++)))<<24),          \
						 l|=(((unsigned long)(*((c)++)))<<16),          \
						 l|=(((unsigned long)(*((c)++)))<< 8),          \
						 l|=(((unsigned long)(*((c)++)))    )           )
#define HOST_l2c(l,c)  (*((c)++)=(unsigned char)(((l)>>24)&0xff),      \
						 *((c)++)=(unsigned char)(((l)>>16)&0xff),      \
						 *((c)++)=(unsigned char)(((l)>> 8)&0xff),      \
						 *((c)++)=(unsigned char)(((l)    )&0xff),      \
						 l)

#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)

#define HOST_c2l(c,l)  (l =(((unsigned long)(*((c)++)))    ),          \
						 l|=(((unsigned long)(*((c)++)))<< 8),          \
						 l|=(((unsigned long)(*((c)++)))<<16),          \
						 l|=(((unsigned long)(*((c)++)))<<24)           )
#define HOST_l2c(l,c)  (*((c)++)=(unsigned char)(((l)    )&0xff),      \
						 *((c)++)=(unsigned char)(((l)>> 8)&0xff),      \
						 *((c)++)=(unsigned char)(((l)>>16)&0xff),      \
						 *((c)++)=(unsigned char)(((l)>>24)&0xff),      \
						 l)
#endif

#define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))

#define Sigma0(x)    (ROTATE((x),30) ^ ROTATE((x),19) ^ ROTATE((x),10))
#define Sigma1(x)    (ROTATE((x),26) ^ ROTATE((x),21) ^ ROTATE((x),7))
#define sigma0(x)    (ROTATE((x),25) ^ ROTATE((x),14) ^ ((x)>>3))
#define sigma1(x)    (ROTATE((x),15) ^ ROTATE((x),13) ^ ((x)>>10))

#define Ch(x,y,z)       (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)      (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define HASH_MAKE_STRING(c,s)   do {    \
		unsigned long ll;               \
		ll=(c)->state[0]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[1]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[2]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[3]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[4]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[5]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[6]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[7]; (void)HOST_l2c(ll,(s));      \
		} while (0)

void sha256_block_data_order(sha256_context *ctx, const void *in, size_t num);
void sha256_cleanse(void *ptr, size_t len);

typedef void *(*memset_t)(void *, int, size_t);
static volatile memset_t memset_func = memset;

void sha256_init(sha256_context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->state[0] = 0x6a09e667UL;
	ctx->state[1] = 0xbb67ae85UL;
	ctx->state[2] = 0x3c6ef372UL;
	ctx->state[3] = 0xa54ff53aUL;
	ctx->state[4] = 0x510e527fUL;
	ctx->state[5] = 0x9b05688cUL;
	ctx->state[6] = 0x1f83d9abUL;
	ctx->state[7] = 0x5be0cd19UL;
}

void sha256_update(sha256_context *ctx, const unsigned char *data, size_t len)
{
	unsigned char *p;
	unsigned int l;
	size_t n;

	if (len == 0)
		return;

	l = (ctx->total[0] + (((unsigned int) len) << 3)) & 0xffffffffUL;
	if (l < ctx->total[0])              /* overflow */
		ctx->total[1]++;
	ctx->total[1] += (unsigned int) (len >> 29); /* might cause compiler warning on 16-bit */
	ctx->total[0] = l;

	n = ctx->num;
	if (n != 0) {
		p = (unsigned char *)ctx->buffer;

		if (len >= 64 || len + n >= 64) {
			memcpy(p + n, data, 64 - n);
			sha256_block_data_order(ctx, p, 1);
			n = 64 - n;
			data += n;
			len -= n;
			ctx->num = 0;
			/*
			 * We use memset rather than OPENSSL_cleanse() here deliberately.
			 * Using OPENSSL_cleanse() here could be a performance issue. It
			 * will get properly cleansed on finalisation so this isn't a
			 * security problem.
			 */
			memset(p, 0, 64); /* keep it zeroed */
		} else {
			memcpy(p + n, data, len);
			ctx->num += (unsigned int)len;
			return;
		}
	}

	n = len / 64;
	if (n > 0) {
		sha256_block_data_order(ctx, data, n);
		n *= 64;
		data += n;
		len -= n;
	}

	if (len != 0) {
		p = (unsigned char *)ctx->buffer;
		ctx->num = (unsigned int)len;
		memcpy(p, data, len);
	}
}

void sha256_final(sha256_context *ctx, unsigned char *md)
{
	unsigned char *p = (unsigned char *)ctx->buffer;
	size_t n = ctx->num;

	p[n] = 0x80;                /* there is always room for one */
	n++;

	if (n > (64 - 8)) {
		memset(p + n, 0, 64 - n);
		n = 0;
		sha256_block_data_order(ctx, p, 1);
	}
	memset(p + n, 0, 64 - 8 - n);

	p += 64 - 8;
#if defined(DATA_ORDER_IS_BIG_ENDIAN)
	(void)HOST_l2c(ctx->total[1], p);
	(void)HOST_l2c(ctx->total[0], p);
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
	(void)HOST_l2c(ctx->total[0], p);
	(void)HOST_l2c(ctx->total[1], p);
#endif
	p -= 64;
	sha256_block_data_order(ctx, p, 1);
	ctx->num = 0;
	sha256_cleanse(p, 64);

	HASH_MAKE_STRING(ctx, md);
}

void sha256_block_data_order(sha256_context *ctx, const void *in, size_t num)
{
	unsigned int a, b, c, d, e, f, g, h, s0, s1, T1, T2;
	unsigned int X[16];
	int i;
	const unsigned char *data = in;

	while (num--) {

		a = ctx->state[0];
		b = ctx->state[1];
		c = ctx->state[2];
		d = ctx->state[3];
		e = ctx->state[4];
		f = ctx->state[5];
		g = ctx->state[6];
		h = ctx->state[7];

		if (data != NULL) {
			HOST_c2l(data, X[0]);
			HOST_c2l(data, X[1]);
			HOST_c2l(data, X[2]);
			HOST_c2l(data, X[3]);
			HOST_c2l(data, X[4]);
			HOST_c2l(data, X[5]);
			HOST_c2l(data, X[6]);
			HOST_c2l(data, X[7]);
			HOST_c2l(data, X[8]);
			HOST_c2l(data, X[9]);
			HOST_c2l(data, X[10]);
			HOST_c2l(data, X[11]);
			HOST_c2l(data, X[12]);
			HOST_c2l(data, X[13]);
			HOST_c2l(data, X[14]);
			HOST_c2l(data, X[15]);
		}

		for (i = 0; i < 16; i++) {
			T1 = h + Sigma1(e) + Ch(e, f, g) + K256[i] + X[i];
			T2 = Sigma0(a) + Maj(a, b, c);
			h = g;
			g = f;
			f = e;
			e = d + T1;
			d = c;
			c = b;
			b = a;
			a = T1 + T2;
		}

		for (; i < 64; i++) {
			s0 = X[(i + 1) & 0x0f];
			s0 = sigma0(s0);
			s1 = X[(i + 14) & 0x0f];
			s1 = sigma1(s1);

			T1 = h + Sigma1(e) + Ch(e, f, g) + K256[i] +
				(X[i & 0xf] += s0 + s1 + X[(i + 9) & 0xf]);
			T2 = Sigma0(a) + Maj(a, b, c);
			h = g;
			g = f;
			f = e;
			e = d + T1;
			d = c;
			c = b;
			b = a;
			a = T1 + T2;
		}

		ctx->state[0] += a;
		ctx->state[1] += b;
		ctx->state[2] += c;
		ctx->state[3] += d;
		ctx->state[4] += e;
		ctx->state[5] += f;
		ctx->state[6] += g;
		ctx->state[7] += h;

	}
}

void sha256_cleanse(void *ptr, size_t len)
{
	memset_func(ptr, 0, len);
}