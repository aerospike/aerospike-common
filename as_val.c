#include "as_val.h"
#include <cf_alloc.h>
#include "internal.h"

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_val * as_val_ref(as_val * v) {
    return v != NULL && cf_rc_reserve(v) > 0 ? v : NULL;
}