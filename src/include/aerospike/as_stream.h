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

#include <aerospike/as_iterator.h>

#define AS_STREAM_END ((void *) 0)

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_stream_s as_stream;
typedef struct as_stream_hooks_s as_stream_hooks;

/**
 * Stream Status Codes
 */
typedef enum as_stream_status_e {
    AS_STREAM_OK = 0,
    AS_STREAM_ERR = 1
} as_stream_status;

/**
 * Stream Structure
 * Contains pointer to the source of the stream and a pointer to the
 * hooks that interface with the source.
 *
 * @field source the source of the stream
 * @field hooks the interface to the source
 */
struct as_stream_s {
    bool is_malloc;
    void * source;
    const as_stream_hooks * hooks;
};

/**
 * Stream Interface
 * Provided functions that interface with the streams.
 */
struct as_stream_hooks_s {
    int                 (* destroy)(as_stream *);
    as_val *            (* read)(const as_stream *);
    as_stream_status    (* write)(const as_stream *, as_val *);
};

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline as_stream * as_stream_init(as_stream * s, void * source, const as_stream_hooks * hooks) {
    if ( s == NULL ) return s;
    s->is_malloc = false;
    s->source = source;
    s->hooks = hooks;
    return s;
}

/**
 * Creates a new stream for a given source and hooks.
 *
 * @param source the source feeding the stream
 * @param hooks the hooks that interface with the source
 */
inline as_stream * as_stream_new(void * source, const as_stream_hooks * hooks) {
    as_stream * s = (as_stream *) malloc(sizeof(as_stream));
    s->is_malloc = true;
    s->source = source;
    s->hooks = hooks;
    return s;
}

/**
 * Frees the stream
 *
 * Proxies to `s->hooks->free(s)`
 *
 * @param s the stream to free
 * @return 0 on success, otherwise 1.
 */
inline void as_stream_destroy(as_stream * s) {
    as_util_hook(destroy, 1, s);
    if ( s && s->is_malloc) free(s);
}
/**
 * Get the source for the stream
 *
 * @param stream to get the source from
 * @return pointer to the source of the stream
 */
inline void * as_stream_source(const as_stream * s) {
    return (s ? s->source : NULL);
}

/**
 * Reads a value from the stream
 *
 * Proxies to `s->hooks->read(s)`
 *
 * @param s the stream to be read.
 * @return the element read from the stream or STREAM_END
 */
inline as_val * as_stream_read(const as_stream * s) {
    return as_util_hook(read, NULL, s);
}

/**
 * Is the stream readable? Tests whether the stream has a read function.
 *
 * @param s the stream to test.
 * @return true if the stream can be read from
 */
inline bool as_stream_readable(const as_stream * s) {
    return s != NULL && s->hooks != NULL && s->hooks->read;
}

/**
 * Write a value to the stream
 *
 * Proxies to `s->hooks->write(s,v)`
 *
 * @param s the stream to write to.
 * @param v the element to write to the stream.
 * @return AS_STREAM_OK on success, otherwise is failure.
 */
inline as_stream_status as_stream_write(const as_stream * s, as_val * v) {
    return as_util_hook(write, AS_STREAM_ERR, s, v);
}


/**
 * Is the stream writable? Tests whether the stream has a write function.
 *
 * @param s the stream to test.
 * @return true if the stream can be written to.
 */
inline bool as_stream_writable(const as_stream * s) {
    return s != NULL && s->hooks != NULL && s->hooks->write;
}
