#pragma once

#include "as_iterator.h"

#define AS_STREAM_END ((void *) 0)

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_stream_s as_stream;
typedef enum as_stream_status_e as_stream_status;
typedef struct as_stream_hooks_s as_stream_hooks;

/**
 * Stream Status Codes
 */
enum as_stream_status_e {
    AS_STREAM_OK = 0,
    AS_STREAM_ERR = 1
};

/**
 * Stream Structure
 * Contains pointer to the source of the stream and a pointer to the
 * hooks that interface with the source.
 *
 * @field source the source of the stream
 * @field hooks the interface to the source
 */
struct as_stream_s {
    void * source;
    const as_stream_hooks * hooks;
};

/**
 * Stream Interface
 * Provided functions that interface with the streams.
 */
struct as_stream_hooks_s {
    int (*destroy)(as_stream *);
    as_val * (*read)(const as_stream *);
    as_stream_status * (*write)(const as_stream *, const as_val *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Creates an iterator from the stream
 *
 * @param s the stream to create an iterator from
 * @return a new iterator
 */
as_iterator * as_stream_iterator_new(as_stream *);

as_iterator * as_stream_iterator_init(as_stream *, as_iterator *i);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_stream_init(as_stream * s, void * source, const as_stream_hooks * hooks) {
    s->source = source;
    s->hooks = hooks;
    return 0;
}

/**
 * Creates a new stream for a given source and hooks.
 *
 * @param source the source feeding the stream
 * @param hooks the hooks that interface with the source
 */
inline as_stream * as_stream_new(void * source, const as_stream_hooks * hooks) {
    as_stream * s = (as_stream *) malloc(sizeof(as_stream));
    as_stream_init(s, source, hooks);
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
 * Write a value to the stream
 *
 * Proxies to `s->hooks->write(s,v)`
 *
 * @param s the stream to write to.
 * @param v the element to write to the stream.
 * @return AS_STREAM_OK on success, otherwise is failure.
 */
inline as_stream_status * as_stream_write(const as_stream * s, const as_val * v) {
    return as_util_hook(write, AS_STREAM_OK, s, v);
}


