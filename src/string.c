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

void
cf_itoa(int _value, char *_s, int _radix)
{
	if (_value < 0) {
		*_s++ = '-';
		_value = - _value;
	}
	int _v = _value;
	int _nd = 0;
	while (_v) {
		_nd++;
		_v /= _radix;
	}
	
	_s[_nd] = 0;
	while (_nd) {
		_nd --;
		_s[_nd ] = itoa_table [ _value % _radix ];
		_value = _value / _radix;
	}
	
}

void
cf_itoa_u64(uint64_t _value, char *_s, int _radix)
{
	uint64_t _v = _value;
	int _nd = 0;
	while (_v) {
		_nd++;
		_v /= _radix;
	}
	
	_s[_nd] = 0;
	while (_nd) {
		_nd --;
		_s[_nd ] = itoa_table [ _value % _radix ];
		_value = _value / _radix;
	}
	
}


