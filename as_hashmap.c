#include "as_hashmap.h"
#include "as_string.h"
#include "as_pair.h"
#include "cf_shash.h"
#include <stdlib.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_hashmap_s as_hashmap;
typedef struct as_hashmap_iterator_source_s as_hashmap_iterator_source;

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

static int as_hashmap_free(as_map *);
static uint32_t as_hashmap_hash(const as_map *);
static uint32_t as_hashmap_size(const as_map *);
static int as_hashmap_set(as_map *, const as_val *, const as_val *);
static as_val * as_hashmap_get(const as_map *, const as_val *);
static int as_hashmap_clear(as_map *);
static as_iterator * as_hashmap_iterator(const as_map *);

static const int as_hashmap_iterator_free(as_iterator *);
static const bool as_hashmap_iterator_has_next(const as_iterator *);
static const as_val * as_hashmap_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_map_hooks as_hashmap_hooks = {
    .free       = as_hashmap_free,
    .hash       = as_hashmap_hash,
    .size       = as_hashmap_size,
    .set        = as_hashmap_set,
    .get        = as_hashmap_get,
    .clear      = as_hashmap_clear,
    .iterator   = as_hashmap_iterator
};

static const as_iterator_hooks as_hashmap_iterator_hooks = {
    .free       = as_hashmap_iterator_free,
    .has_next   = as_hashmap_iterator_has_next,
    .next       = as_hashmap_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static int as_hashmap_reduce_fn (void * key, void * data, void * udata) {
    as_pair * value = *((as_pair **) data);
    as_pair_free(value);
    return SHASH_REDUCE_DELETE;
}

static uint32_t as_hashmap_hashfn(void * k) {
    return *((uint32_t *) k);
}

as_map * as_hashmap_new(uint32_t capacity) {
    shash * t = NULL;
    shash_create(&t, as_hashmap_hashfn, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
    return as_map_new(t, &as_hashmap_hooks);
}

static int as_hashmap_free(as_map * m) {
    shash * t = (shash *) m->source;
    shash_reduce_delete(t, as_hashmap_reduce_fn, NULL);
    shash_destroy(t);
    // shash_destroy(t);
    // m->source = NULL;
    free(m);
    m = NULL;
    return 0;
}

static uint32_t as_hashmap_hash(const as_map * l) {
    return 0;
}

static uint32_t as_hashmap_size(const as_map * m) {
    shash * t = (shash *) m->source;
    return shash_get_size(t);
}

static int as_hashmap_set(as_map * m, const as_val * k, const as_val * v) {
    shash * t = (shash *) m->source;
    uint32_t h = as_val_hash(k);
    as_pair * p = pair(k,v);

    return shash_put(t, &h, &p);
}

static as_val * as_hashmap_get(const as_map * m, const as_val * k) {
    shash *  t = (shash *) m->source;
    uint32_t h = as_val_hash(k);
    as_pair * p = NULL;

    if ( shash_get(t, &h, &p) != SHASH_OK ) {
        return NULL;
    }

    return as_pair_2(p);
}

static int as_hashmap_clear(as_map * m) {
    shash * t = (shash *) m->source;
    shash_deleteall_lockfree(t);
    // shash_reduce_delete(t, as_hashmap_reduce_fn, NULL);
    return 0;
}

static as_iterator * as_hashmap_iterator(const as_map * m) {
    if ( m == NULL ) return NULL;
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) malloc(sizeof(as_hashmap_iterator_source));
    source->h = (shash *) as_map_source(m);
    source->curr = NULL;
    source->next = NULL;
    source->size = (uint32_t) source->h->table_len;
    source->pos = 0;
    return as_iterator_new(source, &as_hashmap_iterator_hooks);
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
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) as_iterator_source(i);
    if ( source ) free(source);
    free(i);
    return 0;
}
