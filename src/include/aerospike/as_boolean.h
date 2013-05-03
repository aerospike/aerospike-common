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

#include <stdbool.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_boolean_s as_boolean;

struct as_boolean_s {
    as_val _;
    bool value;
};


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_boolean *  as_boolean_init(as_boolean *, bool);
as_boolean *  as_boolean_new(bool);

void          as_boolean_destroy(as_boolean *);
void          as_boolean_val_destroy(as_val *);

uint32_t as_boolean_val_hash(const as_val * v);
char *as_boolean_val_tostring(const as_val *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/
 
inline bool as_boolean_tobool(const as_boolean * b) {
    return b->value;
}

inline uint32_t as_boolean_hash(const as_boolean * b) {
    return b->value ? 1 : 0;
}


inline as_val * as_boolean_toval(const as_boolean * b) {
    return (as_val *) b;
}

inline as_boolean * as_boolean_fromval(const as_val * v) {
    return as_util_fromval(v, AS_BOOLEAN, as_boolean);
}
