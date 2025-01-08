/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <string.h>
#include "rmd160.h"
#include "rmdconst.h"

#define DATA_ORDER_IS_LITTLE_ENDIAN

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

#define RIP1(a,b,c,d,e,w,s) { \
		a+=F1(b,c,d)+X(w); \
		a=ROTATE(a,s)+e; \
		c=ROTATE(c,10); }

#define RIP2(a,b,c,d,e,w,s,K) { \
		a+=F2(b,c,d)+X(w)+K; \
		a=ROTATE(a,s)+e; \
		c=ROTATE(c,10); }

#define RIP3(a,b,c,d,e,w,s,K) { \
		a+=F3(b,c,d)+X(w)+K; \
		a=ROTATE(a,s)+e; \
		c=ROTATE(c,10); }

#define RIP4(a,b,c,d,e,w,s,K) { \
		a+=F4(b,c,d)+X(w)+K; \
		a=ROTATE(a,s)+e; \
		c=ROTATE(c,10); }

#define RIP5(a,b,c,d,e,w,s,K) { \
		a+=F5(b,c,d)+X(w)+K; \
		a=ROTATE(a,s)+e; \
		c=ROTATE(c,10); }

#define HASH_MAKE_STRING(c,s)   do {    \
		unsigned long ll;               \
		ll=(c)->state[0]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[1]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[2]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[3]; (void)HOST_l2c(ll,(s));      \
		ll=(c)->state[4]; (void)HOST_l2c(ll,(s));      \
		} while (0)

#define F1(x,y,z)       ((x) ^ (y) ^ (z))
#define F2(x,y,z)       ((((y) ^ (z)) & (x)) ^ (z))
#define F3(x,y,z)       (((~(y)) | (x)) ^ (z))
#define F4(x,y,z)       ((((x) ^ (y)) & (z)) ^ (y))
#define F5(x,y,z)       (((~(z)) | (y)) ^ (x))

#define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))

#define RIPEMD160_A     0x67452301L
#define RIPEMD160_B     0xEFCDAB89L
#define RIPEMD160_C     0x98BADCFEL
#define RIPEMD160_D     0x10325476L
#define RIPEMD160_E     0xC3D2E1F0L

void ripemd160_block_data_order(rmd160_context *c, const void *p, size_t num);
void ripemd160_cleanse(void *ptr, size_t len);

typedef void *(*memset_t)(void *, int, size_t);
static volatile memset_t memset_func = memset;

void rmd160_init(rmd160_context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->state[0] = RIPEMD160_A;
	ctx->state[1] = RIPEMD160_B;
	ctx->state[2] = RIPEMD160_C;
	ctx->state[3] = RIPEMD160_D;
	ctx->state[4] = RIPEMD160_E;
}

void rmd160_update(rmd160_context *ctx, const unsigned char *data, size_t len)
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
			ripemd160_block_data_order(ctx, p, 1);
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
		ripemd160_block_data_order(ctx, data, n);
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

