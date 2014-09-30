/*
 * Copyright (C) 2014		Aaloan Miftah <aaloanm@smrtb.com>
 *
 * Licensed under GPLv2.
 *
 * val_hashmap iterator
 */
#ifndef _AEROSPIKE_AS_VAL_HASHMAP_ITERATOR_H_
#define _AEROSPIKE_AS_VAL_HASHMAP_ITERATOR_H_ 1

#include <aerospike/as_val_hashmap.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_pair.h>

#include <stdbool.h>

typedef struct _as_val_hashmap_iterator {
	as_iterator _;

	as_val_hashmap *map;	/* map we're iterating over */
	bool next;		/* next entry indicator */
	size_t pos;		/* cursor */
	size_t count;		/* nth entry */
	as_pair pair;		/* current pair */
} as_val_hashmap_iterator;

extern as_val_hashmap_iterator *as_val_hashmap_iterator_init(
	as_val_hashmap_iterator *itr, as_val_hashmap *map);
extern as_val_hashmap_iterator *as_val_hashmap_iterator_new(
	as_val_hashmap *map);
extern bool as_val_hashmap_iterator_release(as_val_hashmap_iterator *itr);
extern void as_val_hashmap_iterator_destroy(as_val_hashmap_iterator *itr);

extern bool as_val_hashmap_iterator_has_next(
	const as_val_hashmap_iterator *itr);
extern const as_val *as_val_hashmap_iterator_next(as_val_hashmap_iterator *itr);

#endif /* _AEROSPIKE_AS_VAL_HASHMAP_ITERATOR_H_ */
