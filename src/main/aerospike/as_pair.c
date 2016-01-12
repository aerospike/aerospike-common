/* 
 * Copyright 2008-2016 Aerospike, Inc.
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
#include <aerospike/as_pair.h>
#include <aerospike/as_util.h>
#include <citrusleaf/alloc.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

as_pair * as_pair_init(as_pair * pair, as_val * _1, as_val * _2)
{
	if ( !pair ) return pair;

	as_val_init((as_val *) pair, AS_PAIR, false);
	pair->_1 = _1;
	pair->_2 = _2;
	return pair;
}

as_pair * as_pair_new(as_val * _1, as_val * _2)
{
	as_pair * pair = (as_pair *) cf_malloc(sizeof(as_pair));
	if ( !pair ) return pair;

	as_val_init((as_val *) pair, AS_PAIR, true);
	pair->_1 = _1;
	pair->_2 = _2;
	return pair;
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

void as_pair_val_destroy(as_val * v)
{
	as_pair * p = (as_pair *) v;
	if ( p->_1 ) as_val_destroy(p->_1);
	if ( p->_2 ) as_val_destroy(p->_2);
}

uint32_t as_pair_val_hashcode(const as_val * v)
{
	return 0;
}

char *as_pair_val_tostring(const as_val * v)
{
	as_pair * p = as_pair_fromval(v);
	if ( p == NULL ) return NULL;

	char * a = as_val_tostring(p->_1);
	size_t al = strlen(a);
	char * b = as_val_tostring(p->_2);
	size_t bl = strlen(b);
	
	size_t l = al + bl + 5;
	char * str = (char *) cf_malloc(sizeof(char) * l);
	if (!str) return str;

	strcpy(str, "(");
	strcpy(str+1, a);
	strcpy(str+1+al,", ");
	strcpy(str+1+al+2, b);
	strcpy(str+1+al+2+bl,")");
	*(str+1+al+2+bl+1) = '\0';
	
	cf_free(a);
	cf_free(b);

	return str;
}

