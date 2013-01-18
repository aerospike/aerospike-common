#include <cf_shash.h>
#include "as_hashmap.h"
#include "as_string.h"
#include <stdlib.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

struct as_hashmap_iterator_source_s {
    shash * h;
    shash_elem * curr;
    shash_elem * next;
    uint32_t pos;
    uint32_t size;
};

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static uint32_t     as_hashmap_hash_fn(void *);
static int          as_hashmap_reduce_fn (void *, void *, void *);

// as_map Implementation
static int              as_hashmap_map_free(as_map *);
static uint32_t         as_hashmap_map_hash(const as_map *);
static uint32_t         as_hashmap_map_size(const as_map *);
static int              as_hashmap_map_set(as_map *, const as_val *, const as_val *);
static as_val *         as_hashmap_map_get(const as_map *, const as_val *);
static int              as_hashmap_map_clear(as_map *);
static as_iterator *    as_hashmap_map_iterator(const as_map *);

// as_iterator Implementation
static const int        as_hashmap_iterator_free(as_iterator *);
static const bool       as_hashmap_iterator_has_next(const as_iterator *);
static const as_val *   as_hashmap_iterator_next(as_iterator *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_map_hooks as_hashmap_map = {
    .free       = as_hashmap_map_free,
    .hash       = as_hashmap_map_hash,
    .size       = as_hashmap_map_size,
    .set        = as_hashmap_map_set,
    .get        = as_hashmap_map_get,
    .clear      = as_hashmap_map_clear,
    .iterator   = as_hashmap_map_iterator
};

const as_iterator_hooks as_hashmap_iterator = {
    .free       = as_hashmap_iterator_free,
    .has_next   = as_hashmap_iterator_has_next,
    .next       = as_hashmap_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_hashmap * as_hashmap_new(uint32_t capacity) {
    as_hashmap * m = NULL;
    shash_create(&m, as_hashmap_hash_fn, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
    return m;
}

int as_hashmap_free(as_hashmap * m) {
    as_hashmap_clear(m);
    shash_destroy(m);
    m = NULL;
    return 0;
}

int as_hashmap_set(as_hashmap * m, const as_val * k, const as_val * v) {
    uint32_t h = as_val_hash(k);
    as_pair * p = pair(k,v);
    return shash_put(m, &h, &p);
}

as_val * as_hashmap_get(const as_hashmap * m, const as_val * k) {
    uint32_t h = as_val_hash(k);
    as_pair * p = NULL;

    if ( shash_get((as_hashmap *) m, &h, &p) != SHASH_OK ) {
        return NULL;
    }

    return as_pair_2(p);
}

int as_hashmap_clear(as_hashmap * m) {
    shash_reduce_delete(m, as_hashmap_reduce_fn, NULL);
    return 0;
}

uint32_t as_hashmap_size(const as_hashmap * m) {
    return shash_get_size((as_hashmap *) m);
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/


static uint32_t as_hashmap_hash_fn(void * k) {
    return *((uint32_t *) k);
}

static int as_hashmap_reduce_fn (void * key, void * data, void * udata) {
    as_pair * value = *((as_pair **) data);
    as_pair_free(value);
    return SHASH_REDUCE_DELETE;
}

static int as_hashmap_map_free(as_map * m) {
    as_hashmap_free((shash *) m->source);
    m->source = NULL;
    return 0;
}

static uint32_t as_hashmap_map_hash(const as_map * l) {
    return 0;
}

static uint32_t as_hashmap_map_size(const as_map * m) {
    return as_hashmap_size((shash *) m->source);
}

static int as_hashmap_map_set(as_map * m, const as_val * k, const as_val * v) {
    return as_hashmap_set((shash *) m->source, k, v);
}

static as_val * as_hashmap_map_get(const as_map * m, const as_val * k) {
    return as_hashmap_get((shash *) m->source, k);
}

static int as_hashmap_map_clear(as_map * m) {
    return as_hashmap_clear((shash *) m->source);
}

static as_iterator * as_hashmap_map_iterator(const as_map * m) {
    if ( m == NULL ) return NULL;
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) malloc(sizeof(as_hashmap_iterator_source));
    source->h = (shash *) as_map_source(m);
    source->curr = NULL;
    source->next = NULL;
    source->size = (uint32_t) source->h->table_len;
    source->pos = 0;
    return as_iterator_new(source, &as_hashmap_iterator);
}




static const bool as_hashmap_iterator_seek(as_hashmap_iterator_source * source) {

    // We no longer have slots in the table
    if ( source->pos > source->size ) return false;

    // If curr is set, that means we have a value ready to be read.
    if ( source->curr != NULL ) return true;

    // If next is set, that means we have something to iterate to.
    if ( source->next != NULL ) {
        if ( source->next->in_use ) {
            source->curr = source->next;
            source->next = source->curr->next;
            if ( !source->next ) {
                source->pos++;
            }
            return true;
        }
        else {
            source->pos++;
            source->next = NULL;
        }
    }

    // Iterate over the slots in the table
    for( ; source->pos < source->size; source->pos++ ) {

        // Get the bucket in the current slot
        source->curr = (shash_elem *) (((byte *) source->h->table) + (SHASH_ELEM_SZ(source->h) * source->pos));

        // If the bucket has a value, then return true
        if ( source->curr && source->curr->in_use ) {
            
            // we set next, so we have the next item in the bucket
            source->next = source->curr->next;

            // if next is empty, then we will move to the next bucket
            if ( !source->next ) source->pos++;

            return true;
        }
        else {
            source->curr = NULL;
            source->next = NULL;
        }
    }
    
    source->curr = NULL;
    source->next = NULL;
    source->pos = source->size;
    return false;
}

static const bool as_hashmap_iterator_has_next(const as_iterator * i) {
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) as_iterator_source(i);
    return as_hashmap_iterator_seek(source);
}

static const as_val * as_hashmap_iterator_next(as_iterator * i) {
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) as_iterator_source(i);

    if ( !as_hashmap_iterator_seek(source) ) return NULL;

    shash *         h   = source->h;
    shash_elem *    e   = source->curr;
    as_pair **      p   = (as_pair **) SHASH_ELEM_VALUE_PTR(h, e);
    
    source->curr = NULL; // consume the value, so we can get the next one.
    
    return (as_val *) *p;
}

static const int as_hashmap_iterator_free(as_iterator * i) {
    if ( !i ) return 0;
    if ( i->source ) free(i->source);
    i->source = NULL;
    return 0;
}
