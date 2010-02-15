/*
 *  Citrusleaf Foundation
 *  src/string.c - string helper functions
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cf.h"

static char itoa_table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N' }; 

unsigned int
cf_str_itoa(int _value, char *_s, int _radix)
{
	// special case is the easy way
	if (_value == 0) {
		_s[0] = itoa_table[0];
		_s[1] = 0;
		return(1);
	}
	
	// Account for negatives
	if (_value < 0) {
		*_s++ = '-';
		_value = - _value;
	}
	int _v = _value;
	unsigned int _nd = 0;
	while (_v) {
		_nd++;
		_v /= _radix;
	}

	unsigned int rv = _nd; 	
	_s[_nd] = 0;
	while (_nd) {
		_nd --;
		_s[_nd ] = itoa_table [ _value % _radix ];
		_value = _value / _radix;
	}
	return(rv);
}

unsigned int
cf_str_itoa_u64(uint64_t _value, char *_s, int _radix)
{
	// special case is the easy way
	if (_value == 0) {
		_s[0] = itoa_table[0];
		_s[1] = 0;
		return(1);
	}

	uint64_t _v = _value;
	unsigned int _nd = 0;
	while (_v) {
		_nd++;
		_v /= _radix;
	}
	
	unsigned int rv = _nd;
	_s[_nd] = 0;
	while (_nd) {
		_nd --;
		_s[_nd ] = itoa_table [ _value % _radix ];
		_value = _value / _radix;
	}
	return(rv);
}

unsigned int
cf_str_itoa_u32(uint32_t _value, char *_s, int _radix)
{
	// special case is the easy way
	if (_value == 0) {
		_s[0] = itoa_table[0];
		_s[1] = 0;
		return(1);
	}

	uint32_t _v = _value;
	unsigned int _nd = 0;
	while (_v) {
		_nd++;
		_v /= _radix;
	}
	
	unsigned int rv = _nd;
	_s[_nd] = 0;
	while (_nd) {
		_nd --;
		_s[_nd ] = itoa_table [ _value % _radix ];
		_value = _value / _radix;
	}
	return(rv);
}

#define ATOI_ILLEGAL -1


static int8_t atoi_table[] = {
/*          00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
/* 00 */	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
/* 10 */	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
/* 20 */	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
/* 30 */	 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  -1,  -1,  -1,  -1,  -1,  -1, 
/* 40 */    -1,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,    
/* 50 */    25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  -1,  -1,  -1,  -1,
/* 60 */    -1,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  
/* 70 */    25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  -1,  -1,  -1,  -1 };  


int
cf_str_atoi_u64_x(char *s, uint64_t *value, int radix)
{
    uint64_t i = 0;
    while (*s) {
    	if (*s < 0)	return(-1);
    	int8_t cv = atoi_table[(uint8_t)*s];
    	if (cv < 0 || cv >= radix) return(-1);
        i *= radix;
        i += cv;
        s++;
    }
    *value = i;
    return(0);
}



void
cf_str_split(char *fmt, char *str, cf_vector *v)
{
	char c;
	char *prev = str;
	while ((c = *str)) {
		for (uint j=0;fmt[j];j++) {
			if (fmt[j] == c) {
				*str = 0;
				cf_vector_append(v, &prev);
				prev = str+1;
				break;
			}
		}
		str++;
	}
	if (prev != str)
		cf_vector_append(v, &prev);
}


