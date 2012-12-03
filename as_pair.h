#pragma once

#include "as_util.h"
#include "as_val.h"
#include <sys/types.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_pair_s as_pair;

struct as_pair_s {
    as_val _;
    as_val * _1;
    as_val * _2;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_pair_init(as_pair *, as_val *, as_val *);
int as_pair_destroy(as_pair *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline as_pair * as_pair_new(as_val * _1, as_val * _2) {
    as_pair * p = (as_pair *) malloc(sizeof(as_pair));
    as_pair_init(p, _1, _2);
    return p;
}

inline int as_pair_free(as_pair * p) {
    as_pair_destroy(p);
    free(p);
    return 0;
}

inline as_val * as_pair_1(as_pair * p) {
    return p->_1;
}

inline as_val * as_pair_2(as_pair * p) {
    return p->_2;
}

inline as_val * as_pair_toval(const as_pair * p) {
    return (as_val *)p;
}

inline as_pair * as_pair_fromval(const as_val * v) {
    return as_util_fromval(v, AS_PAIR, as_pair);
}

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define pair(a,b) \
    as_pair_new((as_val *) a, (as_val *) b)
