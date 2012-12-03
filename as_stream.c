#include "as_util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "as_stream.h"
#include "as_iterator.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_stream_iterator_source_s as_stream_iterator_source;

/**
 * Source for stream iterators
 */
struct as_stream_iterator_source_s {
    as_stream * stream;
    const as_val * next;
    bool done;
};

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_stream_init(as_stream *, void *, const as_stream_hooks *);
extern inline int as_stream_destroy(as_stream *);

extern inline as_stream * as_stream_new(void *, const as_stream_hooks *);
extern inline int as_stream_free(as_stream *);

extern inline void * as_stream_source(const as_stream *);

extern inline const as_val * as_stream_read(const as_stream *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static const bool as_stream_iterator_has_next(const as_iterator *);
static const as_val * as_stream_iterator_next(as_iterator *);
static const int as_stream_iterator_free(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_iterator_hooks as_stream_iterator_hooks = {
    as_stream_iterator_has_next,
    as_stream_iterator_next,
    as_stream_iterator_free
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
as_iterator * as_stream_iterator(as_stream * s) {
    as_stream_iterator_source * source = (as_stream_iterator_source *) malloc(sizeof(as_stream_iterator_source));
    source->stream = s;
    source->next = NULL;
    source->done = false;
    return as_iterator_new(source, &as_stream_iterator_hooks);
}

static const int as_stream_iterator_free(as_iterator * i) {
    free(i->source);
    i->source = NULL;
    i->hooks = NULL;
    return 0;
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