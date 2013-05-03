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
#include <string.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_string_s as_string;

struct as_string_s {
    as_val _;
    bool value_is_malloc; 
    char * value;
    size_t len;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_string *    as_string_init(as_string *, char *s, bool is_malloc);
as_string *   as_string_new(char *, bool is_malloc);

void           as_string_destroy(as_string *);
void    		as_string_val_destroy(as_val *v);

uint32_t as_string_val_hash(const as_val *);
char *as_string_val_tostring(const as_val *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline char * as_string_tostring(const as_string * s) {
    return (s->value);
}

size_t as_string_len(as_string * s);

uint32_t as_string_hash(const as_string * s);

inline as_val * as_string_toval(const as_string * s) {
    return (as_val *)s;
}

inline as_string * as_string_fromval(const as_val * v) {
    return as_util_fromval(v, AS_STRING, as_string);
}
