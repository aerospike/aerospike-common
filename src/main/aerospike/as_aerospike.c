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

#include <stdlib.h>

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_clock.h>
#include <aerospike/as_aerospike.h>

#include "internal.h"


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_aerospike_rec_create(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_update(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_exists(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_remove(const as_aerospike * a, const as_rec * r);

extern inline int as_aerospike_log(const as_aerospike * a, const char * name, const int line, const int lvl, const char * msg);
extern inline cf_clock as_aerospike_get_current_time(const as_aerospike * a );

extern inline as_rec * as_aerospike_crec_create(const as_aerospike * a, const as_rec * r);
extern inline as_rec * as_aerospike_crec_open(const as_aerospike * a, const as_rec *r, const char *);
extern inline int as_aerospike_crec_remove(const as_aerospike * a, const as_rec * cr);
extern inline int as_aerospike_crec_update(const as_aerospike * a, const as_rec *cr);
extern inline int as_aerospike_crec_close(const as_aerospike * a, const as_rec *cr);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_init(as_aerospike * a, void * s, const as_aerospike_hooks * h)
{
	a->is_rcalloc = false;
	a->source = s;
	a->hooks = h;
	return a;
}

as_aerospike * as_aerospike_new(void * s, const as_aerospike_hooks * h)
{
	as_aerospike * a = (as_aerospike *) cf_rc_alloc(sizeof(as_aerospike));
	a->is_rcalloc = true;
	a->source = s;
	a->hooks = h;
	return a;
}

void as_aerospike_destroy(as_aerospike * a)
{
	if (a->is_rcalloc)
	   cf_rc_releaseandfree(a);
}

