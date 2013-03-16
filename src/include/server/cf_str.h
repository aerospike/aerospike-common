/*
 *  Citrusleaf Foundation
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include "../cf_types.h"

// These functions copy integers into a buffer, and return the number of bytes copied.
unsigned int cf_str_itoa(int value, char *s, int radix);
unsigned int cf_str_itoa_u64(uint64_t value, char *s, int radix);
unsigned int cf_str_itoa_u32(uint32_t value, char *s, int radix);

// These functions convert a string to a number of different types
// returns 0 on success
int cf_str_atoi(char *s, int *value);
int cf_str_atoi_u32(char *s, uint32_t *value);
int cf_str_atoi_64(char *s, int64_t *value);
int cf_str_atoi_u64(char *s, uint64_t *value);

// and this does it also with radix
int cf_str_atoi_u64_x(char *s, uint64_t *value, int radix);




// Split the string 'str' based on input breaks in fmt
// The splitting is destructive
// The pointers will be added to the end of vector *v
// the vector better be created with object size 'void *'
struct cf_vector_s;
extern void cf_str_split(char *fmt, char *str, struct cf_vector_s *v);

static inline int cf_str_strnchr(uint8_t *s, int sz, int c) {
	for (int i=0;i<sz;i++) {
		if (s[i] == c) return(i);
	}
	return(-1);
}
