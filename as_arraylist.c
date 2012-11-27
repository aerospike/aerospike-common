#include "as_arraylist.h"
#include <stdlib.h>

typedef struct as_arraylist_s as_arraylist;
typedef struct as_arraylist_iterator_source_s as_arraylist_iterator_source;

static const as_list_hooks as_arraylist_hooks;
static const as_iterator_hooks as_arraylist_iterator_hooks;


static int as_arraylist_free(as_list *);
static uint32_t as_arraylist_size(const as_list *);
static int as_arraylist_append(as_list *, as_val *);
static int as_arraylist_prepend(as_list *, as_val *);
static as_val * as_arraylist_get(const as_list *, uint32_t);
static as_val * as_arraylist_head(const as_list *);
static as_list * as_arraylist_tail(const as_list *);
static as_list * as_arraylist_take(const as_list *, uint32_t);
static as_list * as_arraylist_drop(const as_list *, uint32_t);
static as_iterator * as_arraylist_iterator(const as_list *);



struct as_arraylist_s {
    as_val **   elements;
    uint32_t    size;
    uint32_t    capacity;
    uint32_t    block_size;
};

struct as_arraylist_iterator_source_s {
    as_arraylist *  list;
    uint32_t        pos;
};



as_list * as_arraylist_new(uint32_t capacity, uint32_t block_size) {
    as_arraylist * l = (as_arraylist *) malloc(sizeof(as_arraylist));
    l->elements = (as_val **) malloc(sizeof(as_val *) * capacity);
    l->size = 0;
    l->capacity = capacity;
    l->block_size = block_size;
    return as_list_new(l, &as_arraylist_hooks);
}

static int as_arraylist_free(as_list * l) {
    // as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return 0;
}

static uint32_t as_arraylist_size(const as_list * l) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return al->size;
}

static int as_arraylist_append(as_list * l, as_val * v) {
    // as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return 0;
}

static int as_arraylist_prepend(as_list * l, as_val * v) {
    // as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return 0;
}

static as_val * as_arraylist_get(const as_list * l, uint32_t i) {
    as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return al->size > i ? al->elements[i] : NULL;
}

static as_val * as_arraylist_head(const as_list * l) {
    return as_arraylist_get(l, 0);
}

static as_list * as_arraylist_tail(const as_list * l) {
    // as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return (as_list *) l;
}

static as_list * as_arraylist_take(const as_list * l, uint32_t n) {
    // as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return (as_list *) l;
}

static as_list * as_arraylist_drop(const as_list * l, uint32_t n) {
    // as_arraylist * al  = (as_arraylist *) as_list_source(l);
    return 0;
}

static as_iterator * as_arraylist_iterator(const as_list * l) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) malloc(sizeof(as_arraylist_iterator_source));
    source->list = (as_arraylist *) as_list_source(l);
    source->pos = 0;
    return as_iterator_new(source, &as_arraylist_iterator_hooks);
}

static const bool as_arraylist_iterator_has_next(const as_iterator * i) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) as_iterator_source(i);
    return source->pos <= source->list->size;
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

static const as_iterator_hooks as_arraylist_iterator_hooks = {
    .has_next   = as_arraylist_iterator_has_next,
    .next       = as_arraylist_iterator_next,
    .free       = as_arraylist_iterator_free
};

static const as_list_hooks as_arraylist_hooks = {
    .free       = as_arraylist_free,
    .size       = as_arraylist_size,
    .append     = as_arraylist_append,
    .prepend    = as_arraylist_prepend,
    .get        = as_arraylist_get,
    .head       = as_arraylist_head,
    .tail       = as_arraylist_tail,
    .take       = as_arraylist_take,
    .drop       = as_arraylist_drop,
    .iterator   = as_arraylist_iterator
};