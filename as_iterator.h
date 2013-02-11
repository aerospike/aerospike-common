#pragma once
#include <stdbool.h>

#include "as_internal.h"

#include "as_util.h"
#include "as_val.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_iterator_s as_iterator;
typedef struct as_iterator_hooks_s as_iterator_hooks;

/**
 * Iterator Interface
 * Provided functions that interface with the iterators.
 */
struct as_iterator_hooks_s {
    void      (* const destroy)(as_iterator *);
    bool      (* const has_next)(const as_iterator *);
    as_val *  (* const next)(as_iterator *);
};

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

/*
** initializes an iterator object on the stack
*/
 
as_iterator * as_iterator_init(as_iterator * i, void * source, const as_iterator_hooks * hooks);

/**
 * Creates a new iterator for a given source and hooks.
 * this is done with MALLOC and thus the iterator must be freed exactly once
 *
 * @param source the source feeding the iterator
 * @param hooks the hooks that interface with the source
 */
as_iterator * as_iterator_new(void * source, const as_iterator_hooks * hooks);

void as_iterator_destroy(as_iterator * i);

/**
 * Tests if there are more values available in the iterator.
 *
 * Proxies to `i->hooks->has_next(i)`
 *
 * @param i the iterator to be tested.
 * @return true if there are more values, otherwise false.
 */
inline bool as_iterator_has_next(const as_iterator * i) {
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