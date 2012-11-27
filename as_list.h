#pragma once

#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>
#include <inttypes.h>

typedef struct as_list_s as_list;
typedef struct as_list_hooks_s as_list_hooks;


struct as_list_s {
    as_val                  _;
    void *                  source;
    const as_list_hooks *   hooks;
};

struct as_list_hooks_s {
    int (*free)(as_list *);

    uint32_t (* size)(const as_list *);
    
    int (* append)(as_list *, as_val *);
    int (* prepend)(as_list *, as_val *);
    
    as_val * (* get)(const as_list *, uint32_t);
    as_val * (* head)(const as_list *);
    as_list * (* tail)(const as_list *);
    
    as_list * (* take)(const as_list *, uint32_t);
    as_list * (* drop)(const as_list *, uint32_t);
    
    as_iterator * (* iterator)(const as_list *);
};


as_list * as_list_new(void *, const as_list_hooks *);

int as_list_free(as_list *);

void * as_list_source(const as_list * l);



int as_list_append(as_list *, as_val *);

int as_list_prepend(as_list *, as_val *);



as_val * as_list_get(const as_list *, const uint32_t i);

as_val * as_list_head(const as_list *);

as_list * as_list_tail(const as_list *);



as_val * as_list_toval(const as_list *);

as_list * as_list_fromval(const as_val *);

as_iterator * as_list_iterator(const as_list *);
