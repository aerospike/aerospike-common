#include "as_util.h"
#include "as_pair.h"
#include <stdlib.h>
#include <string.h>

static const as_val AS_PAIR_VAL;


int as_pair_free(as_pair * p) {
    if ( p->_1 ) free(p->_1);
    if ( p->_2 ) free(p->_2);
    free(p);
    return 0;
}

static char * as_pair_tostring(as_pair * p) {

    char * a = as_val_tostring(p->_1);
    size_t al = strlen(a);
    char * b = as_val_tostring(p->_2);
    size_t bl = strlen(b);
    
    size_t l = al + bl + 5;
    char * str = (char *) malloc(sizeof(char) * l);

    strcpy(str, "(");
    strcpy(str+1, a);
    strcpy(str+1+al,", ");
    strcpy(str+1+al+2, b);
    strcpy(str+1+al+2+bl,")");
    *(str+1+al+2+bl+1) = '\0';

    return str;
}

static uint32_t as_pair_hash(as_pair * p) {
    return 0;
}

as_pair * as_pair_new(as_val * _1, as_val * _2) {
    as_pair * p = (as_pair *) malloc(sizeof(as_pair));
    as_pair_init(p, _1, _2);
    return p;
}

int as_pair_init(as_pair * p, as_val * _1, as_val * _2) {
    p->_ = AS_PAIR_VAL;
    p->_1 = _1;
    p->_2 = _2;
    return 0;
}

as_val * as_pair_1(as_pair * p) {
    return p->_1;
}

as_val * as_pair_2(as_pair * p) {
    return p->_2;
}




static int as_pair_val_free(as_val * v) {
    return as_val_type(v) == AS_PAIR ? as_pair_free((as_pair *) v) : 1;
}

static uint32_t as_pair_val_hash(as_val * v) {
    return as_val_type(v) == AS_PAIR ? as_pair_hash((as_pair *) v) : 0;
}

static char * as_pair_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_PAIR ) return NULL;
    return as_pair_tostring((as_pair *) v);
}

static const as_val AS_PAIR_VAL = {
    .type       = AS_PAIR,
    .size       = sizeof(as_pair),
    .free       = as_pair_val_free,
    .hash       = as_pair_val_hash,
    .tostring   = as_pair_val_tostring
};