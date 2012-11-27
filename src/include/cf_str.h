/*
 *  Citrusleaf Foundation
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once


#include "cf.h"

extern unsigned int cf_str_itoa(int value, char *s, int radix);
extern unsigned int cf_str_itoa_u64(uint64_t value, char *s, int radix);
extern unsigned int cf_str_itoa_u32(uint32_t value, char *s, int radix);

extern int cf_str_atoi_u64_x(char *s, uint64_t *value, int radix);

// return 0 on success, -1 on fail
static inline int cf_str_atoi(char *s, int *value)
{
    int i = 0;
    bool neg = false;
    
    if (*s == '-') { neg = true; s++; }
    
    while (*s >= '0' && *s <= '9') {
        i *= 10;
        i += *s - '0';
        s++;
    }
    if (*s != 0)	return(-1); // reached a non-num before EOL
    *value = neg ? -i : i;
    return(0);
}

static inline int cf_str_atoi_u64(char *s, uint64_t *value)
{
    uint64_t i = 0;
    while (*s >= '0' && *s <= '9') {
        i *= 10;
        i += *s - '0';
        s++;
    }
    if (*s != 0)	return(-1);
    *value = i;
    return(0);
}


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
