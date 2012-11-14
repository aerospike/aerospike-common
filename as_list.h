#ifndef _AS_LIST_H
#define _AS_LIST_H

#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>

typedef struct as_list_s as_list;

as_list * cons(as_val *, as_list *);

as_list * as_list_new();

int as_list_free(as_list *);

as_val * as_list_toval(const as_list *);

as_list * as_list_fromval(const as_val *);

int as_list_append(as_list *, as_val *);

as_val * as_list_get(const int i);

as_val * as_list_head(const as_list *);

as_list * as_list_tail(const as_list *);

as_iterator * as_list_iterator(const as_list *);

#endif // _AS_LIST_H