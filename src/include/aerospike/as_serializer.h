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

#include <aerospike/as_util.h>
#include <aerospike/as_types.h>
#include <aerospike/as_buffer.h>

#include <stdint.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_serializer_hooks_s;

/**
 * Serializer Object
 */
typedef struct as_serializer_s {

	/**
	 *	@private
	 *	If true, then as_serializer_destroy() will free this.
	 */
    bool free;

    /**
     *	Data for the serializer.
     */
    void * data;

    /**
     *	Hooks for the serializer
     */
    const struct as_serializer_hooks_s * hooks;
    
} as_serializer;

/**
 * Serializer Function Hooks
 */
typedef struct as_serializer_hooks_s {
    void    (* destroy)(as_serializer *);
    int     (* serialize)(as_serializer *, as_val *, as_buffer *);
    int     (* deserialize)(as_serializer *, as_buffer *, as_val **);
} as_serializer_hooks;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_serializer * as_serializer_cons(as_serializer * serializer, bool free, void * data, const as_serializer_hooks * hooks);

as_serializer * as_serializer_init(as_serializer * serializer, void * data, const as_serializer_hooks * hooks);

as_serializer * as_serializer_new(void * data, const as_serializer_hooks *);

void as_serializer_destroy(as_serializer *);

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

static inline int as_serializer_serialize(as_serializer * serializer, as_val * val, as_buffer * buffer)
{
    return as_util_hook(serialize, 1, serializer, val, buffer);
}

static inline int as_serializer_deserialize(as_serializer * serializer, as_buffer * buffer, as_val ** val) 
{
    return as_util_hook(deserialize, 1, serializer, buffer, val);
}
