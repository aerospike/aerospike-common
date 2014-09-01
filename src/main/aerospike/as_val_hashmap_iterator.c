#include <stdint.h>

#define AS_VAL_HASHMAP_PRIVATE 1

#include <citrusleaf/alloc.h>
#include <aerospike/as_val_hashmap.h>
#include <aerospike/as_val_hashmap_iterator.h>

/* as_iterator fn hook table */
#define __hook(t, n) .t = (typeof(iter_hooks.t)) n
static const as_iterator_hooks iter_hooks = {
	__hook(destroy,		as_val_hashmap_iterator_release),
	__hook(has_next,	as_val_hashmap_iterator_has_next),
	__hook(next,		as_val_hashmap_iterator_next)
};

static bool iter_next(as_val_hashmap_iterator *itr)
{
	if (itr->next)
		return true;
	if (itr->map->size == itr->count)
		return false;

	do {
		itr->pos++;
	} while (itr->map->st[itr->pos - 1] != ST_FULL);
	itr->count++;
	itr->next = true;
	return true;
}

static void init_iter(as_val_hashmap_iterator *itr, as_val_hashmap *map)
{
	itr->map = map;
	itr->next = false;
	itr->pos = 0;
	itr->count = 0;
}

/**
 * Initialize hashmap iterator structure.
 *
 * @param itr	target iterator
 * @param map	target map
 * @return	upon success, @itr is returned, otherwise NULL
 */
as_val_hashmap_iterator *as_val_hashmap_iterator_init(
	as_val_hashmap_iterator *itr, as_val_hashmap *map)
{
	if (!itr)
		return NULL;
	
	init_iter(itr, map);
	as_iterator_init((as_iterator *) itr, false, NULL, &iter_hooks);
	return itr;
}

/**
 * Allocate and initialize a new iterator structure
 *
 * @param map	target map
 * @return	upon success, pointer to newly allocated iterator is returned,
 *		otherwise, NULL.
 */
as_val_hashmap_iterator *as_val_hashmap_iterator_new(as_val_hashmap *map)
{
	as_val_hashmap_iterator *itr = cf_malloc(sizeof(as_val_hashmap_iterator));
	if (!itr)
		return NULL;
	init_iter(itr, map);
	as_iterator_init((as_iterator *) itr, true, NULL, &iter_hooks);
	return itr;
}

/**
 * Release iterator resources
 *
 * @param itr	target iterator
 * @return	always returns true
 */
bool as_val_hashmap_iterator_release(as_val_hashmap_iterator *itr)
{
	itr->map = NULL;
	return true;
}

/**
 * Destroy iterator
 *
 * @param itr	target iterator
 */
void as_val_hashmap_iterator_destroy(as_val_hashmap_iterator *itr)
{
	as_iterator_destroy((as_iterator *) itr);
}

/**
 * Determine if iterator has more values to traverse through
 *
 * @param itr	target iterator
 * @return	true if iterator has more values, false otherwise
 */
bool as_val_hashmap_iterator_has_next(const as_val_hashmap_iterator *itr)
{
	return iter_next((as_val_hashmap_iterator *) itr);
}

/**
 * Retrieve next value
 *
 * @param itr	target iterator
 * @return	next value in map, or NULL
 */
const as_val *as_val_hashmap_iterator_next(as_val_hashmap_iterator *itr)
{
	size_t pos;
	if (!iter_next(itr))
		return NULL;
	pos = itr->pos;
	itr->next = false;
	as_pair_init(&itr->pair, itr->map->key_tbl[pos-1],
		     itr->map->val_tbl[pos-1]);
	return (const as_val *) &itr->pair;
}
