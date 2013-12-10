/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <citrusleaf/alloc.h>
#include <aerospike/as_integer.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void          as_integer_destroy(as_integer * i);

extern inline int64_t       as_integer_toint(const as_integer * i);

extern inline as_val *      as_integer_toval(const as_integer * i);
extern inline as_integer *  as_integer_fromval(const as_val * v);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_integer * as_integer_init(as_integer * v, int64_t i) {
	as_val_init(&v->_, AS_INTEGER, false);
    v->value = i;
    return v;
}

as_integer * as_integer_new(int64_t i) {
    as_integer * v = (as_integer *) malloc(sizeof(as_integer));
    if (!v) return v;
    as_val_init(&v->_, AS_INTEGER, true /*is_malloc*/);
    v->value = i;
    return v;
}



void as_integer_val_destroy(as_val * v) {
    return;
}

uint32_t as_integer_val_hashcode(const as_val * v) {
    as_integer * i = as_integer_fromval(v);
    return i != NULL ? i->value : 0;
}

char * as_integer_val_tostring(const as_val * v) {
    as_integer * i = (as_integer *) v;
    if (!i) return NULL;
    char * str = (char *) malloc(sizeof(char) * 32);
    bzero(str, 32);
    sprintf(str,"%ld",i->value);
    return str;
}
