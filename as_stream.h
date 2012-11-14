#ifndef _AS_STREAM_H
#define _AS_STREAM_H

#include "as_iterator.h"

#define AS_STREAM_END ((void *) 0)

typedef struct as_stream_s as_stream;
typedef struct as_stream_hooks_s as_stream_hooks;

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
    const as_val * (*read)(const as_stream *);
    const int (*free)(as_stream *);
};

/**
 * Creates a new stream for a given source and hooks.
 *
 * @param source the source feeding the stream
 * @param hooks the hooks that interface with the source
 */
as_stream * as_stream_create(void *, const as_stream_hooks *);

/**
 * Get the source for the stream
 *
 * @param stream to get the source from
 * @return pointer to the source of the stream
 */
void * as_stream_source(const as_stream *);

/**
 * Reads an element from the stream
 *
 * Proxies to `s->hooks->read(s)`
 *
 * @param s the read to be read.
 * @return the element read from the stream or STREAM_END
 */
const as_val * as_stream_read(const as_stream *);

/**
 * Creates an iterator from the stream
 *
 * @param s the stream to create an iterator from
 * @return a new iterator
 */
as_iterator * as_stream_iterator(as_stream *);

/**
 * Frees the stream
 *
 * Proxies to `s->hooks->free(s)`
 *
 * @param s the stream to free
 * @return 0 on success, otherwise 1.
 */
const int as_stream_free(as_stream *);


#endif // _AS_STREAM_H