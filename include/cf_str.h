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

// Split the string 'str' based on input breaks in fmt
// The splitting is destructive
// The pointers will be added to the end of vector *v
// the vector better be created with object size 'void *'
struct cf_vector_s;
extern void cf_str_split(char *fmt, char *str, struct cf_vector_s *v);