void rmd160_final(rmd160_context *ctx, unsigned char *md)
{
	unsigned char *p = (unsigned char *)ctx->buffer;
	size_t n = ctx->num;

	p[n] = 0x80;                /* there is always room for one */
	n++;

	if (n > (64 - 8)) {
		memset(p + n, 0, 64 - n);
		n = 0;
		ripemd160_block_data_order(ctx, p, 1);
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
	ripemd160_block_data_order(ctx, p, 1);
	ctx->num = 0;
	ripemd160_cleanse(p, 64);

	HASH_MAKE_STRING(ctx, md);
}

void ripemd160_block_data_order(rmd160_context *ctx, const void *p, size_t num)
{
	const unsigned char *data = p;
	register unsigned int A, B, C, D, E;
	unsigned int a, b, c, d, e, l;
	unsigned int XX0, XX1, XX2, XX3, XX4, XX5, XX6, XX7,
		XX8, XX9, XX10, XX11, XX12, XX13, XX14, XX15;
#define X(i)   XX##i

	for (; num--;) {

		A = ctx->state[0];
		B = ctx->state[1];
		C = ctx->state[2];
		D = ctx->state[3];
		E = ctx->state[4];

		(void)HOST_c2l(data, l);
		X(0) = l;
		(void)HOST_c2l(data, l);
		X(1) = l;
		RIP1(A, B, C, D, E, WL00, SL00);
		(void)HOST_c2l(data, l);
		X(2) = l;
		RIP1(E, A, B, C, D, WL01, SL01);
		(void)HOST_c2l(data, l);
		X(3) = l;
		RIP1(D, E, A, B, C, WL02, SL02);
		(void)HOST_c2l(data, l);
		X(4) = l;
		RIP1(C, D, E, A, B, WL03, SL03);
		(void)HOST_c2l(data, l);
		X(5) = l;
		RIP1(B, C, D, E, A, WL04, SL04);
		(void)HOST_c2l(data, l);
		X(6) = l;
		RIP1(A, B, C, D, E, WL05, SL05);
		(void)HOST_c2l(data, l);
		X(7) = l;
		RIP1(E, A, B, C, D, WL06, SL06);
		(void)HOST_c2l(data, l);
		X(8) = l;
		RIP1(D, E, A, B, C, WL07, SL07);
		(void)HOST_c2l(data, l);
		X(9) = l;
		RIP1(C, D, E, A, B, WL08, SL08);
		(void)HOST_c2l(data, l);
		X(10) = l;
		RIP1(B, C, D, E, A, WL09, SL09);
		(void)HOST_c2l(data, l);
		X(11) = l;
		RIP1(A, B, C, D, E, WL10, SL10);
		(void)HOST_c2l(data, l);
		X(12) = l;
		RIP1(E, A, B, C, D, WL11, SL11);
		(void)HOST_c2l(data, l);
		X(13) = l;
		RIP1(D, E, A, B, C, WL12, SL12);
		(void)HOST_c2l(data, l);
		X(14) = l;
		RIP1(C, D, E, A, B, WL13, SL13);
		(void)HOST_c2l(data, l);
		X(15) = l;
		RIP1(B, C, D, E, A, WL14, SL14);
		RIP1(A, B, C, D, E, WL15, SL15);

		RIP2(E, A, B, C, D, WL16, SL16, KL1);
		RIP2(D, E, A, B, C, WL17, SL17, KL1);
		RIP2(C, D, E, A, B, WL18, SL18, KL1);
		RIP2(B, C, D, E, A, WL19, SL19, KL1);
		RIP2(A, B, C, D, E, WL20, SL20, KL1);
		RIP2(E, A, B, C, D, WL21, SL21, KL1);
		RIP2(D, E, A, B, C, WL22, SL22, KL1);
		RIP2(C, D, E, A, B, WL23, SL23, KL1);
		RIP2(B, C, D, E, A, WL24, SL24, KL1);
		RIP2(A, B, C, D, E, WL25, SL25, KL1);
		RIP2(E, A, B, C, D, WL26, SL26, KL1);
		RIP2(D, E, A, B, C, WL27, SL27, KL1);
		RIP2(C, D, E, A, B, WL28, SL28, KL1);
		RIP2(B, C, D, E, A, WL29, SL29, KL1);
		RIP2(A, B, C, D, E, WL30, SL30, KL1);
		RIP2(E, A, B, C, D, WL31, SL31, KL1);

		RIP3(D, E, A, B, C, WL32, SL32, KL2);
		RIP3(C, D, E, A, B, WL33, SL33, KL2);
		RIP3(B, C, D, E, A, WL34, SL34, KL2);
		RIP3(A, B, C, D, E, WL35, SL35, KL2);
		RIP3(E, A, B, C, D, WL36, SL36, KL2);
		RIP3(D, E, A, B, C, WL37, SL37, KL2);
		RIP3(C, D, E, A, B, WL38, SL38, KL2);
		RIP3(B, C, D, E, A, WL39, SL39, KL2);
		RIP3(A, B, C, D, E, WL40, SL40, KL2);
		RIP3(E, A, B, C, D, WL41, SL41, KL2);
		RIP3(D, E, A, B, C, WL42, SL42, KL2);
		RIP3(C, D, E, A, B, WL43, SL43, KL2);
		RIP3(B, C, D, E, A, WL44, SL44, KL2);
		RIP3(A, B, C, D, E, WL45, SL45, KL2);
		RIP3(E, A, B, C, D, WL46, SL46, KL2);
		RIP3(D, E, A, B, C, WL47, SL47, KL2);

		RIP4(C, D, E, A, B, WL48, SL48, KL3);
		RIP4(B, C, D, E, A, WL49, SL49, KL3);
		RIP4(A, B, C, D, E, WL50, SL50, KL3);
		RIP4(E, A, B, C, D, WL51, SL51, KL3);
		RIP4(D, E, A, B, C, WL52, SL52, KL3);
		RIP4(C, D, E, A, B, WL53, SL53, KL3);
		RIP4(B, C, D, E, A, WL54, SL54, KL3);
		RIP4(A, B, C, D, E, WL55, SL55, KL3);
		RIP4(E, A, B, C, D, WL56, SL56, KL3);
		RIP4(D, E, A, B, C, WL57, SL57, KL3);
		RIP4(C, D, E, A, B, WL58, SL58, KL3);
		RIP4(B, C, D, E, A, WL59, SL59, KL3);
		RIP4(A, B, C, D, E, WL60, SL60, KL3);
		RIP4(E, A, B, C, D, WL61, SL61, KL3);
		RIP4(D, E, A, B, C, WL62, SL62, KL3);
		RIP4(C, D, E, A, B, WL63, SL63, KL3);

		RIP5(B, C, D, E, A, WL64, SL64, KL4);
		RIP5(A, B, C, D, E, WL65, SL65, KL4);
		RIP5(E, A, B, C, D, WL66, SL66, KL4);
		RIP5(D, E, A, B, C, WL67, SL67, KL4);
		RIP5(C, D, E, A, B, WL68, SL68, KL4);
		RIP5(B, C, D, E, A, WL69, SL69, KL4);
		RIP5(A, B, C, D, E, WL70, SL70, KL4);
		RIP5(E, A, B, C, D, WL71, SL71, KL4);
		RIP5(D, E, A, B, C, WL72, SL72, KL4);
		RIP5(C, D, E, A, B, WL73, SL73, KL4);
		RIP5(B, C, D, E, A, WL74, SL74, KL4);
		RIP5(A, B, C, D, E, WL75, SL75, KL4);
		RIP5(E, A, B, C, D, WL76, SL76, KL4);
		RIP5(D, E, A, B, C, WL77, SL77, KL4);
		RIP5(C, D, E, A, B, WL78, SL78, KL4);
		RIP5(B, C, D, E, A, WL79, SL79, KL4);

		a = A;
		b = B;
		c = C;
		d = D;
		e = E;

		/* Do other half */
		A = ctx->state[0];
		B = ctx->state[1];
		C = ctx->state[2];
		D = ctx->state[3];
		E = ctx->state[4];

		RIP5(A, B, C, D, E, WR00, SR00, KR0);
		RIP5(E, A, B, C, D, WR01, SR01, KR0);
		RIP5(D, E, A, B, C, WR02, SR02, KR0);
		RIP5(C, D, E, A, B, WR03, SR03, KR0);
		RIP5(B, C, D, E, A, WR04, SR04, KR0);
		RIP5(A, B, C, D, E, WR05, SR05, KR0);
		RIP5(E, A, B, C, D, WR06, SR06, KR0);
		RIP5(D, E, A, B, C, WR07, SR07, KR0);
		RIP5(C, D, E, A, B, WR08, SR08, KR0);
		RIP5(B, C, D, E, A, WR09, SR09, KR0);
		RIP5(A, B, C, D, E, WR10, SR10, KR0);
		RIP5(E, A, B, C, D, WR11, SR11, KR0);
		RIP5(D, E, A, B, C, WR12, SR12, KR0);
		RIP5(C, D, E, A, B, WR13, SR13, KR0);
		RIP5(B, C, D, E, A, WR14, SR14, KR0);
		RIP5(A, B, C, D, E, WR15, SR15, KR0);

		RIP4(E, A, B, C, D, WR16, SR16, KR1);
		RIP4(D, E, A, B, C, WR17, SR17, KR1);
		RIP4(C, D, E, A, B, WR18, SR18, KR1);
		RIP4(B, C, D, E, A, WR19, SR19, KR1);
		RIP4(A, B, C, D, E, WR20, SR20, KR1);
		RIP4(E, A, B, C, D, WR21, SR21, KR1);
		RIP4(D, E, A, B, C, WR22, SR22, KR1);
		RIP4(C, D, E, A, B, WR23, SR23, KR1);
		RIP4(B, C, D, E, A, WR24, SR24, KR1);
		RIP4(A, B, C, D, E, WR25, SR25, KR1);
		RIP4(E, A, B, C, D, WR26, SR26, KR1);
		RIP4(D, E, A, B, C, WR27, SR27, KR1);
		RIP4(C, D, E, A, B, WR28, SR28, KR1);
		RIP4(B, C, D, E, A, WR29, SR29, KR1);
		RIP4(A, B, C, D, E, WR30, SR30, KR1);
		RIP4(E, A, B, C, D, WR31, SR31, KR1);

		RIP3(D, E, A, B, C, WR32, SR32, KR2);
		RIP3(C, D, E, A, B, WR33, SR33, KR2);
		RIP3(B, C, D, E, A, WR34, SR34, KR2);
		RIP3(A, B, C, D, E, WR35, SR35, KR2);
		RIP3(E, A, B, C, D, WR36, SR36, KR2);
		RIP3(D, E, A, B, C, WR37, SR37, KR2);
		RIP3(C, D, E, A, B, WR38, SR38, KR2);
		RIP3(B, C, D, E, A, WR39, SR39, KR2);
		RIP3(A, B, C, D, E, WR40, SR40, KR2);
		RIP3(E, A, B, C, D, WR41, SR41, KR2);
		RIP3(D, E, A, B, C, WR42, SR42, KR2);
		RIP3(C, D, E, A, B, WR43, SR43, KR2);
		RIP3(B, C, D, E, A, WR44, SR44, KR2);
		RIP3(A, B, C, D, E, WR45, SR45, KR2);
		RIP3(E, A, B, C, D, WR46, SR46, KR2);
		RIP3(D, E, A, B, C, WR47, SR47, KR2);

		RIP2(C, D, E, A, B, WR48, SR48, KR3);
		RIP2(B, C, D, E, A, WR49, SR49, KR3);
		RIP2(A, B, C, D, E, WR50, SR50, KR3);
		RIP2(E, A, B, C, D, WR51, SR51, KR3);
		RIP2(D, E, A, B, C, WR52, SR52, KR3);
		RIP2(C, D, E, A, B, WR53, SR53, KR3);
		RIP2(B, C, D, E, A, WR54, SR54, KR3);
		RIP2(A, B, C, D, E, WR55, SR55, KR3);
		RIP2(E, A, B, C, D, WR56, SR56, KR3);
		RIP2(D, E, A, B, C, WR57, SR57, KR3);
		RIP2(C, D, E, A, B, WR58, SR58, KR3);
		RIP2(B, C, D, E, A, WR59, SR59, KR3);
		RIP2(A, B, C, D, E, WR60, SR60, KR3);
		RIP2(E, A, B, C, D, WR61, SR61, KR3);
		RIP2(D, E, A, B, C, WR62, SR62, KR3);
		RIP2(C, D, E, A, B, WR63, SR63, KR3);

		RIP1(B, C, D, E, A, WR64, SR64);
		RIP1(A, B, C, D, E, WR65, SR65);
		RIP1(E, A, B, C, D, WR66, SR66);
		RIP1(D, E, A, B, C, WR67, SR67);
		RIP1(C, D, E, A, B, WR68, SR68);
		RIP1(B, C, D, E, A, WR69, SR69);
		RIP1(A, B, C, D, E, WR70, SR70);
		RIP1(E, A, B, C, D, WR71, SR71);
		RIP1(D, E, A, B, C, WR72, SR72);
		RIP1(C, D, E, A, B, WR73, SR73);
		RIP1(B, C, D, E, A, WR74, SR74);
		RIP1(A, B, C, D, E, WR75, SR75);
		RIP1(E, A, B, C, D, WR76, SR76);
		RIP1(D, E, A, B, C, WR77, SR77);
		RIP1(C, D, E, A, B, WR78, SR78);
		RIP1(B, C, D, E, A, WR79, SR79);

		D = ctx->state[1] + c + D;
		ctx->state[1] = ctx->state[2] + d + E;
		ctx->state[2] = ctx->state[3] + e + A;
		ctx->state[3] = ctx->state[4] + a + B;
		ctx->state[4] = ctx->state[0] + b + C;
		ctx->state[0] = D;

		data += 64;
	}
}

void ripemd160_cleanse(void *ptr, size_t len)
{
	memset_func(ptr, 0, len);
}