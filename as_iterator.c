#include "as_iterator.h"
#include <stdlib.h>
#include "internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_iterator * as_iterator_new(void *, const as_iterator_hooks *);
extern inline int as_iterator_free(as_iterator *);

extern inline as_iterator * as_iterator_init(as_iterator *, void *, const as_iterator_hooks *);
extern inline int as_iterator_destroy(as_iterator *);

extern inline const void * as_iterator_source(const as_iterator *);

extern inline const bool as_iterator_has_next(const as_iterator *);
extern inline const as_val * as_iterator_next(as_iterator *);
