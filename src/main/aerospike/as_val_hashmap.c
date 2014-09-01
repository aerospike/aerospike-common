/*
 * Copyright 2014 (C)		Aaloan Miftah <aaloanm@smrtb.com>
 * Copyright 1999 (C)		CERN - European Organization for Nuclear Research
 *
 * Licensed under GPLv2.
 *
 * Hash map implementation for as_val key/value pairs.
 */

#include <stdlib.h>
#include <string.h>

/* we need state constants (ST_FREE, etc.) */
#define AS_VAL_HASHMAP_PRIVATE 1

#include <citrusleaf/alloc.h>
#include <aerospike/as_boolean.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_val_hashmap.h>

#include "primes.h"

#define DEFAULT_MAX_LOAD_FACTOR		0.75f
#define DEFAULT_MIN_LOAD_FACTOR		0.50f

/* as_map fn hook table */
#define __hook(t, n) .t = (typeof(map_hooks.t)) n
static const as_map_hooks map_hooks = {
	__hook(destroy,		as_val_hashmap_release),
	__hook(hashcode,	as_val_hashmap_hashcode),
	__hook(size,		as_val_hashmap_size),
	__hook(set,		as_val_hashmap_set),
	__hook(get,		as_val_hashmap_get),
	__hook(remove,		as_val_hashmap_remove),
	__hook(clear,		as_val_hashmap_clear),
	__hook(foreach,		as_val_hashmap_foreach)
};

/**
 * choose_hiwm() - calculate high watermark threshold for a map
 * @map:	const &as_val_hashmap
 */
static inline size_t choose_hiwm(const as_val_hashmap *map)
{
	size_t cap = map->cap;
	size_t x = cap - 2;
	size_t y = (size_t) (cap * map->max_lf);
	return x < y ? x : y;
}

/**
 * choose_lowm() - calculate low watermark threshold for a map
 */
static inline size_t choose_lowm(const as_val_hashmap *map)
{
	return (size_t) (map->cap * map->min_lf);
}

/**
 * choose_grow_cap() - calculate new capacity for expansion
 */
static inline size_t choose_grow_cap(const as_val_hashmap *map)
{
	size_t sz = map->size + 1;
	size_t x = sz + 2;
	size_t y = (size_t) ((4 * sz) / (3 * map->min_lf + map->max_lf));
	return next_prime(x > y ? x : y);
}

/**
 * choose_shrink_cap() - calculate new capacity for shrinking
 */
static inline size_t choose_shrink_cap(const as_val_hashmap *map)
{
	size_t sz = map->size;
	size_t x = sz + 2;
	size_t y = (size_t) ((4 * sz) / (map->min_lf + 3 * map->max_lf));
	return next_prime(x > y ? x : y);
}

/**
 * eq_val() - compare two values for equality
 */
static bool eq_val(const as_val *x, const as_val *y)
{
	if (!x && !y)
		return true;
	if (!x || !y)
		return false;
	if (as_val_type(x) != as_val_type(y))
		return false;
	switch (as_val_type(x)) {
	case AS_NIL:
		return true;
	case AS_BOOLEAN:
		return as_boolean_get((const as_boolean *) x) ==
			as_boolean_get((const as_boolean *) y);
	case AS_INTEGER:
		return as_integer_get((const as_integer *) x) ==
			as_integer_get((const as_integer *) y);
	case AS_STRING:
		return strcmp(as_string_get((const as_string *) x),
			      as_string_get((const as_string *) y)) == 0;
	case AS_PAIR:
		return eq_val(as_pair_1((as_pair *) x),
			      as_pair_1((as_pair *) y)) &&
		       eq_val(as_pair_2((as_pair *) x),
			      as_pair_2((as_pair *) y));
	case AS_LIST:
	case AS_MAP:
	case AS_REC:
	case AS_BYTES:
		/* TODO */
	default:
		return false;
	}
}

/**
 * indexof() - determine index of key in a map
 * @map:	haystack
 * @k:		needle
 *
 * If @k is found in @map, a positive value denoting index of @k in @map,
 * otherwise -1 is returned.
 */
static ssize_t indexof(const as_val_hashmap *map, const as_val *k)
{
	char s;
	const char *st = map->st;
	size_t cap = map->cap;
	uint32_t hash = as_val_hashcode(k);
	ssize_t idx = hash % cap;
	uint32_t dec = hash % (cap - 2);
	if (dec == 0)
		dec = 1;
	while ((s = st[idx]) != ST_FREE &&
		(s == ST_REMOVED || !eq_val(map->key_tbl[idx], k))) {
		idx -= dec;
		if (idx < 0)
			idx += cap;
	}
	return s == ST_FREE ? -1 : idx;
}

