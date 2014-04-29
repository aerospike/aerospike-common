/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <aerospike/as_util.h>
#include <aerospike/as_types.h>
#include <citrusleaf/cf_clock.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_aerospike_s as_aerospike;
typedef struct as_aerospike_hooks_s as_aerospike_hooks;

struct as_aerospike_s {
	bool    is_rcalloc;
	void * source;
	const as_aerospike_hooks * hooks;
};

struct as_aerospike_hooks_s {
	void (* destroy)(as_aerospike *);

	int (* rec_create)(const as_aerospike *, const as_rec *);
	int (* rec_update)(const as_aerospike *, const as_rec *);
	int (* rec_remove)(const as_aerospike *, const as_rec *);
	int (* rec_exists)(const as_aerospike *, const as_rec *);

	int (*log)(const as_aerospike *, const char *, const int, const int, const char *);
	cf_clock (* get_current_time)( const as_aerospike * );

	as_rec *(* create_subrec)(const as_aerospike *, const as_rec *);
	as_rec *(* open_subrec)(const as_aerospike *, const as_rec *, const char *);
	int (* update_subrec)(const as_aerospike *, const as_rec *);
	int (* remove_subrec)(const as_aerospike *, const as_rec *);
	int (* close_subrec)(const as_aerospike *, const as_rec *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_init(as_aerospike *a, void *source, const as_aerospike_hooks *hooks);

as_aerospike * as_aerospike_new(void *source, const as_aerospike_hooks *hooks);

void as_aerospike_destroy(as_aerospike *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

static inline int as_aerospike_rec_create(const as_aerospike * a, const as_rec * r)
{
	return as_util_hook(rec_create, 1, a, r);
}

static inline int as_aerospike_rec_update(const as_aerospike * a, const as_rec * r)
{
	return as_util_hook(rec_update, 1, a, r);
}

static inline int as_aerospike_rec_exists(const as_aerospike * a, const as_rec * r)
{
	return as_util_hook(rec_exists, 1, a, r);
}

static inline int as_aerospike_rec_remove(const as_aerospike * a, const as_rec * r)
{
	return as_util_hook(rec_remove, 1, a, r);
}

static inline int as_aerospike_log(const as_aerospike * a, const char * name, const int line, const int lvl, const char * msg) 
{
	return as_util_hook(log, 1, a, name, line, lvl, msg);
}

static inline cf_clock as_aerospike_get_current_time(const as_aerospike * a )
{
	return as_util_hook(get_current_time, 0, a);
}

static inline as_rec * as_aerospike_crec_create(const as_aerospike * a, const as_rec * r) {
	return as_util_hook(create_subrec, NULL, a, r);
}

static inline int as_aerospike_crec_update(const as_aerospike * a, const as_rec * cr)
{
	return as_util_hook(update_subrec, 1, a, cr);
}

static inline int as_aerospike_crec_remove(const as_aerospike * a, const as_rec * cr)
{
	return as_util_hook(remove_subrec, 1, a, cr);
}

static inline as_rec * as_aerospike_crec_open(const as_aerospike * a, const as_rec * r, const char * dig)
{
	return as_util_hook(open_subrec, NULL, a, r, dig);
}

static inline int as_aerospike_crec_close(const as_aerospike * a, const as_rec * cr)
{
	return as_util_hook(close_subrec, 1, a, cr);
}
