#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "as_util.h"
#include "as_stream.h"
#include "as_iterator.h"

#include "internal.h"

/******************************************************************************
 * TYPES
 *****************************************************************************/

typedef struct as_stream_iterator_source_s as_stream_iterator_source;

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
