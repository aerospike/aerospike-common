/* 
 * Copyright 2008-2016 Aerospike, Inc.
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

#include "citrusleaf/cf_b64.h"

#include <stdbool.h>
#include <stdint.h>


/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

static const char base64_chars[] = { 
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static const bool base64_valid_a[] = {
		/*00*/ /*01*/ /*02*/ /*03*/ /*04*/ /*05*/ /*06*/ /*07*/   /*08*/ /*09*/ /*0A*/ /*0B*/ /*0C*/ /*0D*/ /*0E*/ /*0F*/
/*00*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*10*/  false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*20*/	false, false, false, false, false, false, false, false,   false, false, false,  true, false, false, false,  true,
/*30*/	 true,  true,  true,  true,  true,  true,  true,  true,    true,  true, false, false, false, false, false, false,
/*40*/	false,  true,  true,  true,  true,  true,  true,  true,    true,  true,  true,  true,  true,  true,  true,  true,
/*50*/	 true,  true,  true,  true,  true,  true,  true,  true,    true,  true,  true, false, false, false, false, false,
/*60*/	false,  true,  true,  true,  true,  true,  true,  true,    true,  true,  true,  true,  true,  true,  true,  true,
/*70*/	 true,  true,  true,  true,  true,  true,  true,  true,    true,  true,  true, false, false, false, false, false,
/*80*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*90*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*A0*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*B0*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*C0*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*D0*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*E0*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false,
/*F0*/	false, false, false, false, false, false, false, false,   false, false, false, false, false, false, false, false
};

static const uint8_t base64_decode_a[] = {
		/*00*/ /*01*/ /*02*/ /*03*/ /*04*/ /*05*/ /*06*/ /*07*/   /*08*/ /*09*/ /*0A*/ /*0B*/ /*0C*/ /*0D*/ /*0E*/ /*0F*/
/*00*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*10*/      0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*20*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,    62,     0,     0,     0,    63,
/*30*/	   52,    53,    54,    55,    56,    57,    58,    59,      60,    61,     0,     0,     0,     0,     0,     0,
/*40*/	    0,     0,     1,     2,     3,     4,     5,     6,       7,     8,     9,    10,    11,    12,    13,    14,
/*50*/	   15,    16,    17,    18,    19,    20,    21,    22,      23,    24,    25,     0,     0,     0,     0,     0,
/*60*/	    0,    26,    27,    28,    29,    30,    31,    32,      33,    34,    35,    36,    37,    38,    39,    40,
/*70*/	   41,    42,    43,    44,    45,    46,    47,    48,      49,    50,    51,     0,     0,     0,     0,     0,
/*80*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*90*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*A0*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*B0*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*C0*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*D0*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*E0*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0,
/*F0*/	    0,     0,     0,     0,     0,     0,     0,     0,       0,     0,     0,     0,     0,     0,     0,     0
};

// Shortcuts:
#define EA base64_chars
#define VA base64_valid_a
#define DA base64_decode_a


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

// Must have allocated big enough 'out' - e.g. use cf_b64_encoded_len(in_size).
void
cf_b64_encode(const uint8_t* in, uint32_t in_size, char* out)
{
	int i = 0;
	int j = 0;

	while (in_size >= 3) {
		uint8_t i0 = in[i];
		uint8_t i1 = in[i + 1];
		uint8_t i2 = in[i + 2];

		out[j]     = EA[ (i0 & 0xfc) >> 2];
		out[j + 1] = EA[((i0 & 0x03) << 4) + ((i1 & 0xf0) >> 4)];
		out[j + 2] = EA[((i1 & 0x0f) << 2) + ((i2 & 0xc0) >> 6)];
		out[j + 3] = EA[  i2 & 0x3f];

		i += 3;
		j += 4;

		in_size -= 3;
	}

	if (in_size == 1) {
		uint8_t i0 = in[i];

		out[j]     = EA[(i0 & 0xfc) >> 2];
		out[j + 1] = EA[(i0 & 0x03) << 4];
		out[j + 2] = '=';
		out[j + 3] = '=';
	}
	else if (in_size == 2) {
		uint8_t i0 = in[i];
		uint8_t i1 = in[i + 1];

		out[j]     = EA[ (i0 & 0xfc) >> 2];
		out[j + 1] = EA[((i0 & 0x03) << 4) + ((i1 & 0xf0) >> 4)];
		out[j + 2] = EA[ (i1 & 0x0f) << 2];
		out[j + 3] = '=';
	}
}