/**
 * indexof_insert() - determine index to insert key at in map
 * @map:	haystack
 * @k:		needle
 *
 * If @k exists in @map, '-index - 1' is returned, otherwise a positive
 * value denoting the suggested insertion index.
 */
static ssize_t indexof_insert(const as_val_hashmap *map, const as_val *k)
{
	char s;
	const char *st = map->st;
	size_t cap = map->cap;
	uint32_t hash = as_val_hashcode(k);
	ssize_t idx = hash % cap;
	uint32_t dec = hash % (cap - 2);
	if (dec == 0)
		dec = 1;
	while ((s = st[idx]) == ST_FULL && !eq_val(map->key_tbl[idx], k)) {
		idx -= dec;
		if (idx < 0)
			idx += cap;
	}
	if (s == ST_REMOVED) {
		size_t j = idx;
		while ((s = st[idx]) != ST_FREE &&
			(s == ST_REMOVED || !eq_val(map->key_tbl[idx], k))) {
			idx -= dec;
			if (idx < 0)
				idx += cap;
		}
		if (s == ST_FREE)
			idx = j;
	}
	return s == ST_FULL ? (-idx - 1) : idx;
}

/**
 * resize_hash_map() - resize hash map
 * @map:	target map
 * @cap:	new capacity
 */
static void resize_hash_map(as_val_hashmap *map, size_t cap)
{
	size_t i, j;
	as_val_hashmap old_map = *map;

	/*
	 * decrement value calculated with (hash % (cap - 2)),
	 * if cap <= 2, results in arith exception
	 */
	if (cap < 3)
		cap = 3;

	map->cap = cap;
	map->nr_fr = cap - map->size;
	map->lo_wm = choose_lowm(map);
	map->hi_wm = choose_hiwm(map);
	map->st = cf_malloc(cap);
	map->key_tbl = cf_malloc(sizeof(as_val *) * cap);
	map->val_tbl = cf_malloc(sizeof(as_val *) * cap);

	memset(map->st, ST_FREE, cap);

	as_val_hashmap_foreach_entry(i, j, &old_map) {
		as_val *k = old_map.key_tbl[i];
		as_val *v = old_map.val_tbl[i];
		ssize_t idx = indexof_insert(map, k);
		map->key_tbl[idx] = k;
		map->val_tbl[idx] = v;
		map->st[idx] = ST_FULL;
	}
	cf_free(old_map.st);
	cf_free(old_map.key_tbl);
	cf_free(old_map.val_tbl);
}

as_val_hashmap *__as_val_hashmap_init(as_val_hashmap *map, size_t cap)
{
	if (!map)
		return NULL;

	/* calculate next prime capacity from initial capacity */
	if (cap < 3)
		cap = 3;
	else
		cap = next_prime(cap);

	/* set default values if not present or invalid */
	if (map->max_lf <= 0.0f || map->max_lf > 1.0f)
		map->max_lf = DEFAULT_MAX_LOAD_FACTOR;
	if (map->min_lf < 0.0f || map->min_lf >= 1.0f)
		map->min_lf = DEFAULT_MIN_LOAD_FACTOR;

	/* initialize val_hashmap */
	map->size = 0;
	map->cap = cap;
	map->nr_fr = cap;
	map->lo_wm = 0;
	map->hi_wm = choose_hiwm(map);

	map->key_tbl = cf_malloc(sizeof(as_val *) * cap);
	if (!map->key_tbl)
		goto out_mem;
	map->val_tbl = cf_malloc(sizeof(as_val *) * cap);
	if (!map->val_tbl)
		goto out_key_tbl;
	map->st = cf_malloc(cap);
	if (!map->st)
		goto out_val_tbl;

	memset(map->st, ST_FREE, cap);

	return map;
out_val_tbl:
	cf_free(map->val_tbl);
out_key_tbl:
	cf_free(map->key_tbl);
out_mem:
	return NULL;
}

/**
 * Initialize hash map structure.
 *
 * @param map	target map
 * @param cap	initial capacity
 * @return	on success @map is returned, otherwise NULL.
 */
as_val_hashmap *as_val_hashmap_init(as_val_hashmap *map, size_t cap)
{
	map = __as_val_hashmap_init(map, cap);
	if (!map)
		return NULL;
	/* initialize parent map structure */
	as_map_cons((as_map *) map, false, NULL, &map_hooks);
	return map;
}

/**
 * Allocate and initialize hash map
 *
 * @param min_lf	minimum load factor before shrinking map [0..1)
 * @param max_lf	maximum load factor before expanding map (0..1]
 * @param inicap	initial capacity
 * @return		pointer to new hash map, or NULL
 */
