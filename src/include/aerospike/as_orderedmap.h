/*
 * Copyright 2022 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#pragma once

#include <aerospike/as_std.h>
#include <aerospike/as_map.h>
#include <aerospike/as_pair.h>

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 *	TYPES
 ******************************************************************************/

/**
 * Private helper structure.
 */
typedef struct map_entry_s {
	as_val* key;
	as_val* value;
} map_entry;

/**
 *	A sorted array based implementation of `as_map`.
 *
 *	To use the map, you can either initialize a stack allocated map,
 *	using `as_orderedmap_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap map;
 *	as_orderedmap_init(&map, 256);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated map using
 *	`as_orderedmap_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap* map = as_orderedmap_new(256);
 *	~~~~~~~~~~
 *
 *	When you are finished using the map, then you should release the
 *	map and associated resources, using `as_orderedmap_destroy()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap_destroy(map);
 *	~~~~~~~~~~
 *
 *
 *	The `as_orderedmap` is a subtype of `as_map`. This allows you to
 *	alternatively use `as_map` functions, by typecasting `as_orderedmap` to
 *	`as_map`.
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap map;
 *	as_map* l = (as_map*)as_orderedmap_init(&map, 4);
 *	as_stringmap_set_int64(l, "a", 1);
 *	as_stringmap_set_int64(l, "b", 2);
 *	as_stringmap_set_int64(l, "c", 3);
 *	as_stringmap_set_int64(l, "d", 4);
 *	as_map_destroy(l);
 *	~~~~~~~~~~
 *
 *	The `as_stringmap` functions are simplified functions for using string key.
 *
 *	Each of the `as_map` functions proxy to the `as_orderedmap` functions.
 *	So, calling `as_map_destroy()` is equivalent to calling
 *	`as_orderedmap_destroy()`.
 *
 *	Notes:
 *
 *	This orderedmap implementation is NOT threadsafe.
 *
 *	Internally, the orderedmap stores keys' and values' pointers - it does NOT
 *	copy the keys or values, so the caller must ensure these keys and values are
 *	not destroyed while the orderedmap is still in use.
 *
 *	Further, the orderedmap does not increment ref-counts of the keys or values.
 *	However when an element is removed from the orderedmap, the orderedmap will
 *	call as_val_destroy() on both the key and value. And when the orderedmap is
 *	cleared or destroyed, as_val_destroy() will be called for all keys and
 *	values. Therefore if the caller inserts keys and values in the orderedmap
 *	without extra ref-counts, the caller is effectively handing off ownership of
 *	these objects to the orderedmap.
 *
 *	@extends as_map
 *	@ingroup aerospike_t
 */
typedef struct as_orderedmap_s {
	as_map _;

	uint32_t count;
	uint32_t capacity;
	map_entry* table;

	uint32_t hold_count;
	map_entry* hold_table;
	uint32_t* hold_locations;
} as_orderedmap;

/**
 *	Iterator for as_orderedmap.
 *
 *	To use the iterator, you can either initialize a stack allocated variable,
 *	use `as_orderedmap_iterator_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap_iterator it;
 *	as_orderedmap_iterator_init(&it, &map);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated variable using
 *	`as_orderedmap_iterator_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap_iterator* it = as_orderedmap_iterator_new(&map);
 *	~~~~~~~~~~
 *
 *	To iterate, use `as_orderedmap_iterator_has_next()` and
 *	`as_orderedmap_iterator_next()`:
 *
 *	~~~~~~~~~~{.c}
 *	while ( as_orderedmap_iterator_has_next(&it) ) {
 *		const as_val* val = as_orderedmap_iterator_next(&it);
 *	}
 *	~~~~~~~~~~
 *
 *	When you are finished using the iterator, then you should release the
 *	iterator and associated resources:
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap_iterator_destroy(it);
 *	~~~~~~~~~~
 *
 *
 *	The `as_orderedmap_iterator` is a subtype of  `as_iterator`. This allows you
 *	to alternatively use `as_iterator` functions, by typecasting
 *	`as_orderedmap_iterator` to `as_iterator`.
 *
 *	~~~~~~~~~~{.c}
 *	as_orderedmap_iterator it;
 *	as_iterator* i = (as_iterator*)as_orderedmap_iterator_init(&it, &map);
 *
 *	while (as_iterator_has_next(i)) {
 *		const as_val* as_iterator_next(i);
 *	}
 *
 *	as_iterator_destroy(i);
 *	~~~~~~~~~~
 *
 *	Each of the `as_iterator` functions proxy to the `as_orderedmap_iterator`
 *	functions. So, calling `as_iterator_destroy()` is equivalent to calling
 *	`as_orderedmap_iterator_destroy()`.
 *
 *	Notes:
 *
 *	as_orderedmap_iterator_next() returns an as_pair pointer. The as_pair
 *	contains the key and value pointers of the current map element. This one
 *	as_pair "container" is re-used for all the iterations, i.e. the contents
 *	will be overwritten and are only valid until the next iteration.
 *
 *	@extends as_iterator
 */
