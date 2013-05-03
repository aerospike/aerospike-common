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
#include <string.h>

#include <citrusleaf/cf_alloc.h>
#include <aerospike/as_util.h>
#include <aerospike/as_pair.h>

#include "internal.h"

extern inline uint32_t as_pair_hash(const as_pair *p);
extern inline as_val * as_pair_1(as_pair * p);
extern inline as_val * as_pair_2(as_pair * p);
extern inline as_val * as_pair_toval(const as_pair * p);
extern inline as_pair * as_pair_fromval(const as_val * v) ;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_pair * as_pair_init(as_pair * p, as_val * _1, as_val * _2) {
    as_val_init(&p->_,AS_PAIR, false /*is_rcalloc*/);
    p->_1 = _1;
    p->_2 = _2;
    return p;
}

as_pair * as_pair_new(as_val * _1, as_val * _2) {
    as_pair * p = (as_pair *) malloc(sizeof(as_pair));
    as_val_init(&p->_,AS_PAIR, true /*is_rcalloc*/);
    p->_1 = _1;
    p->_2 = _2;
    return p;
}

void as_pair_destroy(as_pair * p) {
	as_val_val_destroy((as_val *)p);
}

void as_pair_val_destroy(as_val * v) {
	as_pair *p = (as_pair *) v;
    if ( p->_1 ) as_val_destroy(p->_1);
    if ( p->_2 ) as_val_destroy(p->_2);
}

char * as_pair_tostring(const as_pair * p) {

    char * a = as_val_tostring(p->_1);
    size_t al = strlen(a);
    char * b = as_val_tostring(p->_2);
    size_t bl = strlen(b);
    
    size_t l = al + bl + 5;
    char * str = (char *) malloc(sizeof(char) * l);

    strcpy(str, "(");
    strcpy(str+1, a);
    strcpy(str+1+al,", ");
    strcpy(str+1+al+2, b);
    strcpy(str+1+al+2+bl,")");
    *(str+1+al+2+bl+1) = '\0';
    free(a);
    free(b);
    return str;
}

uint32_t as_pair_val_hash(const as_val *v) {
    return 0;
}

char *as_pair_val_tostring(const as_val *v) {
    return( as_pair_tostring( (as_pair *) v ) );
}

