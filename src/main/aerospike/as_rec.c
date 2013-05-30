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
#include <stdio.h>

#include <citrusleaf/cf_alloc.h>
#include <aerospike/as_rec.h>
#include <aerospike/as_bytes.h>


/******************************************************************************
 * INLINES
 ******************************************************************************/

extern inline void *    as_rec_source(as_rec *r);
extern inline void      as_rec_destroy(as_rec *r);

extern inline as_val *  as_rec_get(const as_rec * r, const char * name) ;
extern inline int       as_rec_set(const as_rec * r, const char * name, const as_val * value) ;
extern inline int       as_rec_remove(const as_rec * r, const char * name) ;
extern inline uint32_t  as_rec_ttl(const as_rec * r);
extern inline uint16_t  as_rec_gen(const as_rec * r) ;
extern inline as_bytes *as_rec_digest(const as_rec * r) ;
extern inline uint16_t  as_rec_numbins(const as_rec * r) ;

extern inline as_val *  as_rec_toval(const as_rec * r) ;
extern inline as_rec *  as_rec_fromval(const as_val * v) ;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_rec * as_rec_init(as_rec * r, void * data, const as_rec_hooks * hooks) {
    if ( !r ) return r;
    as_val_init(&r->_, AS_REC, false);
    r->data = data;
    r->hooks = hooks;
    return r;
}

as_rec * as_rec_new(void * data, const as_rec_hooks * hooks) {
    as_rec * r = (as_rec *) malloc(sizeof(as_rec));
    as_val_init(&r->_, AS_REC, true);
    r->data = data;
    r->hooks = hooks;
    return r;
}

void as_rec_val_destroy(as_val *v) {
    as_rec * r = as_rec_fromval(v);
    as_util_hook(destroy, false, r);
}

uint32_t as_rec_val_hashcode(const as_val * v) {
    as_rec * r = as_rec_fromval(v);
    return as_util_hook(hashcode, 0, r);
}

char * as_rec_val_tostring(const as_val * v) {
    return strdup("[ REC ]");
}
