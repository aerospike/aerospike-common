/*
 * Copyright (C) 2014		Aaloan Miftah <aaloanm@smrtb.com>
 *
 * Licensed under GPLv2.
 *
 * Hash map implementation for as_val key/value pairs.
 */
#ifndef _AEROSPIKE_AS_VAL_HASHMAP_H_
#define _AEROSPIKE_AS_VAL_HASHMAP_H_ 1

#include <aerospike/as_map.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct _as_val_hashmap {
	/* private: internal use only */
	as_map _;

	float min_lf;		/* load factor before shrinking */
	float max_lf;		/* load factor before expanding */

	size_t lo_wm;		/* low watermark */
	size_t hi_wm;		/* high watermark */
	size_t nr_fr;		/* number of entries free */

	size_t size;		/* number of entries full */
	size_t cap;		/* total capacity */

	as_val **key_tbl;	/* key table */
	as_val **val_tbl;	/* value table */
	char	*st;		/* table state */
} as_val_hashmap;

extern as_val_hashmap *as_val_hashmap_init(as_val_hashmap *map, size_t inicap);
extern as_val_hashmap *as_val_hashmap_new(float min_lf, float max_lf,
					  size_t inicap);
extern bool as_val_hashmap_destroy(as_val_hashmap *map);

extern uint32_t as_val_hashmap_hashcode(const as_val_hashmap *map);

/**
 * Retrieve the number of entries in a map.
 *
 * @param map	target map
 * @return	number of entries in @map
 * @relatealso	as_val_hashmap
 */
static inline uint32_t as_val_hashmap_size(const as_val_hashmap *map)
{
	return map->size;
}

extern void as_val_hashmap_clear(as_val_hashmap *map);

extern as_val *as_val_hashmap_get(const as_val_hashmap *map, const as_val *k);
extern as_val *as_val_hashmap_set(as_val_hashmap *map, as_val *k, as_val *v);
extern as_val *as_val_hashmap_remove(as_val_hashmap *map, const as_val *k);

extern bool as_val_hashmap_foreach(const as_val_hashmap *map,
				   as_map_foreach_callback cb, void *udata);

/**
 * as_val_hashmap_foreach_entry() - iterate through all full entries in a map
 * @pos:	cursor
 * @count:	entry counter
 * @map:	&as_val_hashmap
 */
#define as_val_hashmap_foreach_entry(pos, count, map)			\
	for (pos = 0, count = 0; count != (map)->size; pos++,		\
		count += ((map)->st[pos-1] == ST_FULL ? 1 : 0))		\
		if ((map)->st[pos] == ST_FULL)

#endif /* _AEROSPIKE_AS_VAL_HASHMAP_H_ */
