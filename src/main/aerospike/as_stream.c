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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <aerospike/as_util.h>
#include <aerospike/as_stream.h>
#include <aerospike/as_iterator.h>

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

extern inline as_stream * as_stream_init(as_stream *, void *, const as_stream_hooks *);

extern inline void as_stream_destroy(as_stream *);

extern inline as_stream * as_stream_new(void *, const as_stream_hooks *);

extern inline void * as_stream_source(const as_stream *);

extern inline as_val * as_stream_read(const as_stream *);

extern inline bool as_stream_readable(const as_stream *);

extern inline as_stream_status as_stream_write(const as_stream *, as_val * v);

extern inline bool as_stream_writable(const as_stream *);
