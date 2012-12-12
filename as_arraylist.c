#include "as_arraylist.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_arraylist_s as_arraylist;
typedef struct as_arraylist_iterator_source_s as_arraylist_iterator_source;

struct as_arraylist_s {
    as_val **   elements;
    uint32_t    size;
    uint32_t    capacity;
    uint32_t    block_size;
    bool        shadow;
};

struct as_arraylist_iterator_source_s {
    as_arraylist *  list;
    uint32_t        pos;
};

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_arraylist_free(as_list *);

static uint32_t as_arraylist_hash(const as_list *);
static uint32_t as_arraylist_size(const as_list *);
static int as_arraylist_append(as_list *, as_val *);
static int as_arraylist_prepend(as_list *, as_val *);
static as_val * as_arraylist_get(const as_list *, const uint32_t);
static int as_arraylist_set(as_list *, const uint32_t, as_val *);
static as_val * as_arraylist_head(const as_list *);
static as_list * as_arraylist_tail(const as_list *);
static as_iterator * as_arraylist_iterator(const as_list *);

static const int as_arraylist_iterator_free(as_iterator *);
static const bool as_arraylist_iterator_has_next(const as_iterator *);
static const as_val * as_arraylist_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_list_hooks as_arraylist_hooks = {
    .free       = as_arraylist_free,
    .hash       = as_arraylist_hash,
    .size       = as_arraylist_size,
    .append     = as_arraylist_append,
    .prepend    = as_arraylist_prepend,
    .get        = as_arraylist_get,
    .set        = as_arraylist_set,
    .head       = as_arraylist_head,
    .tail       = as_arraylist_tail,
    .iterator   = as_arraylist_iterator
};

static const as_iterator_hooks as_arraylist_iterator_hooks = {
    .free       = as_arraylist_iterator_free,
    .has_next   = as_arraylist_iterator_has_next,
    .next       = as_arraylist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_arraylist_init(as_list * l, uint32_t capacity, uint32_t block_size) {
    as_arraylist * a = (as_arraylist *) malloc(sizeof(as_arraylist));
    a->elements = (as_val **) malloc(sizeof(as_val *) * capacity);
    a->size = 0;
    a->capacity = capacity;
    a->block_size = block_size;
    a->shadow = false;
    return as_list_init(l, a, &as_arraylist_hooks);
}

as_list * as_arraylist_new(uint32_t capacity, uint32_t block_size) {
    as_arraylist * a = (as_arraylist *) malloc(sizeof(as_arraylist));
    a->elements = (as_val **) malloc(sizeof(as_val *) * capacity);
    a->size = 0;
    a->capacity = capacity;
    a->block_size = block_size;
    a->shadow = false;
    return as_list_new(a, &as_arraylist_hooks);
}

static int as_arraylist_free(as_list * l) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    
    if ( !al->shadow ) {
        for (int i = 0; i < al->size; i++ ) {
            free(al->elements[i]);
            al->elements[i] = NULL;
        }
    }

    free(l);
    return 0;
}

static uint32_t as_arraylist_hash(const as_list * l) {
    return 0;
}

static int as_arraylist_ensure(as_arraylist * al, uint32_t n) {
    if ( (al->size + n) >= al->capacity ) {
        al->elements = realloc(al->elements, sizeof(as_val) * (al->capacity + al->block_size));
        bzero(al->elements + (sizeof(as_val) * al->capacity), sizeof(as_val) * al->block_size);
    }
    return 0;
}

static uint32_t as_arraylist_size(const as_list * l) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return al->size;
}

static int as_arraylist_append(as_list * l, as_val * v) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    as_arraylist_ensure(al,1);
    al->elements[al->size] = v;
    al->size = al->size + 1;
    return 0;
}

static int as_arraylist_prepend(as_list * l, as_val * v) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    as_arraylist_ensure(al,1);

    for (int i = al->size; i > 0; i-- ) {
        al->elements[i] = al->elements[i-1];
    }

    al->elements[0] = v;
    al->size = al->size + 1;

    return 0;
}

static as_val * as_arraylist_get(const as_list * l, const uint32_t i) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return al->size > i ? al->elements[i] : NULL;
}

static int as_arraylist_set(as_list * l, const uint32_t i, as_val * v) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);

    if ( i > al->capacity ) {
        as_arraylist_ensure(al, i - al->capacity);
    }

    free(al->elements[i]);
    al->elements[i] = v;

    return 0;
}

static as_val * as_arraylist_head(const as_list * l) {
    return as_arraylist_get(l, 0);
}

static as_list * as_arraylist_tail(const as_list * l) {

    as_arraylist * al  = (as_arraylist *) as_list_source(l);

    if ( al->size == 0 ) return NULL;

    as_arraylist * al2 = (as_arraylist *) malloc(sizeof(as_arraylist));
    al2->elements = al->elements + 1;
    al2->size = al->size - 1;
    al2->capacity = al->capacity - 1;
    al2->block_size = al->block_size;
    al2->shadow = true;

    return as_list_new(al2, &as_arraylist_hooks);
}

static as_iterator * as_arraylist_iterator(const as_list * l) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) malloc(sizeof(as_arraylist_iterator_source));
    source->list = (as_arraylist *) as_list_source(l);
    source->pos = 0;
    return as_iterator_new(source, &as_arraylist_iterator_hooks);
}

static const bool as_arraylist_iterator_has_next(const as_iterator * i) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) as_iterator_source(i);
    return source->pos < source->list->size;
}

static const as_val * as_arraylist_iterator_next(as_iterator * i) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) as_iterator_source(i);
    if ( as_arraylist_iterator_has_next(i) ) {
        as_val * head = source->list->elements[source->pos];
        source->pos = source->pos+1;
        return head;
    }
    return NULL;
}

static const int as_arraylist_iterator_free(as_iterator * i) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) as_iterator_source(i);
    if ( source ) free(source);
    free(i);
    return 0;
}
