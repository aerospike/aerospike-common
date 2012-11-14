#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "as_stream.h"
#include "as_iterator.h"

typedef struct as_stream_iterator_source_s as_stream_iterator_source;

static const bool as_stream_iterator_has_next(const as_iterator * i);
static const as_val * as_stream_iterator_next(as_iterator * i);
static const int as_stream_iterator_free(as_iterator * i);
static const as_iterator_hooks as_stream_iterator_hooks;

/**
 * Source for stream iterators
 */
struct as_stream_iterator_source_s {
    as_stream * stream;
    const as_val * next;
    bool done;
};


/**
 * Creates a new stream for a given source and hooks.
 *
 * @param source the source feeding the stream
 * @param hooks the hooks that interface with the source
 */
as_stream * as_stream_create(void * source, const as_stream_hooks * hooks) {
    as_stream * s = (as_stream *) malloc(sizeof(as_stream));
    s->source = source;
    s->hooks = hooks;
    return s;
}

/**
 * Get the source for the stream
 *
 * @param stream to get the source from
 * @return pointer to the source of the stream
 */
void * as_stream_source(const as_stream * s) {
    return s->source;
}
/**
 * Reads an element from the stream
 *
 * Proxies to `s->hooks->read(s)`
 *
 * @param s the read to be read.
 * @return the element read from the stream or STREAM_END
 */
const as_val * as_stream_read(const as_stream * s) {
    return s->hooks->read(s);
}

/**
 * Creates an iterator from the stream
 *
 * @param s the stream to create an iterator from
 * @return a new iterator
 */
as_iterator * as_stream_iterator(as_stream * s) {
    as_stream_iterator_source * source = (as_stream_iterator_source *) malloc(sizeof(as_stream_iterator_source));
    source->stream = s;
    source->next = NULL;
    source->done = false;
    return as_iterator_create(source, &as_stream_iterator_hooks);
}

/**
 * Frees the stream
 *
 * Proxies to `s->hooks->free(s)`
 *
 * @param s the stream to free
 * @return 0 on success, otherwise 1.
 */
const int as_stream_free(as_stream * s) {
    return s->hooks->free(s);
}


static const bool as_stream_iterator_has_next(const as_iterator * i) {
    as_stream_iterator_source * source = (as_stream_iterator_source *) as_iterator_source(i);
    if ( source->done ) return false;
    if ( source->next ) return true;
    
    const as_val * v = as_stream_read(source->stream);
    if ( v != AS_STREAM_END ) {
        source->next = v;
        return true;
    } 

    source->done = true;
    return false;
}

static const as_val * as_stream_iterator_next(as_iterator * i) {

    as_stream_iterator_source * source = (as_stream_iterator_source *) as_iterator_source(i);

    if ( source->done ) return AS_STREAM_END;

    if ( source->next ) {
        const as_val * next = source->next;
        source->next = NULL;
        return next;
    }

    return as_stream_read(source->stream);
}

static const int as_stream_iterator_free(as_iterator * i) {
    free(i);
    return 0;
}

static const as_iterator_hooks as_stream_iterator_hooks = {
    as_stream_iterator_has_next,
    as_stream_iterator_next,
    as_stream_iterator_free
};