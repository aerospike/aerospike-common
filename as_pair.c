
#include "as_pair.h"
#include <stdlib.h>

struct as_pair_s {
    as_val _;
    as_val * _1;
    as_val * _2;
};


static const as_val AS_PAIR_VAL;
static int as_pair_freeval(as_val *);


int as_pair_free(as_pair * p) {
    if ( p->_1 ) free(p->_1);
    if ( p->_2 ) free(p->_2);
    free(p);
    return 0;
}

static uint32_t as_pair_hash(as_pair * p) {
    return 0;
}

as_pair * as_pair_new(as_val * _1, as_val * _2) {
    as_pair * p = (as_pair *) malloc(sizeof(as_pair));
    p->_ = AS_PAIR_VAL;
    p->_1 = _1;
    p->_2 = _2;
    return p;
}


as_val * as_pair_1(as_pair * p) {
    return p->_1;
}

as_val * as_pair_2(as_pair * p) {
    return p->_2;
}

as_val * as_pair_toval(const as_pair * p) {
    return (as_val *) p;
}

as_pair * as_pair_fromval(const as_val * v) {
    return as_val_type(v) == AS_PAIR ? (as_pair *) v : NULL;
}

static int as_pair_freeval(as_val * v) {
    return as_val_type(v) == AS_PAIR ? as_pair_free((as_pair *) v) : 1;
}

static int as_pair_hashval(as_val * v) {
    return as_val_type(v) == AS_PAIR ? as_pair_hash((as_pair *) v) : 0;
}

static const as_val AS_PAIR_VAL = {
    .type   = AS_PAIR, 
    .free   = as_pair_freeval,
    .hash   = as_pair_hashval
};