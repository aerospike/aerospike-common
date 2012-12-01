#include "as_iterator.h"
#include <stdlib.h>


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
