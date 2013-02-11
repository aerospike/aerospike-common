#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cf_alloc.h>

#include "as_arraylist.h"
#include "internal.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

//
// as_list Implementation
// note these are not exactly "static", they are called through the hook
// functionality
//
static void             as_arraylist_list_destroy(as_list *);
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
static as_iterator *    as_arraylist_list_iterator_init(const as_list *, as_iterator *);
static as_iterator *    as_arraylist_list_iterator_new(const as_list *);

//
// as_iterator Implementation
//
static void       as_arraylist_iterator_destroy(as_iterator *);
static bool       as_arraylist_iterator_has_next(const as_iterator *);
static as_val *   as_arraylist_iterator_next(as_iterator *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_list_hooks as_arraylist_list_hooks = {
    .destroy       = as_arraylist_list_destroy,
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
    .iterator_init   = as_arraylist_list_iterator_init,
    .iterator_new    = as_arraylist_list_iterator_new
};

const as_iterator_hooks as_arraylist_iterator_hooks = {
    .destroy    = as_arraylist_iterator_destroy,
    .has_next   = as_arraylist_iterator_has_next,
    .next       = as_arraylist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/


as_list * as_arraylist_init(as_list * l, uint32_t capacity, uint32_t block_size) {
    as_val_init(&l->_, AS_LIST, false /*is_malloc*/);
    l->hooks = &as_arraylist_list_hooks;
    l->u.arraylist.elements = (as_val **) malloc(sizeof(as_val *) * capacity);
    l->u.arraylist.size = 0;
    l->u.arraylist.capacity = capacity;
    l->u.arraylist.size = block_size;
    return l;
}

as_list * as_arraylist_new(uint32_t capacity, uint32_t block_size) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    as_val_init(&l->_, AS_LIST, true /*is_malloc*/);
    l->hooks = &as_arraylist_list_hooks;
    l->u.arraylist.elements = (as_val **) malloc(sizeof(as_val *) * capacity);
    l->u.arraylist.size = 0;
    l->u.arraylist.capacity = capacity;
    l->u.arraylist.block_size = block_size;
    return l;
}


void as_arraylist_list_destroy(as_list * l) {
    as_arraylist_source *a = &l->u.arraylist;
    for (int i = 0; i < a->size; i++ ) {
        if (a->elements[i]) {
            as_val_destroy(a->elements[i]);
        }
        a->elements[i] = NULL;
    }
    free(a->elements);
    a->elements = NULL;
    a->size = 0;
    a->capacity = 0;
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_arraylist_ensure(as_list * l, uint32_t n) {
    as_arraylist_source *a = &l->u.arraylist;
    if ( (a->size + n) > a->capacity ) {
        if ( a->block_size > 0 ) {
            as_val ** elements = (as_val **) cf_realloc(a->elements, sizeof(as_val *) * (a->capacity + a->block_size));
            if ( elements != NULL ) {
                a->elements = elements;
                // bzero(a->elements + (sizeof(as_val *) * a->capacity), sizeof(as_val *) * a->block_size);
                a->capacity = a->capacity + a->block_size;
                return AS_ARRAYLIST_OK;
            }
            a->elements = elements;
            return AS_ARRAYLIST_ERR_ALLOC;
        }
        return AS_ARRAYLIST_ERR_MAX;
    }
    return AS_ARRAYLIST_OK;
}

static uint32_t as_arraylist_list_hash(const as_list * l) {
    return 0;
}

static uint32_t as_arraylist_list_size(const as_list * l) {
    const as_arraylist_source *a = &l->u.arraylist;
    return a->size;
}

static int as_arraylist_list_append(as_list * l, as_val * v) {
    as_arraylist_source *a = &l->u.arraylist;
    int rc = as_arraylist_ensure(l,1);
    if ( rc != AS_ARRAYLIST_OK ) return rc;
    a->elements[a->size++] = v;
    return rc;
}

static int as_arraylist_list_prepend(as_list * l, as_val * v) {
    as_arraylist_source *a = &l->u.arraylist;
    int rc = as_arraylist_ensure(l,1);
    if ( rc != AS_ARRAYLIST_OK ) return rc;

    for (int i = a->size; i > 0; i-- ) {
        a->elements[i] = a->elements[i-1];
    }

    a->elements[0] = v;
    a->size++;

    return rc;
}

static as_val * as_arraylist_list_get(const as_list * l, const uint32_t i) {
    const as_arraylist_source *a = &l->u.arraylist;
    return a->size > i ? a->elements[i] : NULL;
}

static int as_arraylist_list_set(as_list * l, const uint32_t i, as_val * v) {
    as_arraylist_source *a = &l->u.arraylist;
    int rc = AS_ARRAYLIST_OK;

    if ( i > a->capacity ) {
        rc = as_arraylist_ensure(l, i - a->capacity);
        if ( rc != AS_ARRAYLIST_OK ) return rc;
    }

    as_val_destroy(a->elements[i]);
    a->elements[i] = v;
    a->size = i > a->size ? i : a->size;

    return rc;
}

static as_val * as_arraylist_list_head(const as_list * l) {
    const as_arraylist_source *a = &l->u.arraylist;
    return a->elements[0];
}

// returns all elements other than the head
static as_list * as_arraylist_list_tail(const as_list * l) {

    const as_arraylist_source *a = &l->u.arraylist;

    if ( a->size == 0 ) return NULL;

    as_list * s = as_arraylist_new(a->size-1, a->block_size);
    const as_arraylist_source *sa = &l->u.arraylist;

    for(int i = 1, j = 0; i < a->size; i++, j++) {
        as_val_reserve(a->elements[i]);
        sa->elements[j] = a->elements[i];
    }

    return s;
}

static as_list * as_arraylist_list_drop(const as_list * l, uint32_t n) {

    const as_arraylist_source *a = &l->u.arraylist;
    uint32_t        sz  = a->size;
    uint32_t        c   = n < sz ? n : sz;
    as_list *  s   = as_arraylist_new(sz-c, a->block_size);
    const as_arraylist_source *sa = &l->u.arraylist;

    for(int i = c, j = 0; j < sa->size; i++, j++) {
        as_val_reserve(a->elements[i]);
        sa->elements[j] = a->elements[i];
    }

    return s;
}

static as_list * as_arraylist_list_take(const as_list * l, uint32_t n) {

    const as_arraylist_source *a = &l->u.arraylist;
    uint32_t        sz  = a->size;
    uint32_t        c   = n < sz ? n : sz;
    as_list *  s   = as_arraylist_new(c, a->block_size);
    const as_arraylist_source *sa = &l->u.arraylist;

    for(int i = 0; i < c; i++) {
        sa->elements[i] = a->elements[i];
        as_val_reserve(a->elements[i]);
    }

    return s;
}

static as_iterator * as_arraylist_list_iterator_init(const as_list * l, as_iterator *i) {
    i->is_malloc = false;
    i->hooks = &as_arraylist_iterator_hooks;
    as_arraylist_iterator_source * s = (as_arraylist_iterator_source *) &(i->u.arraylist);
    s->list = &(l->u.arraylist);
    s->pos = 0;
    return i;
}

static as_iterator * as_arraylist_list_iterator_new(const as_list * l) {
    as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
    i->is_malloc = true;
    i->hooks = &as_arraylist_iterator_hooks;
    as_arraylist_iterator_source * s = (as_arraylist_iterator_source *) &(i->u.arraylist);
    s->list = &(l->u.arraylist);
    s->pos = 0;
    return i;
}


static bool as_arraylist_iterator_has_next(const as_iterator * i) {
    const as_arraylist_iterator_source * s = &i->u.arraylist;
    return s->pos < s->list->size;
}

static as_val * as_arraylist_iterator_next(as_iterator * i) {
    as_arraylist_iterator_source * s = &i->u.arraylist;
    if ( s->pos < s->list->size ) {
        as_val * val = *(s->list->elements + s->pos);
        s->pos++;
        return val;
    }
    return NULL;
}

static void as_arraylist_iterator_destroy(as_iterator * i) {
    return;
}
