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

#include <citrusleaf/alloc.h>
#include <aerospike/as_bytes.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void          as_bytes_destroy(as_bytes * s);

extern inline uint8_t *     as_bytes_tobytes(const as_bytes * s);

extern inline as_val *      as_bytes_toval(const as_bytes * s);
extern inline as_bytes *    as_bytes_fromval(const as_val * v);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const char hex_chars[] = "0123456789ABCDEF";

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_bytes * as_bytes_init(as_bytes * v, uint8_t * s, uint32_t len, bool free) {
    as_val_init(&v->_, AS_BYTES, false /*is_malloc*/);
    v->free = free;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = s;
    v->len = len;
    v->capacity = len;
    return v;
}

as_bytes * as_bytes_empty_init(as_bytes * v, uint32_t len) {
    as_val_init(&v->_, AS_BYTES, false /*is_malloc*/);
    v->free = true;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = malloc(len);
    memset(v->value, 0, len);
    v->len = len;
    v->capacity = len;
    return v;
}

as_bytes * as_bytes_new(uint8_t * s, uint32_t len, bool free) {
    as_bytes * v = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&v->_, AS_BYTES, true /*is_malloc*/);
    v->free = free;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = s;
    v->len = len;
    v->capacity = len;
    return v;
}

as_bytes * as_bytes_empty_new(uint32_t len) {
    as_bytes * v = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&v->_, AS_BYTES, true /*is_malloc*/);
    v->free = true;
    v->type = AS_BYTES_TYPE_BLOB;
    v->value = malloc(len);
    memset(v->value, 0, len);
    v->len = len;
    v->capacity = len;
    return v;
}

uint32_t as_bytes_len(as_bytes * s) {
	return s->len;
}

as_bytes_type as_bytes_get_type(const as_bytes * s) {
    return s->type;
}

void as_bytes_set_type(as_bytes *s, as_bytes_type t) {
    s->type = t;
}

int as_bytes_get(const as_bytes * s, uint32_t index, uint8_t * buf, uint32_t buf_len) {
    if ((index < 0) || (index + buf_len > s->len)) return(-1);
    memcpy(buf, &s->value[index], buf_len);
    return 0;
}

int as_bytes_set(as_bytes * s, uint32_t index, const uint8_t * buf, uint32_t buf_len) {
    if ((index < 0) || (index + buf_len > s->len)) return(-1);
    memcpy(&s->value[index], buf, buf_len);
    return 0;
}

// create a new as_bytes, a substring of the source
as_bytes * as_bytes_slice_new(const as_bytes * src, uint32_t start, uint32_t end) {
    if ( start > src->len || end > src->len ) {
    	return 0;
    }
    int len = end - start;

    as_bytes * dest = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&dest->_, AS_BYTES, true);
    dest->free = true;
    dest->value = malloc(len);
    memcpy(dest->value, &src->value[start], len);
    dest->len = len;
    dest->capacity = len;
    return dest;
}

// create a new as_bytes, a substring of the source
as_bytes * as_bytes_slice_init(as_bytes * dest, const as_bytes * src, uint32_t start, uint32_t end) {
    if ( start > src->len || end > src->len ) {
    	return 0;
    }
    int len = end - start;

    as_val_init(&dest->_, AS_BYTES, false);
    dest->free = true;
    dest->value = malloc(len);
    memcpy(dest->value, &src->value[start], len);
    dest->len = len;
    dest->capacity = len;
    return dest;
}

int as_bytes_append(as_bytes * v, const uint8_t * buf, uint32_t buf_len)  {
    if (buf_len < 0) return(-1);
    // not enough capacity? increase
    if (v->len + buf_len > v->capacity) {
        uint8_t *t;
        if (v->free == false) {
            t = malloc(v->len + buf_len);
            if (!t) return(-1);
            v->free = true;
            memcpy(t, v->value + v->len, v->len);
        }
        else {
            t = realloc(v->value, v->len + buf_len);
            if (!t) return(-1);
        }
        memcpy(&t[v->len], buf, buf_len);
        v->value = t;
        v->free = true;
        v->len = v->capacity = v->len + buf_len;
    }
    else {
        memcpy(&v->value[v->len],buf,buf_len);
        v->len += buf_len;
    }
    return 0;
}

int as_bytes_append_bytes(as_bytes * s1, as_bytes * s2) {
    // not enough capacity? increase
    if (s1->len + s2->len > s1->capacity) {
        uint8_t *t;
        if (s1->free == false) {
            t = malloc(s1->len + s2->len);
            if (!t) return(-1);
            s1->free = true;
            memcpy(t, s1->value + s1->len, s1->len);
            s1->free = true;
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
    return 0;
}

int as_bytes_delete(as_bytes * v, uint32_t pos, uint32_t len) {
    if ((pos < 0) || (pos > v->len)) {
    	return -1;
    }
    if (len < 0) {
    	return -1;
    }
    if (pos + len > v->len) {
    	return -1;
    }
    // overlapping writes require memmove
    memmove(&v->value[pos], &v->value[pos+len],v->len - (pos + len));
    return 0;
}

int as_bytes_set_len(as_bytes * v, uint32_t len) {
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
    return 0;
}



void as_bytes_val_destroy(as_val * v) {
    as_bytes * b = as_bytes_fromval(v);
    if ( b && b->free && b->value ) {
        free(b->value);
    }
}

uint32_t as_bytes_val_hashcode(const as_val * v) {
    as_bytes * bytes = as_bytes_fromval(v);
    if ( bytes == NULL || bytes->value == NULL ) return 0;
    uint32_t hash = 0;
    uint8_t * buf = bytes->value;
    int len = bytes->len;
    while ( --len ) {
        int b = *buf++;
        hash = b + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

char * as_bytes_val_tostring(const as_val * v) {
    as_bytes * s = (as_bytes *) v;
    if (s->value == NULL) return(NULL);
    uint32_t sl = as_bytes_len(s);
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
    return str;
}
