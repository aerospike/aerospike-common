#include "as_arraylist.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cf_alloc.h>

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

//
// as_list Implementation
//
static int              as_arraylist_list_free(as_list *);
static uint32_t         as_arraylist_list_hash(const as_list *);
static uint32_t         as_arraylist_list_size(const as_list *);
static int              as_arraylist_list_append(as_list *, as_val *);
static int              as_arraylist_list_prepend(as_list *, as_val *);
static as_val *         as_arraylist_list_get(const as_list *, const uint32_t);
static int              as_arraylist_list_set(as_list *, const uint32_t, as_val *);
static as_val *         as_arraylist_list_head(const as_list *);
static as_list *        as_arraylist_list_tail(const as_list *);
static as_list *        as_arraylist_list_drop(const as_list *, uint32_t);
static as_list *        as_arraylist_list_take(const as_list *, uint32_t);
static as_iterator *    as_arraylist_list_iterator(const as_list *);

//
// as_iterator Implementation
//
static const int        as_arraylist_iterator_free(as_iterator *);
static const bool       as_arraylist_iterator_has_next(const as_iterator *);
static const as_val *   as_arraylist_iterator_next(as_iterator *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_list_hooks as_arraylist_list = {
    .free       = as_arraylist_list_free,
    .hash       = as_arraylist_list_hash,
    .size       = as_arraylist_list_size,
    .append     = as_arraylist_list_append,
    .prepend    = as_arraylist_list_prepend,
    .get        = as_arraylist_list_get,
    .set        = as_arraylist_list_set,
    .head       = as_arraylist_list_head,
    .tail       = as_arraylist_list_tail,
    .drop       = as_arraylist_list_drop,
    .take       = as_arraylist_list_take,
    .foreach    = NULL,     // @TODO: implement
    .iterator   = as_arraylist_list_iterator
};

const as_iterator_hooks as_arraylist_iterator = {
    .free       = as_arraylist_iterator_free,
    .has_next   = as_arraylist_iterator_has_next,
    .next       = as_arraylist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/


as_arraylist * as_arraylist_init(as_arraylist * a, uint32_t capacity, uint32_t block_size) {
    if ( !a ) return a;
    capacity = capacity == 0 ? 8 : capacity;
    a->elements = (as_val **) cf_malloc(sizeof(as_val *) * capacity);
    a->size = 0;
    a->capacity = capacity;
    a->block_size = block_size;
    return a;
}

int as_arraylist_destroy(as_arraylist * a) {
    if ( !a ) return 0;
    for (int i = 0; i < a->size; i++ ) {
        as_val_free(a->elements[i]);
        a->elements[i] = NULL;
    }
    cf_free(a->elements);
    a->elements = NULL;
    a->size = 0;
    a->capacity = 0;
    return 0;
}

as_arraylist * as_arraylist_new(uint32_t capacity, uint32_t block_size) {
    as_arraylist * a = (as_arraylist *) cf_rc_alloc(sizeof(as_arraylist));
    return as_arraylist_init(a, capacity, block_size);
}

int as_arraylist_free(as_arraylist * a) {
    if ( !a ) return 0;
    if ( cf_rc_release(a) > 0 ) return 0;
    as_arraylist_destroy(a);
    cf_rc_free(a);
    return 0;
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_arraylist_ensure(as_arraylist * a, uint32_t n) {
    if ( (a->size + n) >= a->capacity && a->block_size > 0 ) {
        a->elements = realloc(a->elements, sizeof(as_val *) * (a->capacity + a->block_size));
        if ( a->elements ) {
            bzero(a->elements + (sizeof(as_val *) * a->capacity), sizeof(as_val *) * a->block_size);
            a->capacity = a->capacity + a->block_size;
            return AS_ARRAYLIST_OK;
        }
        return AS_ARRAYLIST_ERR_ALLOC;
    }
    return AS_ARRAYLIST_ERR_MAX;
}


inline int as_arraylist_list_free(as_list * l) {
    as_arraylist_free((as_arraylist *) l->source);
    l->source = NULL;
    return 0;
}

static uint32_t as_arraylist_list_hash(const as_list * l) {
    return 0;
}


static uint32_t as_arraylist_list_size(const as_list * l) {
    as_arraylist * a  = (as_arraylist *) as_list_source(l);
    return a->size;
}

static int as_arraylist_list_append(as_list * l, as_val * v) {
    as_arraylist * a = (as_arraylist *) as_list_source(l);
    as_arraylist_ensure(a,1);
    a->elements[a->size++] = v;
    return 0;
}

static int as_arraylist_list_prepend(as_list * l, as_val * v) {
    as_arraylist * a  = (as_arraylist *) as_list_source(l);
    as_arraylist_ensure(a,1);

    for (int i = a->size; i > 0; i-- ) {
        a->elements[i] = a->elements[i-1];
    }

    a->elements[0] = v;
    a->size++;

    return 0;
}

static as_val * as_arraylist_list_get(const as_list * l, const uint32_t i) {
    as_arraylist * a  = (as_arraylist *) as_list_source(l);
    return a->size > i ? a->elements[i] : NULL;
}

static int as_arraylist_list_set(as_list * l, const uint32_t i, as_val * v) {
    as_arraylist * a  = (as_arraylist *) as_list_source(l);

    if ( i > a->capacity ) {
        as_arraylist_ensure(a, i - a->capacity);
    }

    as_val_free(a->elements[i]);
    a->elements[i] = v;
    a->size = i > a->size ? i : a->size;

    return 0;
}

static as_val * as_arraylist_list_head(const as_list * l) {
    return as_arraylist_list_get(l, 0);
}

static as_list * as_arraylist_list_tail(const as_list * l) {

    as_arraylist * a  = (as_arraylist *) as_list_source(l);

    if ( a->size == 0 ) return NULL;

    as_arraylist * s = as_arraylist_new(a->size-1, a->block_size);

    for(int i = 1, j = 0; i < a->size; i++, j++) {
        s->elements[j] = as_val_ref(a->elements[i]);
    }

    return as_list_new(s, &as_arraylist_list);
}

static as_list * as_arraylist_list_drop(const as_list * l, uint32_t n) {

    as_arraylist *  a   = (as_arraylist *) as_list_source(l);
    uint32_t        sz  = a->size;
    uint32_t        c   = n < sz ? n : sz;
    as_arraylist *  s   = as_arraylist_new(sz-c, a->block_size);

    s->size = sz-c;

    /**
     * Is memcpy faster?
     */

    for(int i = c, j = 0; i < sz; i++, j++) {
        s->elements[j] = as_val_ref(a->elements[i]);
    }

    return as_list_new(s, &as_arraylist_list);
}

static as_list * as_arraylist_list_take(const as_list * l, uint32_t n) {

    as_arraylist *  a   = (as_arraylist *) as_list_source(l);
    uint32_t        sz  = a->size;
    uint32_t        c   = n < sz ? n : sz;
    as_arraylist *  s   = as_arraylist_new(c, a->block_size);

    s->size = c;

    /**
     * Is memcpy faster?
     */
    
    for(int i = 0; i < c; i++) {
        s->elements[i] = a->elements[i];
        s->elements[i] = as_val_ref(a->elements[i]);
    }

    return as_list_new(s, &as_arraylist_list);
}

static as_iterator * as_arraylist_list_iterator(const as_list * l) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) malloc(sizeof(as_arraylist_iterator_source));
    source->list = (as_arraylist *) l->source;
    source->pos = 0;
    return as_iterator_new(source, &as_arraylist_iterator);
}



static const bool as_arraylist_iterator_has_next(const as_iterator * i) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) i->source;
    return source && source->pos < source->list->size;
}

static const as_val * as_arraylist_iterator_next(as_iterator * i) {
    as_arraylist_iterator_source * source = (as_arraylist_iterator_source *) i->source;
    if ( source && (source->pos < source->list->size) ) {
        as_val * val = *(source->list->elements + source->pos);
        source->pos++;
        return val;
    }
    return NULL;
}

static const int as_arraylist_iterator_free(as_iterator * i) {
    if ( !i ) return 0;
    if ( i->source ) free(i->source);
    i->source = NULL;
    return 0;
}