typedef struct as_orderedmap_iterator_s {
	as_iterator _;

	const as_orderedmap* map;
	uint32_t ix;
	as_pair pair;
} as_orderedmap_iterator;


/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated orderedmap.
 *
 *	@param map 			The map to initialize.
 *	@param capacity		The number of entries (keys) to allocate for.
 *
 *	@return On success, the initialized map. Otherwise NULL.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN as_orderedmap* as_orderedmap_init(as_orderedmap* map, uint32_t capacity);

/**
 *	Creates a new map as an orderedmap.
 *
 *	@param capacity		The number of keys to allocate for.
 *
 *	@return On success, the new map. Otherwise NULL.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN as_orderedmap* as_orderedmap_new(uint32_t capacity);

/**
 *	Free the map and associated resources.
 *
 *	@param map 	The map to destroy.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN void as_orderedmap_destroy(as_orderedmap* map);

/**
 *	Get the number of entries in the map.
 *
 *	@param map 	The map.
 *
 *	@return The number of entries in the map.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN uint32_t as_orderedmap_size(const as_orderedmap* map);

/**
 *	Get the value for specified key.
 *
 *	@param map 		The map.
 *	@param key		The key.
 *
 *	@return The value for the specified key. Otherwise NULL.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN as_val* as_orderedmap_get(const as_orderedmap* map, const as_val* key);

/**
 *	Set the value for specified key.
 *
 *	@param map 		The map.
 *	@param key		The key.
 *	@param val		The value for the given key.
 *
 *	@return 0 on success. Otherwise an error occurred.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN int as_orderedmap_set(as_orderedmap* map, const as_val* key, const as_val* val);

/**
 *	Remove all entries from the map.
 *
 *	@param map		The map.
 *
 *	@return 0 on success. Otherwise an error occurred.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN int as_orderedmap_clear(as_orderedmap* map);

/**
 *	Remove the entry specified by the key.
 *
 *	@param map 	The map to remove the entry from.
 *	@param key 	The key of the entry to be removed.
 *
 *	@return 0 on success. Otherwise an error occurred.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN int as_orderedmap_remove(as_orderedmap* map, const as_val* key);

/**
 *	Set map attributes.
 *
 *	@relatesalso as_orderedmap
 */
static inline void
as_orderedmap_set_flags(as_orderedmap* map, uint32_t flags)
{
	map->_.flags = flags & AS_MAP_FLAGS_MASK;

	// Ensure k-ordered is set when other bits require k-ordered to be set.
	if (map->_.flags != 0) {
		map->_.flags |= 1;
	}
}


/******************************************************************************
 *	ITERATION FUNCTIONS
 *****************************************************************************/

/**
 *	Call the callback function for each entry in the map.
 *
 *	@param map		The map.
 *	@param callback	The function to call for each entry.
 *	@param udata	User-data to be passed to the callback.
 *
 *	@return true if iteration completes fully. false if iteration was aborted.
 *
 *	@relatesalso as_orderedmap
 */
AS_EXTERN bool as_orderedmap_foreach(const as_orderedmap* map, as_map_foreach_callback callback, void* udata);

/**
 *	Initializes a stack allocated as_iterator for the given as_orderedmap.
 *
 *	@param it		The iterator to initialize.
 *	@param map		The map to iterate.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 *
 *	@relatesalso as_orderedmap_iterator
 */
AS_EXTERN as_orderedmap_iterator* as_orderedmap_iterator_init(as_orderedmap_iterator* it, const as_orderedmap* map);

/**
 *	Creates a heap allocated as_iterator for the given as_orderedmap.
 *
 *	@param map 		The map to iterate.
 *
 *	@return On success, the new iterator. Otherwise NULL.
 *
 *	@relatesalso as_orderedmap_iterator
 */
AS_EXTERN as_orderedmap_iterator* as_orderedmap_iterator_new(const as_orderedmap* map);

/**
 *	Destroy the iterator and releases resources used by the iterator.
 *
 *	@param it		The iterator to release
 *
 *	@relatesalso as_orderedmap_iterator
 */
AS_EXTERN void as_orderedmap_iterator_destroy(as_orderedmap_iterator* it);

/**
 *	Tests if there are more values available in the iterator.
 *
 *	@param it		The iterator to be tested.
 *
 *	@return true if there are more values. Otherwise false.
 *
 *	@relatesalso as_orderedmap_iterator
 */
AS_EXTERN bool as_orderedmap_iterator_has_next(const as_orderedmap_iterator* it);

/**
 *	Attempts to get the next value from the iterator.
 *	This will return the next value, and iterate past the value.
 *
 *	@param it		The iterator to get the next value from.
 *
 *	@return The next value in the list if available. Otherwise NULL.
 *
 *	@relatesalso as_orderedmap_iterator
 */
AS_EXTERN const as_val* as_orderedmap_iterator_next(as_orderedmap_iterator* it);

#ifdef __cplusplus
} // end extern "C"
#endif
