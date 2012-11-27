#pragma once

#include "as_val.h"
#include <stdbool.h>

typedef struct as_iterator_s as_iterator;
typedef struct as_iterator_hooks_s as_iterator_hooks;


/**
 * Iterator Structure
 * Contains pointer to the source of the iterator and a pointer to the
 * hooks that interface with the source.
 *
 * @field source the source of the iterator
 * @field hooks the interface to the source
 */
struct as_iterator_s {
    const void * source;
    const as_iterator_hooks * hooks;
};

/**
 * Iterator Interface
 * Provided functions that interface with the iterators.
 */
struct as_iterator_hooks_s {
    const bool (*has_next)(const as_iterator *);
    const as_val * (*next)(as_iterator *);
    const int (*free)(as_iterator *);
};

/**
 * Creates a new iterator for a given source and hooks.
 *
 * @param source the source feeding the iterator
 * @param hooks the hooks that interface with the source
 */
as_iterator * as_iterator_new(const void *, const as_iterator_hooks *);

/**
 * Get the source for the iterator
 *
 * @param iterator to get the source from
 * @return pointer to the source of the iterator
 */
const void * as_iterator_source(const as_iterator *);

/**
 * Tests if there are more values available in the iterator.
 *
 * Proxies to `i->hooks->has_next(i)`
 *
 * @param i the iterator to be tested.
 * @return true if there are more values, otherwise false.
 */
const bool as_iterator_has_next(const as_iterator *);

/**
 * Attempts to get the next value from the iterator.
 * This will return the next value, and iterate past the value.
 *
 * Proxies to `i->hooks->next(i)`
 *
 * @param i the iterator to get the next value from.
 * @return the next value available in the iterator.
 */
const as_val * as_iterator_next(as_iterator *);

/**
 * Frees the iterator and associated data, including the source and hooks.
 *
 * Proxies to `i->hooks->free(i)`
 *
 * @param i the iterator to free
 * @return 0 on success, otherwise 1.
 */
const int as_iterator_free(as_iterator *);
