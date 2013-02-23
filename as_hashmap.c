#include <cf_shash.h>
#include <cf_alloc.h>
#include <stdlib.h>

#include "as_hashmap.h"
#include "as_string.h"

#include "internal.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static void		 as_hashmap_map_destroy(as_map *m);
uint32_t     as_hashmap_hash_fn(void *);
int          as_hashmap_reduce_fn (void *, void *, void *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_map_hooks as_hashmap_map_hooks = {
    .destroy        = as_hashmap_map_destroy,
    .hash           = as_hashmap_hash,
    .size           = as_hashmap_size,
    .set            = as_hashmap_set,
    .get            = as_hashmap_get,
    .clear          = as_hashmap_clear,
    .iterator_init  = as_hashmap_iterator_init,
    .iterator_new   = as_hashmap_iterator_new
};

const as_iterator_hooks as_hashmap_iterator_hooks = {
    .destroy        = as_hashmap_iterator_destroy,
    .has_next       = as_hashmap_iterator_has_next,
    .next           = as_hashmap_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_map *as_hashmap_init(as_map *m, uint32_t capacity) {
    as_val_init(&m->_, AS_MAP, false /*is_malloc*/);
    m->hooks = &as_hashmap_map_hooks;
    as_hashmap_source  *s = &(m->u.hashmap);
    shash_create(&s->h, as_hashmap_hash_fn, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
    return m;
}

as_map *as_hashmap_new(uint32_t capacity) {
    as_map *m = (as_map *) malloc(sizeof(as_map));
    as_val_init(&m->_, AS_MAP, true /*is_malloc*/);
    m->hooks = &as_hashmap_map_hooks;
    as_hashmap_source  *s = &(m->u.hashmap);
    shash_create(&s->h, as_hashmap_hash_fn, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
    return m;
}

static void as_hashmap_map_destroy(as_map * m) {
    as_hashmap_clear(m);
    as_hashmap_source  *s = &(m->u.hashmap);
    shash_destroy(s->h);
}

void as_hashmap_destroy(as_map *m) {
	as_val_val_destroy( (as_val *) m );
}

int as_hashmap_set(as_map * m, const as_val * k, const as_val * v) {

    as_hashmap_source  *s = &(m->u.hashmap);
    uint32_t h = as_val_hash(k);
    as_pair * p = NULL;

    if ( shash_get(s->h, &h, &p) == SHASH_OK ) {
        as_val_destroy((as_val *)p);
        p = NULL;
    }
    p = pair_new(k,v);
    return shash_put(s->h, &h, &p);
}

as_val * as_hashmap_get(const as_map * m, const as_val * k) {
    const as_hashmap_source  *s = &(m->u.hashmap);
    uint32_t h = as_val_hash(k);
    as_pair * p = NULL;

    if ( shash_get(s->h, &h, &p) != SHASH_OK ) {
        return NULL;
    }
    as_val *v = as_pair_2(p);
    return v;
}

int as_hashmap_clear(as_map * m) {
    as_hashmap_source  *s = &(m->u.hashmap);
    shash_reduce_delete(s->h, as_hashmap_reduce_fn, NULL);
    return 0;
}

uint32_t as_hashmap_size(const as_map * m) {
    const as_hashmap_source  *s = &(m->u.hashmap);
    return shash_get_size(s->h);
}

uint32_t as_hashmap_hash(const as_map *m) {
    return(1);
}

uint32_t as_hashmap_hash_fn(void * k) {
    return *((uint32_t *) k);
}

int as_hashmap_reduce_fn (void * key, void * data, void * udata) {
    as_val * value = *((as_val **) data); // this is the pair at this key-value
    as_val_destroy(value);
    return SHASH_REDUCE_DELETE;
}


as_iterator * as_hashmap_iterator_init(const as_map * m, as_iterator *i ) {
    i->is_malloc = false;
    i->hooks = &as_hashmap_iterator_hooks;
    as_hashmap_iterator_source * is = (as_hashmap_iterator_source *) &i->u.hashmap;
    const as_hashmap_source * ms = (as_hashmap_source *) &m->u.hashmap;
    is->h = ms->h;
    is->curr = NULL;
    is->next = NULL;
    is->size = (uint32_t) ms->h->table_len;
    is->pos = 0;
    return i;
}

as_iterator * as_hashmap_iterator_new(const as_map * m) {
    as_iterator *i = (as_iterator *) malloc(sizeof(as_iterator));
    i->is_malloc = true;
    i->hooks = &as_hashmap_iterator_hooks;
    as_hashmap_iterator_source * is = (as_hashmap_iterator_source *) &i->u.hashmap;
    const as_hashmap_source * ms = (as_hashmap_source *) &m->u.hashmap;
    is->h = ms->h;
    is->curr = NULL;
    is->next = NULL;
    is->size = (uint32_t) ms->h->table_len;
    is->pos = 0;
    return i;
}

static bool as_hashmap_iterator_seek(as_hashmap_iterator_source * source) {

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

bool as_hashmap_iterator_has_next(const as_iterator * i) {
    as_hashmap_iterator_source * s = (as_hashmap_iterator_source *) &i->u.hashmap;
    return as_hashmap_iterator_seek(s);
}

as_val * as_hashmap_iterator_next(as_iterator * i) {
    as_hashmap_iterator_source * s = (as_hashmap_iterator_source *) &i->u.hashmap;

    if ( !as_hashmap_iterator_seek(s) ) return NULL;

    shash *         h   = s->h;
    shash_elem *    e   = s->curr;
    as_pair **      p   = (as_pair **) SHASH_ELEM_VALUE_PTR(h, e);
    
    s->curr = NULL; // consume the value, so we can get the next one.
    
    return (as_val *) *p;
}

void as_hashmap_iterator_destroy(as_iterator * i) {
    return;
}
