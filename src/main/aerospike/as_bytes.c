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
#include <aerospike/as_bytes.h>

extern inline uint8_t * as_bytes_tobytes(const as_bytes * s);
extern inline as_val * as_bytes_toval(const as_bytes * s);
extern inline as_bytes * as_bytes_fromval(const as_val * v);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_bytes * as_bytes_init(as_bytes * v, uint8_t * s, size_t len, bool is_malloc) {
    as_val_init(&v->_, AS_BYTES, false /*is_malloc*/);
    v->value_is_malloc = is_malloc;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = s;
    v->len = len;
    v->capacity = len;
    return v;
}

as_bytes * as_bytes_empty_init(as_bytes * v, size_t len) {
    as_val_init(&v->_, AS_BYTES, false /*is_malloc*/);
    v->value_is_malloc = true;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = malloc(len);
    memset(v->value, 0, len);
    v->len = len;
    v->capacity = len;
    return v;
}

as_bytes * as_bytes_new(uint8_t * s, size_t len, bool is_malloc) {
    as_bytes * v = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&v->_, AS_BYTES, true /*is_malloc*/);
    v->value_is_malloc = is_malloc;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = s;
    v->len = len;
    v->capacity = len;
    return v;
}

as_bytes * as_bytes_empty_new(size_t len) {
    as_bytes * v = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&v->_, AS_BYTES, true /*is_malloc*/);
    v->value_is_malloc = true;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = malloc(len);
    memset(v->value, 0, len);
    v->len = len;
    v->capacity = len;
    return v;
}

void as_bytes_destroy(as_bytes * s) {
	as_val_val_destroy( (as_val *)s );
}

void as_bytes_val_destroy(as_val * v) {
	as_bytes *s = (as_bytes *) v;
	if ( s->value_is_malloc && s->value ) free(s->value);
}

size_t as_bytes_len(as_bytes * s) {
	return(s->len);
}

as_bytes_type as_bytes_get_type(const as_bytes * s) {
    return(s->type);
}

void as_bytes_set_type(as_bytes *s, as_bytes_type t) {
    s->type = t;
    return;
}

int as_bytes_get(const as_bytes * s, int index, uint8_t *buf, int buf_len) {
    if ((index < 0) || (index + buf_len > s->len)) return(-1);
    memcpy(buf, &s->value[index], buf_len);
    return(0);
}

int as_bytes_set(as_bytes * s, int index, const uint8_t *buf, int buf_len) {
    if ((index < 0) || (index + buf_len > s->len)) return(-1);
    memcpy(&s->value[index], buf, buf_len);
    return(0);
}

// create a new as_bytes, a substring of the source
as_bytes *as_bytes_slice_new(const as_bytes *src, int start_index, int end_index) {
    int len = end_index - start_index;
    if ((start_index < 0) || (start_index > src->len)) return(0);
    if ((end_index < 0) || (end_index > src->len)) return(0);
    as_bytes * v = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&v->_, AS_BYTES, true /*is_malloc*/);
    v->value_is_malloc = true;
    v->value = malloc(len);
    memcpy(v->value, &src->value[start_index], len);
    v->len = len;
    v->capacity = len;
    return(v);
}

// create a new as_bytes, a substring of the source
as_bytes *as_bytes_slice_init(as_bytes *v, const as_bytes *src, int start_index, int end_index){
    int len = end_index - start_index;
    if ((start_index < 0) || (start_index > src->len)) return(0);
    if ((end_index < 0) || (end_index > src->len)) return(0);
    as_val_init(&v->_, AS_BYTES, false /*is_malloc*/);
    v->value_is_malloc = true;
    v->value = malloc(len);
    memcpy(v->value, &src->value[start_index], len);
    v->len = len;
    v->capacity = len;
    return v;
}

int as_bytes_append(as_bytes *v, const uint8_t *buf, int buf_len) 
{
    if (buf_len < 0) return(-1);
    // not enough capacity? increase
    if (v->len + buf_len > v->capacity) {
        uint8_t *t;
        if (v->value_is_malloc == false) {
            t = malloc(v->len + buf_len);
            if (!t) return(-1);
            v->value_is_malloc = true;
            memcpy(t, v->value + v->len, v->len);
        }
        else {
            t = realloc(v->value, v->len + buf_len);
            if (!t) return(-1);
        }
        memcpy(&t[v->len], buf, buf_len);
        v->value = t;
        v->value_is_malloc = true;
        v->len = v->capacity = v->len + buf_len;
    }
    else {
        memcpy(&v->value[v->len],buf,buf_len);
        v->len += buf_len;
    }
    return(0);
}

int as_bytes_append_bytes(as_bytes *s1, as_bytes *s2) 
{
    // not enough capacity? increase
    if (s1->len + s2->len > s1->capacity) {
        uint8_t *t;
        if (s1->value_is_malloc == false) {
            t = malloc(s1->len + s2->len);
            if (!t) return(-1);
            s1->value_is_malloc = true;
            memcpy(t, s1->value + s1->len, s1->len);
            s1->value_is_malloc = true;
        }
        else {
            t = realloc(s1->value, s1->len + s2->len);
            if (!t) return(-1);
        }
        memcpy(&t[s1->len], s2->value, s2->len);
        s1->value = t;
        s1->len = s1->capacity = s1->len + s2->len;
    }
    else {
        memcpy(&s1->value[s1->len],s2->value,s2->len);
        s1->len += s2->len;
    }
    return(0);
}

int as_bytes_delete(as_bytes *v, int d_pos, int d_len)
{
    if ((d_pos < 0) || (d_pos > v->len)) return(-1);
    if (d_len < 0) return(-1);
    if (d_pos + d_len > v->len) return(-1);
    // overlapping writes require memmove
    memmove(&v->value[d_pos], &v->value[d_pos+d_len],v->len - (d_pos + d_len));
    return(0);
}

int as_bytes_set_len(as_bytes *v, int len)
{
    if (v->len == len) return(0);
    if (len > v->len) {
        if (len > v->capacity) {
            v->value = realloc(v->value, len);
            if (v->value == 0) return(-1);
            memset(v->value + v->capacity, 0, len - v->capacity);
            v->capacity = len;
        } 
        v->len = len;
    }
    v->len = len;
    return(0);
}

uint32_t as_bytes_hash(const as_bytes * s) {
    if (s->value == NULL) return(0);
    uint32_t hash = 0;
    uint8_t * str = s->value;
    int len = s->len;
    while ( --len ) {
        int c = *str++;
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

uint32_t as_bytes_val_hash(const as_val * v) {
    return as_bytes_hash((as_bytes *) v);
}

static char hex_chars[] = "0123456789ABCDEF";

char * as_bytes_val_tostring(const as_val * v) {
    as_bytes * s = (as_bytes *) v;
    if (s->value == NULL) return(NULL);
    size_t sl = as_bytes_len(s);
    if (sl == 0) {
        return( strdup("\"\"") );
    }
    size_t st = (4 * sl) + 3;
    char * str = (char *) malloc(st);
    str[0] = '\"';
    int j=1;
    for (int i=0;i<sl;i++) {
        str[j] = hex_chars[ s->value[i] >> 4 ];
        str[j+1] = hex_chars[ s->value[i] & 0xf ];
        str[j+2] = ' ';
        j += 3;
    }
    j--; // chomp
    str[j] = '\"';
    str[j+1] = 0;
    return str ;
}
