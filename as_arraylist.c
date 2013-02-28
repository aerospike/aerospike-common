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

static bool             as_arraylist_list_foreach(const as_list *, void *, bool (*foreach)(as_val *, void *));

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
    .destroy            = as_arraylist_list_destroy,
    .hash               = as_arraylist_list_hash,
    .size               = as_arraylist_list_size,
    .append             = as_arraylist_list_append,
    .prepend            = as_arraylist_list_prepend,
    .get                = as_arraylist_list_get,
    .set                = as_arraylist_list_set,
    .head               = as_arraylist_list_head,
    .tail               = as_arraylist_list_tail,
    .drop               = as_arraylist_list_drop,
    .take               = as_arraylist_list_take,
    .foreach            = as_arraylist_list_foreach,
    .iterator_init      = as_arraylist_list_iterator_init,
    .iterator_new       = as_arraylist_list_iterator_new
};

const as_iterator_hooks as_arraylist_iterator_hooks = {
    .destroy    = as_arraylist_iterator_destroy,
    .has_next   = as_arraylist_iterator_has_next,
    .next       = as_arraylist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

// Initialize an arraylist, with room for  "capacity" number of elements
// and with the new (delta) allocation amount of "block_size" elements.
// Use calloc() for the new element memory so that it is initialized to zero
as_list * as_arraylist_init(as_list * l, uint32_t capacity, uint32_t block_size) {
    as_val_init(&l->_, AS_LIST, false /*is_malloc*/);
    l->hooks = &as_arraylist_list_hooks;
    // Allocate 'capacity' elements of SIZE bytes each, all initialized to 0.
    l->u.arraylist.elements =
    		(as_val **) calloc( capacity, sizeof(as_val *) );
    l->u.arraylist.size = 0; // No elements have been added yet
    l->u.arraylist.capacity = capacity;
    l->u.arraylist.block_size = block_size;
    return l;
}

// Create a new arraylist, with room for  "capacity" number of elements
// and with the new (delta) allocation amount of "block_size" elements.
// Use calloc() for the new element memory so that it is initialized to zero
as_list * as_arraylist_new(uint32_t capacity, uint32_t block_size) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    as_val_init(&l->_, AS_LIST, true /*is_malloc*/);
    l->hooks = &as_arraylist_list_hooks;
    // Allocate 'capacity' elements of SIZE bytes each, all initialized to 0.
    l->u.arraylist.elements = (as_val **) calloc(capacity, sizeof(as_val *));
    l->u.arraylist.size = 0; // No elements have been added yet
    l->u.arraylist.capacity = capacity;
    l->u.arraylist.block_size = block_size;
    return l;
} // end as_arraylist_new()

//
// as_arraylist_destroy
// helper function for those who like the joy of as_arraylist_new
void as_arraylist_destroy(as_list * l) {
	as_val_val_destroy( (as_val *) l);
}

// Destroy the list.  It is assumed that valid elements are not null,
// and all "element[]" array positions that do NOT contain elements ARE null.
// So it is important that all element arrays begin initialized
// with zero.
// Destroy EACH element in the array, then free the element array itself.
// (*) a.elements is the (allocated) element array
// (*) a.size is the current number of elements in this list
// (*) a.capacity is the total number of elements for which there is space
// (*) a.block_size is the number of NEW elements we will allocate for
//     when we grow the array list (e.g. 8, meaning space for 8 more elements)
void as_arraylist_list_destroy(as_list * l) {
    as_arraylist_source *a = &l->u.arraylist;
    for (int i = 0; i < a->size; i++ ) {
        if (a->elements[i]) {    // check for non-null valid elements
            as_val_destroy(a->elements[i]);
        }
        a->elements[i] = NULL;
    }
    free(a->elements);
    a->elements = NULL;
    a->size = 0;  // no more elements
    a->capacity = 0; // no more room for elements
} // end as_arraylist_list_destroy()

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

// Check the amount of space allocated for this object and allocate MORE
// if necessary. There must be room for "delta_new" more elements.
// Parms:
//  (l) is the array list
//  (delta_new) is the number of NEW elements that are going to be added, and
//   that we have to make sure we have room for.
// Structure notes:
// (*) The Element array is a list of pointers to as_val
// (*) a.elements is the (allocated) element array
// (*) a.size is the current number of elements in this list
// (*) a.capacity is the total number of elements for which there is space
// (*) a.block_size is the number of NEW elements we will allocate for
//     when we grow the array list (e.g. 8, meaning space for 8 more elements)
static int as_arraylist_ensure(as_list * l, uint32_t delta_new) {
    as_arraylist_source *a = &l->u.arraylist;
    // Check for capacity (in terms of elements, NOT size in bytes), and if we
    // need to allocate more, do a realloc.
    if ( (a->size + delta_new) > a->capacity ) {
    	// by convention - we allocate more space ONLY when the unit of
    	// (new) allocation is > 0.
        if ( a->block_size > 0 ) {
        	// Compute how much room we're missing for the new stuff
        	int new_room = (a->size + delta_new) - a->capacity;
        	// Compute new capacity in terms of multiples of block_size
        	// This will get us (conservatively) at least one block
            int new_blocks = (new_room + a->block_size) / a->block_size;
            int new_capacity = a->capacity + (new_blocks * a->block_size);
            as_val ** elements =
              (as_val **) cf_realloc(a->elements, sizeof(as_val *) * new_capacity);
            if ( elements != NULL ) {
            	// Looks like it worked, so fill in the new values
                a->elements = elements;
                // bzero commented out because it is too expensive
                // bzero(a->elements + (sizeof(as_val *) * a->capacity), sizeof(as_val *) * a->block_size);
                a->capacity = new_capacity;  // New, Improved Size
                return AS_ARRAYLIST_OK;
            }
            a->elements = elements;
            return AS_ARRAYLIST_ERR_ALLOC;
        }
        return AS_ARRAYLIST_ERR_MAX;
    } // end if
    return AS_ARRAYLIST_OK;
} // end as_arraylist_ensure()

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
    a->elements[a->size] = v;
    a->size++;
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

// Set the arraylist (L) at element index position (i) with element value (v).
// Notice that in order to maintain proper object/memory management, we
// just first destroy (as_val_destroy()) the old object at element position(i)
// before assigning the new element.  Also note that the object at element
// position (i) is assumed to exist, so all element positions must be
// appropriately initialized to zero.
static int as_arraylist_list_set(as_list * l, const uint32_t index, as_val * v) {

    as_arraylist_source *a = &l->u.arraylist;
    int rc = AS_ARRAYLIST_OK;
    if ( index > a->capacity ) {
        rc = as_arraylist_ensure(l, index - a->capacity);
        if ( rc != AS_ARRAYLIST_OK ) {
        	return rc;
        }
    }
    as_val_destroy(a->elements[index]);
    a->elements[index] = v;
    if( index  >= a->size ){
    	a->size = index + 1;
    }

    return rc;
} // end as_arraylist_list_set()

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
    as_arraylist_source *sa = &s->u.arraylist;
    sa->size = sz-c;

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
    as_arraylist_source *sa = &s->u.arraylist;
    sa->size = c;

    for(int i = 0; i < c; i++) {
         as_val_reserve(a->elements[i]);
         sa->elements[i] = a->elements[i];
    }

    return s;
}

static bool as_arraylist_list_foreach(const as_list * l, void * udata, bool (*foreach)(as_val * val, void * udata)) {
    const as_arraylist_source * a = &l->u.arraylist;
    for(int i = 0; i < a->size; i++ ) {
        if ( foreach(a->elements[i], udata) == false ) {
            return false;
        }
    }
    return true;
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