as_val_hashmap *as_val_hashmap_new(float min_lf, float max_lf, size_t inicap)
{
	as_val_hashmap *ret;
	as_val_hashmap *map = cf_malloc(sizeof(as_val_hashmap));
	if (!map)
		return NULL;
	map->min_lf = min_lf;
	map->max_lf = max_lf;
	ret = __as_val_hashmap_init(map, inicap);
	if (!ret) {
		cf_free(map);
		return NULL;
	}
	/* initialize parent map structure */
	as_map_cons((as_map *) map, true, NULL, &map_hooks);
	return ret;
}

/**
 * Release hash map
 *
 * @param map	target map
 */
bool as_val_hashmap_release(as_val_hashmap *map)
{
	cf_free(map->st);
	cf_free(map->key_tbl);
	cf_free(map->val_tbl);
	return true;
}

/**
 * Destroy hash map
 *
 * @param map	target map
 */
void as_val_hashmap_destroy(as_val_hashmap *map)
{
	as_map_destroy((as_map *) map);
}

/**
 * Calculate hash code of map
 *
 * @param map	target map
 * @return	hash code of map
 */
uint32_t as_val_hashmap_hashcode(const as_val_hashmap *map)
{
	size_t i, j;
	uint32_t hash = 0;
	as_val_hashmap_foreach_entry(i, j, map) {
		hash = as_val_hashcode(map->key_tbl[i]) ^
			as_val_hashcode(map->val_tbl[i]);
	}
	return hash;
}

/**
 * Remove all mappings from map
 *
 * @param map	target map
 * @return	always returns 0
 */
int as_val_hashmap_clear(as_val_hashmap *map)
{
	size_t new_cap;

	map->size = 0;
	map->nr_fr = map->cap;
	memset(map->st, ST_FREE, map->cap);

	new_cap = choose_shrink_cap(map);
	resize_hash_map(map, new_cap);
	return 0;
}

/**
 * Retrive mapping for key
 *
 * @param map	target map
 * @param k	key to search for
 * @return	value mapped to @k, or NULL if @k has no mapping
 */
as_val *as_val_hashmap_get(const as_val_hashmap *map, const as_val *k)
{
	ssize_t idx = indexof(map, k);
	if (idx < 0)
		return NULL;
	return map->val_tbl[idx];
}

/**
 * Map value to key
 *
 * @param map	target map
 * @param k	key
 * @param v	value
 * @return	previous value mapped to @k, or NULL if @k had no previous mapping
 */
as_val *as_val_hashmap_set(as_val_hashmap *map, as_val *k, as_val *v)
{
	ssize_t idx = indexof_insert(map, k);
	if (idx < 0) {
		as_val *old_v;

		idx = -idx - 1;
		old_v = map->val_tbl[idx];
		map->val_tbl[idx] = v;
		return old_v;
	}

	if (map->size > map->hi_wm) {
		size_t new_cap = choose_grow_cap(map);
		resize_hash_map(map, new_cap);
		return as_val_hashmap_set(map, k, v);
	}

	map->key_tbl[idx] = k;
	map->val_tbl[idx] = v;
	if (map->st[idx] == ST_FREE)
		map->nr_fr--;
	map->st[idx] = ST_FULL;
	map->size++;
	if (map->nr_fr < 1) {
		size_t new_cap = choose_grow_cap(map);
		resize_hash_map(map, new_cap);
	}
	return NULL;
}

/**
 * Remove mapping for key
 *
 * @param map	target map
 * @param k	key
 * @return	value that was mapped to @k, or NULL if @k had no mapping
 */
as_val *as_val_hashmap_remove(as_val_hashmap *map, const as_val *k)
{
	as_val *old_v;
	ssize_t idx = indexof(map, k);
	if (idx < 0)
		return NULL;
	old_v = map->val_tbl[idx];
	map->st[idx] = ST_REMOVED;
	map->size--;
	if (map->size < map->lo_wm) {
		size_t new_cap = choose_shrink_cap(map);
		resize_hash_map(map, new_cap);
	}
	return old_v;
}

/**
 * Iterate through all mappings in a map
 *
 * @param map	target map
 * @param cb	callback function
 * @param udata	user data
 * @return true on success, false otherwise
 */
bool as_val_hashmap_foreach(const as_val_hashmap *map,
			    as_map_foreach_callback cb, void *udata)
{
	size_t i, j;
	as_val_hashmap_foreach_entry(i, j, map) {
		const as_val *k = map->key_tbl[i];
		const as_val *v = map->val_tbl[i];
		if (!cb(k, v, udata))
			break;
	}
	return true;
}
