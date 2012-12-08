/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

bool cf_base64_validate_input(const uint8_t *b, const int len);
int cf_base64_encode_maxlen(int len);
void cf_base64_encode(uint8_t * in_bytes, uint8_t *out_bytes, int *len_r);
void cf_base64_tostring(uint8_t * in_bytes, char *out_bytes, int *len_r);
int cf_base64_decode_inplace(uint8_t *bytes, int *len_r, bool validate);
int cf_base64_decode(uint8_t *in_bytes, uint8_t *out_bytes, int *len_r, bool validate);
int cf_base64_test();

