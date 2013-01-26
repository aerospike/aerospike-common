#include "as_val.h"
#include <string.h>
#include "internal.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_nil_val_free(as_val *);
static uint32_t as_nil_val_hash(as_val *);
static char * as_nil_val_tostring(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

/**
 * Represents empty values. As in a value with no value.
 */
const as_val as_nil = {
    .type       = AS_NIL, 
    .size       = 0,
    .free       = as_nil_val_free,
    .hash       = as_nil_val_hash,
    .tostring   = as_nil_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static int as_nil_val_free(as_val * v) {
    return 0;
}

static uint32_t as_nil_val_hash(as_val * v) {
    return 0;
}

static char * as_nil_val_tostring(as_val * v) {
    return strdup("NIL");
}