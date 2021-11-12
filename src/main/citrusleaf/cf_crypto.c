/* 
 * Copyright 2008-2021 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

//==========================================================
// Includes.
//

#include <citrusleaf/cf_crypto.h>
#include <stdio.h>



/*
 * Copy of SHA1 API and implementation.
 *
 * For FIPS 140 compliance, this is NOT to be used for crypto purposes!
 */

//==========================================================
// Typedefs & constants.
//

enum {
	shaSuccess = 0,
	shaNull,
	shaInputTooLong,
	shaStateError
};

#define SHA1HashSize 20

typedef struct SHA1Context {
	uint32_t Intermediate_Hash[SHA1HashSize / 4];

	uint32_t Length_Low;
	uint32_t Length_High;

	int_least16_t Message_Block_Index;
	uint8_t Message_Block[64];

	int Computed;
	int Corrupted;
} SHA1Context;


//==========================================================
// Forward declarations.
//

static int SHA1Reset(SHA1Context *context);
static int SHA1Input(SHA1Context *context, const uint8_t *message_array, unsigned length);
static int SHA1Result(SHA1Context *context, uint8_t Message_Digest[SHA1HashSize]);

static void SHA1ProcessMessageBlock(SHA1Context *);
static void SHA1PadMessage(SHA1Context *);


//==========================================================
// Inlines & macros.
//

#define SHA1CircularShift(bits,word) \
	(((word) << (bits)) | ((word) >> (32-(bits))))


//==========================================================
// Public API.
//

void
SHA1(const uint8_t* d, size_t n, uint8_t* md)
{
	SHA1Context sha;

	SHA1Reset(&sha);
	SHA1Input(&sha, d, n);
	SHA1Result(&sha, md);
}

// TODO - left here for legacy reasons, should be no need for this.
bool
cf_convert_sha1_to_hex(unsigned char* hash, unsigned char* sha1_hex_buff)
{
	if (! sha1_hex_buff || ! hash) {
		return false;
	}

	for (unsigned int i = 0; i < SHA_DIGEST_LENGTH; i++) {
		sprintf((char*)(sha1_hex_buff + (i * 2)), "%02x", hash[i]);
	}

	return true;
}


//==========================================================
// Local helpers.
//

static int
SHA1Reset(SHA1Context *context)
{
	if (! context) {
		return shaNull;
	}

	context->Length_Low = 0;
	context->Length_High = 0;
	context->Message_Block_Index = 0;

	context->Intermediate_Hash[0] = 0x67452301;
	context->Intermediate_Hash[1] = 0xEFCDAB89;
	context->Intermediate_Hash[2] = 0x98BADCFE;
	context->Intermediate_Hash[3] = 0x10325476;
	context->Intermediate_Hash[4] = 0xC3D2E1F0;

	context->Computed = 0;
	context->Corrupted = 0;

	return shaSuccess;
}

static int
SHA1Input(SHA1Context *context, const uint8_t *message_array, unsigned length)
{
	if (! length) {
		return shaSuccess;
	}

	if (! context || ! message_array) {
		return shaNull;
	}

	if (context->Computed) {
		context->Corrupted = shaStateError;
		return shaStateError;
	}

	if (context->Corrupted) {
		return context->Corrupted;
	}

	while(length-- && ! context->Corrupted) {
		context->Message_Block[context->Message_Block_Index++] =
				(*message_array & 0xFF);

		context->Length_Low += 8;

		if (context->Length_Low == 0) {
			context->Length_High++;

			if (context->Length_High == 0) {
				/* Message is too long */
				context->Corrupted = 1;
			}
		}

		if (context->Message_Block_Index == 64) {
			SHA1ProcessMessageBlock(context);
		}

		message_array++;
	}

	return shaSuccess;
}

static int
SHA1Result( SHA1Context *context, uint8_t Message_Digest[SHA1HashSize])
{
	int i;

	if (! context || ! Message_Digest) {
		return shaNull;
	}

	if (context->Corrupted) {
		return context->Corrupted;
	}

	if (! context->Computed) {
		SHA1PadMessage(context);

		for (i = 0; i < 64; ++i) {
			context->Message_Block[i] = 0;
		}

		context->Length_Low = 0;
		context->Length_High = 0;
		context->Computed = 1;
	}

	for (i = 0; i < SHA1HashSize; ++i) {
		Message_Digest[i] =
				context->Intermediate_Hash[i >> 2] >> 8 * (3 - (i & 0x03));
	}

	return shaSuccess;
}

static void
SHA1ProcessMessageBlock(SHA1Context *context)
{
	const uint32_t K[] = {
			0x5A827999,
			0x6ED9EBA1,
			0x8F1BBCDC,
			0xCA62C1D6
	};

	int t;
	uint32_t temp;
	uint32_t W[80];
	uint32_t A, B, C, D, E;

	for (t = 0; t < 16; t++) {
		W[t] = (uint32_t)(context->Message_Block[t * 4]) << 24;
		W[t] |= (uint32_t)(context->Message_Block[t * 4 + 1]) << 16;
		W[t] |= context->Message_Block[t * 4 + 2] << 8;
		W[t] |= context->Message_Block[t * 4 + 3];
	}

	for (t = 16; t < 80; t++) {
		W[t] = SHA1CircularShift(1,
				W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
	}

	A = context->Intermediate_Hash[0];
	B = context->Intermediate_Hash[1];
	C = context->Intermediate_Hash[2];
	D = context->Intermediate_Hash[3];
	E = context->Intermediate_Hash[4];

	for (t = 0; t < 20; t++) {
		temp =  SHA1CircularShift(5, A) +
				((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (t = 20; t < 40; t++) {
		temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (t = 40; t < 60; t++) {
		temp = SHA1CircularShift(5, A) +
				((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (t = 60; t < 80; t++) {
		temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	context->Intermediate_Hash[0] += A;
	context->Intermediate_Hash[1] += B;
	context->Intermediate_Hash[2] += C;
	context->Intermediate_Hash[3] += D;
	context->Intermediate_Hash[4] += E;

	context->Message_Block_Index = 0;
}

static void
SHA1PadMessage(SHA1Context *context)
{
	if (context->Message_Block_Index > 55) {
		context->Message_Block[context->Message_Block_Index++] = 0x80;

		while(context->Message_Block_Index < 64) {
			context->Message_Block[context->Message_Block_Index++] = 0;
		}

		SHA1ProcessMessageBlock(context);

		while(context->Message_Block_Index < 56) {
			context->Message_Block[context->Message_Block_Index++] = 0;
		}
	}
	else {
		context->Message_Block[context->Message_Block_Index++] = 0x80;

		while(context->Message_Block_Index < 56) {
			context->Message_Block[context->Message_Block_Index++] = 0;
		}
	}

	context->Message_Block[56] = context->Length_High >> 24;
	context->Message_Block[57] = context->Length_High >> 16;
	context->Message_Block[58] = context->Length_High >> 8;
	context->Message_Block[59] = context->Length_High;
	context->Message_Block[60] = context->Length_Low >> 24;
	context->Message_Block[61] = context->Length_Low >> 16;
	context->Message_Block[62] = context->Length_Low >> 8;
	context->Message_Block[63] = context->Length_Low;

	SHA1ProcessMessageBlock(context);
}



/*
 * Copy of RIPEMD160 API and implementation.
 *
 * For FIPS 140 compliance, this is NOT to be used for crypto purposes!
 */

//==========================================================
// Typedefs & constants.
//


//==========================================================
// Forward declarations.
//


//==========================================================
// Inlines & macros.
//


//==========================================================
// Public API.
//


//==========================================================
// Local helpers.
//
