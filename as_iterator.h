#pragma once

#include "as_util.h"
#include "as_val.h"
#include <stdbool.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

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

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Creates a new iterator for a given source and hooks.
 *
 * @param source the source feeding the iterator
 * @param hooks the hooks that interface with the source
 */
as_iterator * as_iterator_new(const void *, const as_iterator_hooks *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_iterator_init(as_iterator * i, const void * source, const as_iterator_hooks * hooks) {
    i->source = source;
    i->hooks = hooks;
    return 0;
}

/**
 * Get the source for the iterator
 *
 * @param iterator to get the source from
 * @return pointer to the source of the iterator
 */
inline const void * as_iterator_source(const as_iterator * i) {
    return i->source;
}

/**
 * Frees the iterator and associated data, including the source and hooks.
 *
 * Proxies to `i->hooks->free(i)`
 *
 * @param i the iterator to free
 * @return 0 on success, otherwise 1.
 */
inline const int as_iterator_free(as_iterator * i) {
  return as_util_hook(free, 1, i);
}

/**
 * Tests if there are more values available in the iterator.
 *
 * Proxies to `i->hooks->has_next(i)`
 *
 * @param i the iterator to be tested.
 * @return true if there are more values, otherwise false.
 */
inline const bool as_iterator_has_next(const as_iterator * i) {
  return as_util_hook(has_next, false, i);
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
inline const as_val * as_iterator_next(as_iterator * i) {
  return as_util_hook(next, NULL, i);
}