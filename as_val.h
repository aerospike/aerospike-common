#pragma once

#include <stdlib.h>
#include <inttypes.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef enum as_val_t as_val_t;
typedef struct as_val_s as_val;

enum as_val_t {
    AS_UNKNOWN      = 0,
    AS_NIL          = 1,
    AS_BOOLEAN      = 2,
    AS_INTEGER      = 3,
    AS_STRING       = 4,
    AS_LIST         = 5,
    AS_MAP          = 6,
    AS_REC          = 7,
    AS_PAIR         = 8
};

struct as_val_s {
    as_val_t type;                      // type identifier, specified in as_val_t
    size_t size;                        // type size, should be sizeof(type)
    int (*free)(as_val * v);            // free memory used by the value
    uint32_t (*hash)(as_val * v);       // hash value for the value
    char * (*tostring)(as_val * v);     // string value for the value
};

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

/**
 * Represents empty values. As in a value with no value.
 */
extern const as_val as_nil;

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define as_val_free(v) \
    (v && ((as_val *)v)->free ? ((as_val *)v)->free((as_val *)v) : 1)

#define as_val_type(v) \
    (v && ((as_val *)v)->type ? ((as_val *)v)->type : AS_UNKNOWN)

#define as_val_hash(v) \
    (v && ((as_val *)v)->hash ? ((as_val *)v)->hash((as_val *)v) : 0)

#define as_val_tostring(v) \
    (v && ((as_val *)v)->tostring ? ((as_val *)v)->tostring((as_val *)v) : NULL)

#define as_val_size(v) \
    (v ? ((as_val *)v)->size : sizeof(as_val))


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_val * as_val_ref(as_val *);

