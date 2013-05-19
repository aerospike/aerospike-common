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
#pragma once

#include <sys/types.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define pair_new(a,b) as_pair_new((as_val *) a, (as_val *) b)

/******************************************************************************
 * TYPES
 ******************************************************************************/

struct as_pair_s {
    as_val      _;
    as_val *    _1;
    as_val *    _2;
};

typedef struct as_pair_s as_pair;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_pair *   as_pair_new(as_val *, as_val *);
as_pair *   as_pair_init(as_pair *, as_val *, as_val *);

void        as_pair_val_destroy(as_val *);
uint32_t    as_pair_val_hashcode(const as_val *);
char *	    as_pair_val_tostring(const as_val *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline void as_pair_destroy(as_pair * p) {
    as_val_val_destroy((as_val *)p);
}



inline as_val * as_pair_1(as_pair * p) {
    return p->_1;
}

inline as_val * as_pair_2(as_pair * p) {
    return p->_2;
}



inline as_val * as_pair_toval(const as_pair * p) {
    return (as_val *)p;
}

inline as_pair * as_pair_fromval(const as_val * v) {
    return as_util_fromval(v, AS_PAIR, as_pair);
}