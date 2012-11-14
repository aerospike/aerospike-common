#include "as_iterator.h"
#include <stdlib.h>


/**
 * Creates a new iterator for a given source and hooks.
 *
 * @param source the source feeding the iterator
 * @param hooks the hooks that interface with the source
 */
as_iterator * as_iterator_create(const void * source, const as_iterator_hooks * hooks) {
    as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
    i->source = source;
    i->hooks = hooks;
    return i;
}

/**
 * Get the source for the iterator
 *
 * @param iterator to get the source from
 * @return pointer to the source of the iterator
 */
const void * as_iterator_source(const as_iterator * i) {
    return i->source;
}

/**
 * Tests if there are more values available in the iterator.
 *
 * Proxies to `i->hooks->has_next(i)`
 *
 * @param i the iterator to be tested.
 * @return true if there are more values, otherwise false.
 */
const bool as_iterator_has_next(const as_iterator * i) {
  return i->hooks->has_next(i);
}

/**
 * Attempts to get the next value from the iterator.
 * This will return the next value, and iterate past the value.
 *
 * Proxies to `i->hooks->next(i)`
 *
 * @param i the iterator to get the next value from.
 * @return the next value available in the iterator.
 */
const as_val * as_iterator_next(as_iterator * i) {
  return i->hooks->next(i);
}

/**
 * Frees the iterator and associated data, including the source and hooks.
 *
 * Proxies to `i->hooks->free(i)`
 *
 * @param i the iterator to free
 * @return 0 on success, otherwise 1.
 */
const int as_iterator_free(as_iterator * i) {
  return i->hooks->free(i);
}
