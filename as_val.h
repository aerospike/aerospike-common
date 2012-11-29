#pragma once

#include <stdlib.h>
#include <inttypes.h>

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
    as_val_t type;
    int (*free)(as_val * v);
    uint32_t (*hash)(as_val * v);
    char * (*tostring)(as_val * v);
};

#define as_val_free(v) \
    (v != NULL && ((as_val *)v)->free != NULL ? ((as_val *)v)->free((as_val *)v) : 1)

#define as_val_type(v) \
    (v != NULL && ((as_val *)v)->free != NULL ? ((as_val *)v)->type : AS_UNKNOWN)

#define as_val_hash(v) \
    (v != NULL && ((as_val *)v)->hash != NULL ? ((as_val *)v)->hash((as_val *)v) : 0)

#define as_val_tostring(v) \
    (v != NULL && ((as_val *)v)->tostring != NULL ? ((as_val *)v)->tostring((as_val *)v) : NULL)
