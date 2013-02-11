#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "as_internal.h"

#include "as_util.h"
#include "as_stream.h"
#include "as_iterator.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_stream_iterator_source_s as_stream_iterator_source;

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_stream_init(as_stream *, void *, const as_stream_hooks *);
extern inline int as_stream_destroy(as_stream *);

extern inline as_stream * as_stream_new(void *, const as_stream_hooks *);

extern inline void * as_stream_source(const as_stream *);

extern inline as_val * as_stream_read(const as_stream *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static void as_stream_iterator_destroy(as_iterator *);
static bool as_stream_iterator_has_next(const as_iterator *);
static as_val * as_stream_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_iterator_hooks as_stream_iterator_hooks = {
    .destroy    = as_stream_iterator_destroy,
    .has_next   = as_stream_iterator_has_next,
    .next       = as_stream_iterator_next
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
as_iterator * as_stream_iterator_init(as_stream * s, as_iterator *i) {
    i->is_malloc = false;
    i->hooks = &as_stream_iterator_hooks;
    as_stream_iterator_source * ss = (as_stream_iterator_source *) &(i->u.stream);
    ss->stream = s;
    ss->next = NULL;
    ss->done = false;
    return i;
}

as_iterator * as_stream_iterator_new(as_stream * s) {
    as_iterator *i = (as_iterator *) malloc(sizeof(as_iterator));
    i->is_malloc = false;
    i->hooks = &as_stream_iterator_hooks;
    as_stream_iterator_source * ss = (as_stream_iterator_source *) &(i->u.stream);
    ss->stream = s;
    ss->next = NULL;
    ss->done = false;
    return i;
}


static void as_stream_iterator_destroy(as_iterator * i) {
    if (i->is_malloc) free(i);
}

static bool as_stream_iterator_has_next(const as_iterator * i) {
    as_stream_iterator_source * ss = (as_stream_iterator_source *) &(i->u.stream);
    if ( ss->done ) return false;
    if ( ss->next ) return true;
    
    const as_val * v = as_stream_read(ss->stream);
    if ( v != AS_STREAM_END ) {
        ss->next = v;
        return true;
    } 

    ss->done = true;
    return false;
}

static as_val * as_stream_iterator_next(as_iterator * i) {

    as_stream_iterator_source * ss = (as_stream_iterator_source *) &(i->u.stream);

    if ( ss->done ) return AS_STREAM_END;

    if ( ss->next ) {
        as_val * next = ss->next;
        ss->next = NULL;
        return next;
    }

    return as_stream_read(ss->stream);
}