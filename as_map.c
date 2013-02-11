#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cf_alloc.h>

#include "as_map.h"
#include "as_pair.h"

#include "internal.h"


/******************************************************************************
 * INLINES
 ******************************************************************************/

extern inline uint32_t as_map_size(const as_map * m);
extern inline as_val * as_map_get(const as_map * m, const as_val * k);
extern inline int as_map_set(as_map * m, const as_val * k, const as_val * v);
extern inline int as_map_clear(as_map * m);
extern inline as_iterator * as_map_iterator_init(as_iterator *i, const as_map * m);
extern inline as_iterator * as_map_iterator_new(const as_map * m);
extern inline int as_map_hash(as_map * m);
extern inline as_val * as_map_toval(const as_map * m) ;
extern inline as_map * as_map_fromval(const as_val * v);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void as_map_destroy(as_map * m) {
    as_util_hook(destroy, 1, m);
}

void as_map_val_destroy(as_val *v) {
	as_map *m = (as_map *) v;
    as_util_hook(destroy, 1, m);
}

uint32_t as_map_val_hash(const as_val * v) {
    return as_val_type(v) == AS_MAP ? as_map_hash((as_map *) v) : 0;
}

char * as_map_val_tostring(const as_val * v) {

    as_map * m = (as_map *) v;

    char *      buf = NULL;
    uint32_t    blk = 256;
    uint32_t    cap = blk;
    uint32_t    pos = 0;
    bool        sep = false;

    buf = (char *) malloc(sizeof(char) * cap);
    bzero(buf, sizeof(char) * cap);

    strcpy(buf, "Map(");
    pos += 4;
    
    as_iterator i;
    as_map_iterator_init(&i, m);
    while ( as_iterator_has_next(&i) ) {
        as_pair * pair = (as_pair *) as_iterator_next(&i);

        char * keystr = as_val_tostring(as_pair_1(pair));
        size_t keylen = strlen(keystr);

        char * valstr = as_val_tostring(as_pair_2(pair));
        size_t vallen = strlen(valstr);

        if ( pos + keylen + 2 + vallen + 2 >= cap ) {
            uint32_t adj = keylen+2+vallen+2 > blk ? keylen+2+vallen+2 : blk;
            buf = realloc(buf, sizeof(char) * (cap + adj));
            bzero(buf+cap, sizeof(char)*adj);
            cap += adj;
        }

        if ( sep ) {
            strcpy(buf + pos, ", ");
            pos += 2;
        }

        strncpy(buf + pos, keystr, keylen);
        strcpy(buf + pos + keylen, "->");
        strncpy(buf + pos + keylen + 2, valstr, vallen);
        pos += keylen + 2 + vallen;
        sep = true;

        free(keystr);
        keystr = NULL;
        free(valstr);
        valstr = NULL;
    }

    as_iterator_destroy(&i);

    strcpy(buf + pos, ")");
    
    return buf;
}
