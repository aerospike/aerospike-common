#include "as_iterator.h"
#include <stdlib.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_iterator_init(as_iterator *, const void *, const as_iterator_hooks *);
extern inline const void * as_iterator_source(const as_iterator *);
extern inline const int as_iterator_free(as_iterator *);
extern inline const bool as_iterator_has_next(const as_iterator *);
extern inline const as_val * as_iterator_next(as_iterator *);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Creates a new iterator for a given source and hooks.
 *
 * @param source the source feeding the iterator
 * @param hooks the hooks that interface with the source
 */
as_iterator * as_iterator_new(const void * source, const as_iterator_hooks * hooks) {
    as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
    i->source = source;
    i->hooks = hooks;
    return i;
}