// Must have allocated big enough 'out' - e.g. use cf_b64_decoded_buf_size().
// Note that 'in_len' must be a padded encoded size - an exact multiple of 4.
// 'out_size' returns the decoded size after padding has been accounted for -
// i.e. it may be 1 or 2 less than the 'out' buffer size. Also, 'in_len' = 0 is
// handled, and gives 'out_size' = 0.
void
cf_b64_decode(const char* in, uint32_t in_len, uint8_t* out, uint32_t* out_size)
{
	uint32_t i = 0;
	uint32_t j = 0;

	while (i < in_len) {
		uint8_t i0 = (uint8_t)in[i];
		uint8_t i1 = (uint8_t)in[i + 1];
		uint8_t i2 = (uint8_t)in[i + 2];
		uint8_t i3 = (uint8_t)in[i + 3];

		out[j]     = (DA[i0] << 2) | (DA[i1] >> 4);
		out[j + 1] = (DA[i1] << 4) | (DA[i2] >> 2);
		out[j + 2] = (DA[i2] << 6) |  DA[i3];

		i += 4;
		j += 3;
	}

	if (out_size) {
		if (i != 0) {
			if (in[i - 1] == '=') {
				j--;
			}

			if (in[i - 2] == '=') {
				j--;
			}
		}

		*out_size = j;
	}
}

// Same as cf_b64_decode() but 'in' and 'out' are the same buffer.
void
cf_b64_decode_in_place(uint8_t* in_out, uint32_t in_len, uint32_t* out_size)
{
	uint32_t i = 0;
	uint32_t j = 0;

	uint32_t d = 0;

	if (out_size && in_len != 0) {
		if (in_out[in_len - 1] == '=') {
			d++;
		}

		if (in_out[in_len - 2] == '=') {
			d++;
		}
	}

	while (i < in_len) {
		uint8_t i0 = in_out[i];
		uint8_t i1 = in_out[i + 1];
		uint8_t i2 = in_out[i + 2];
		uint8_t i3 = in_out[i + 3];

		uint8_t b0 = (DA[i0] << 2) | (DA[i1] >> 4);
		uint8_t b1 = (DA[i1] << 4) | (DA[i2] >> 2);
		uint8_t b2 = (DA[i2] << 6) |  DA[i3];

		in_out[j]     = b0;
		in_out[j + 1] = b1;
		in_out[j + 2] = b2;

		i += 4;
		j += 3;
	}

	if (out_size) {
		*out_size = j - d;
	}
}

// Make sure the length is ok and all characters are valid base64 characters,
// including any '=' padding at the end. Note that 'in_len' = 0 is considered
// invalid.
static bool
is_valid_encoded(const char* in, uint32_t in_len)
{
	if (! in || in_len == 0 || (in_len & 3) != 0) {
		return false;
	}

	const uint8_t* end = (const uint8_t*)in + in_len - 2;
	const uint8_t* read = (const uint8_t*)in;

	while (read < end) {
		if (! VA[*read++]) {
			return false;
		}
	}

	if (*read == '=') {
		return *(read + 1) == '=';
	}

	if (! VA[*read++]) {
		return false;
	}

	return *read == '=' || VA[*read];
}

// Same as cf_b64_decode() but validates input as ok to decode.
bool
cf_b64_validate_and_decode(const char* in, uint32_t in_len, uint8_t* out,
		uint32_t* out_size)
{
	if (! is_valid_encoded(in, in_len)) {
		return false;
	}

	cf_b64_decode(in, in_len, out, out_size);

	return true;
}

// Same as cf_b64_decode_in_place() but validates input as ok to decode.
bool
cf_b64_validate_and_decode_in_place(uint8_t* in_out, uint32_t in_len,
		uint32_t* out_size)
{
	if (! is_valid_encoded((const char*)in_out, in_len)) {
		return false;
	}

	cf_b64_decode_in_place(in_out, in_len, out_size);

	return true;
}
