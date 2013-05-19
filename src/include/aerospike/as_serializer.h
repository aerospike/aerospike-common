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

#include <inttypes.h>

#include <aerospike/as_util.h>
#include <aerospike/as_types.h>
#include <aerospike/as_buffer.h>


/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_serializer_hooks_s;

/**
 * Serializer Data
 */
union as_serializer_data_u {
    void * generic;
};

typedef union as_serializer_data_u as_serializer_data;

/**
 * Serializer Object
 */
struct as_serializer_s {
    bool                                    is_malloc;
    as_serializer_data                      data;
    const struct as_serializer_hooks_s *    hooks;
};

typedef struct as_serializer_s as_serializer;

/**
 * Serializer Function Hooks
 */
struct as_serializer_hooks_s {
    void    (* destroy)(as_serializer *);
    int     (* serialize)(as_serializer *, as_val *, as_buffer *);
    int     (* deserialize)(as_serializer *, as_buffer *, as_val **);
};

typedef struct as_serializer_hooks_s as_serializer_hooks;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_serializer * as_serializer_init(as_serializer *, const void *source, const as_serializer_hooks *);
as_serializer * as_serializer_new(const void *source, const as_serializer_hooks *);

void as_serializer_destroy(as_serializer *);

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

inline int as_serializer_serialize(as_serializer * s, as_val * v, as_buffer * b) {
    return as_util_hook(serialize, 1, s, v, b);
}

inline int as_serializer_deserialize(as_serializer * s, as_buffer * b, as_val ** v) {
    return as_util_hook(deserialize, 1, s, b, v);
}
