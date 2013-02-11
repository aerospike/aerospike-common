#include "as_iterator.h"
#include <stdlib.h>
#include "as_internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/
extern inline bool as_iterator_has_next(const as_iterator * i) ;
extern inline const as_val * as_iterator_next(as_iterator * i) ;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void as_iterator_destroy(as_iterator * i) {
    as_util_hook(destroy, 1, i);
    if (i->is_malloc) free(i);
}

