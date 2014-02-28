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

#include <citrusleaf/alloc.h>

#include <aerospike/as_serializer.h>

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

extern inline int as_serializer_serialize(as_serializer * serializer, as_val * val, as_buffer * buffer);

extern inline int as_serializer_deserialize(as_serializer * serializer, as_buffer * buffer, as_val ** val);

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_serializer * as_serializer_cons(as_serializer * serializer, bool free, void * data, const as_serializer_hooks * hooks) 
{
	if ( !serializer ) return serializer;

    serializer->free = false;
    serializer->data = data;
    serializer->hooks = hooks;
    return serializer;
}

as_serializer * as_serializer_init(as_serializer * serializer, void * data, const as_serializer_hooks * hooks) 
{
    return as_serializer_cons(serializer, false, data, hooks);
}

as_serializer * as_serializer_new(void * data, const as_serializer_hooks * hooks)
{
    as_serializer * serializer = (as_serializer *) cf_malloc(sizeof(as_serializer));
    return as_serializer_cons(serializer, true, data, hooks);
}

void as_serializer_destroy(as_serializer * serializer)
{
    if ( serializer->free ) {
    	cf_free(serializer);
    }
}


