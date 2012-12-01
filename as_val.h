#pragma once

#include <stdlib.h>
#include <inttypes.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef enum as_val_t as_val_t;
typedef struct as_val_s as_val;

enum as_val_t {
    AS_UNKNOWN = 0,
    AS_EMPTY,
    AS_BOOLEAN,
    AS_INTEGER,
    AS_STRING,
    AS_LIST,
    AS_MAP,
    AS_REC,
    AS_PAIR
};

struct as_val_s {
    as_val_t type;                      // type identifier, specified in as_val_t
    size_t size;                        // type size, should be sizeof(type)
    int (*free)(as_val * v);            // free memory used by the value
    uint32_t (*hash)(as_val * v);       // hash value for the value
    char * (*tostring)(as_val * v);     // string value for the value
};

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define as_val_free(v) \
    (v && ((as_val *)v)->free ? ((as_val *)v)->free((as_val *)v) : 1)

#define as_val_type(v) \
    (v && ((as_val *)v)->free ? ((as_val *)v)->type : AS_UNKNOWN)

#define as_val_hash(v) \
    (v && ((as_val *)v)->hash ? ((as_val *)v)->hash((as_val *)v) : 0)

#define as_val_tostring(v) \
    (v && ((as_val *)v)->tostring ? ((as_val *)v)->tostring((as_val *)v) : NULL)

#define as_val_size(v) \
    (v ? ((as_val *)v)->size : sizeof(as_val))
