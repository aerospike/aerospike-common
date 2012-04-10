/*
 *  Citrusleaf Foundation
 *  src/ttree.c - T-tree index
 *
 *  Copyright 2012 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */

#include <string.h>
#include <stdlib.h>

#include "slist.h"


typedef struct slist_s {
	uint8_t		n_elements;
	byte		values[];
} __attribute__ ((__packed__)) slist;

int slist_create(void *v_this, void *v_tdata) {
	slist *this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	this = cf_malloc(sizeof(slist) + tdata->value_sz);
	if (! this)	return SLIST_ERR;

	this->n_elements = 0;

	memcpy(v_this, &this, sizeof(slist *));

	return SLIST_OK;
}

void slist_destroy(void *v_this) {
	cf_free(*(slist **)v_this);
}

static inline void *get_value_by_index(slist *this, uint8_t index, uint32_t value_sz) {
	return (byte *)this->values + (value_sz * index);
}

static inline slist *reallocate_up_if_necessary(slist **p_this, uint32_t value_sz, uint8_t resize_factor) {
	slist *this = *p_this;

	if (this->n_elements < resize_factor) {
		// resize up 1
		*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz * (this->n_elements + 1));
	}
	else if (this->n_elements >= resize_factor * (ffs((int)resize_factor) - 1)) {
		if (this->n_elements % resize_factor == 0) {
			// resize up by resize_factor
			*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz * (this->n_elements + resize_factor));
		}
	}
	else {
		uint8_t f = 2;
		uint8_t c = resize_factor;

		while (c < this->n_elements) {
			c += f;
			if (c % resize_factor == 0) {
				f *= 2;
			}
		}

		if (c == this->n_elements) {
			// resize up by f
			*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz * (this->n_elements + f));
		}
	}

	return *p_this;
}

static inline slist *reallocate_down(slist **p_this, uint32_t value_sz, uint8_t resize_factor) {
	slist *this = *p_this;

	if (! this->n_elements) {
		// resize to leave room for one element
		// TODO - Change this behavior?
		*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz);
	}
	else if (this->n_elements < resize_factor) {
		// resize to size of n_elements
		*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz * this->n_elements);
	}
	else if (this->n_elements >= resize_factor * (ffs((int)resize_factor) - 1)) {
		// resize to n_elements rounded up to nearest resize_factor
		*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz * ((this->n_elements + resize_factor - 1) / resize_factor * resize_factor));
	}
	else {
		uint8_t f = 2;
		uint8_t c = resize_factor;

		while (c < this->n_elements) {
			c += f;
			if (c % resize_factor == 0) {
				f *= 2;
			}
		}

		// resize to n_elements rounded up to nearest f
		*p_this = cf_realloc(*p_this, sizeof(slist) + value_sz * ((this->n_elements + f - 1) / f * f));
	}

	return *p_this;
}

int slist_put(void *v_this, void *key, void *value, void *v_tdata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	if (! this->n_elements) {
		memcpy(this->values, value, tdata->value_sz);
		this->n_elements++;
		return SLIST_OK;
	}

	int16_t first = 0;
	int16_t last = this->n_elements - 1;

	void *cmp_value = this->values;

	while (first <= last) {
		uint8_t middle = (first + last) >> 1;
		cmp_value = get_value_by_index(this, middle, tdata->value_sz);
		int cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			first = middle;
			break;
		}
		else if (cmp < 0) {
			last = middle - 1;
		}
		else {
			first = middle + 1;
		}
	}

	this = reallocate_up_if_necessary((slist **)v_this, tdata->value_sz, tdata->resize_factor);

	cmp_value = get_value_by_index(this, first, tdata->value_sz);
	memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (this->n_elements - first));
	memcpy(cmp_value, value, tdata->value_sz);
	this->n_elements++;

	return SLIST_OK;
}

int slist_put_unique(void *v_this, void *key, void *value, void *v_tdata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	if (! this->n_elements) {
		memcpy(this->values, value, tdata->value_sz);
		this->n_elements++;
		return SLIST_OK;
	}

	int16_t first = 0;
	int16_t last = this->n_elements - 1;

	void *cmp_value = this->values;

	while (first <= last) {
		uint8_t middle = (first + last) >> 1;
		cmp_value = get_value_by_index(this, middle, tdata->value_sz);
		int cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			return SLIST_ERR_FOUND;
		}
		else if (cmp < 0) {
			last = middle - 1;
		}
		else {
			first = middle + 1;
		}
	}

	this = reallocate_up_if_necessary((slist **)v_this, tdata->value_sz, tdata->resize_factor);

	cmp_value = get_value_by_index(this, first, tdata->value_sz);
	memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (this->n_elements - first));
	memcpy(cmp_value, value, tdata->value_sz);
	this->n_elements++;

	return SLIST_OK;
}

int slist_get(void *v_this, void *key, void *value, void *v_tdata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	if (! this->n_elements) {
		return SLIST_ERR_NOTFOUND;
	}

	int16_t first = 0;
	int16_t last = this->n_elements - 1;

	while (first <= last) {
		uint8_t middle = (first + last) >> 1;
		void *cmp_value = get_value_by_index(this, middle, tdata->value_sz);
		int cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			memcpy(value, cmp_value, tdata->value_sz);
			return SLIST_OK;
		}
		else if (cmp < 0) {
			last = middle - 1;
		}
		else {
			first = middle + 1;
		}
	}

	return SLIST_ERR_NOTFOUND;
}

int slist_delete(void *v_this, void *key, void *v_tdata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	if (! this->n_elements) {
		return SLIST_ERR_NOTFOUND;
	}

	int16_t first = 0;
	int16_t last = this->n_elements - 1;

	while (first <= last) {
		uint8_t middle = (first + last) >> 1;
		void *cmp_value = get_value_by_index(this, middle, tdata->value_sz);
		int cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (this->n_elements - middle - 1));
			this->n_elements--;
			reallocate_down((slist **)v_this, tdata->value_sz, tdata->resize_factor);
			return SLIST_OK;
		}
		else if (cmp < 0) {
			last = middle - 1;
		}
		else {
			first = middle + 1;
		}
	}

	return SLIST_ERR_NOTFOUND;
}

int slist_get_and_delete(void *v_this, void *key, void *value, void *v_tdata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	if (! this->n_elements) {
		return SLIST_ERR_NOTFOUND;
	}

	int16_t first = 0;
	int16_t last = this->n_elements - 1;

	while (first <= last) {
		uint8_t middle = (first + last) >> 1;
		void *cmp_value = get_value_by_index(this, middle, tdata->value_sz);
		int cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			memcpy(value, cmp_value, tdata->value_sz);
			memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (this->n_elements - middle - 1));
			this->n_elements--;
			this = reallocate_down((slist **)v_this, tdata->value_sz, tdata->resize_factor);
			return SLIST_OK;
		}
		else if (cmp < 0) {
			last = middle - 1;
		}
		else {
			first = middle + 1;
		}
	}

	return SLIST_ERR_NOTFOUND;
}

int slist_reduce(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	void *value = this->values;
	void *end_value = get_value_by_index(this, this->n_elements, tdata->value_sz);

	while (value != end_value) {
		reduce_fn(value, value, udata);
		value = (byte *)value + tdata->value_sz;
	}

	return SLIST_OK;
}

int slist_reduce_delete(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata) {
	slist *this = *(slist **)v_this;
	slist_tdata *tdata = (slist_tdata *)v_tdata;

	uint8_t old_n_elements = this->n_elements;

	void *value = this->values;
	void *end_value = get_value_by_index(this, this->n_elements, tdata->value_sz);

	while (value != end_value) {
		if (SLIST_REDUCE_DELETE == reduce_fn(value, value, udata)) {
			memmove(value, (byte *)value + tdata->value_sz, (byte *)end_value - (byte *)value - tdata->value_sz);
			this->n_elements--;
			end_value = (byte *)end_value - tdata->value_sz;
		}
		else {
			value = (byte *)value + tdata->value_sz;
		}
	}

	if (this->n_elements != old_n_elements) {
		this = reallocate_down((slist **)v_this, tdata->value_sz, tdata->resize_factor);
	}

	return SLIST_OK;
}

uint32_t slist_get_size(void *v_this) {
	return (uint32_t)((*(slist **)v_this)->n_elements);
}


static int delete_odds(void *key, void *value, void *udata) {
	int v = *(int *)value;
	if (v % 2) {
		return TTREE_REDUCE_DELETE;
	}

	return TTREE_OK;
}

static int delete_evens(void *key, void *value,  void *udata) {
	if (*(int *)value % 2) {
		return TTREE_OK;
	}

	return TTREE_REDUCE_DELETE;
}


void slist_test() {
	slist_tdata tdata;
	tdata.key_sz = sizeof(int);
	tdata.value_sz = sizeof(int);
	tdata.resize_factor = 4;

	slist *this = 0;
	slist_create((void *)&this, &tdata);

	int put_count = 0;
	int odds = 0;
	int evens = 0;

	srand(22);
	for (int i = 0; i < 200; i++) {
		int r = rand() % 100;

		if (rand() % 2) {
			slist_get(&this, &r, &r, &tdata);
		}
		else {
			slist_put(&this, &r, &r, &tdata);
			put_count++;

			if (r % 2) {
				odds++;
			}
			else {
				evens++;
			}
		}
	}

	cf_info(AS_AS, "size = %d | put_count %d | odds %d | evens %d", slist_get_size(&this), put_count, odds, evens);

	srand(22);
	for (int i = 0; i < 10000; i++) {
		int r = rand() % 100;

		if (rand() % 2) {
			continue;
		}
		else {
			slist_delete(&this, &r, &tdata);
		}
	}

	/*
	slist_reduce_delete(&this, delete_odds, &udata, NULL);

	cf_info(AS_AS, "size = %d | put_count %d | odds %d | evens %d", slist_get_size(&this), put_count, odds, evens);

	slist_reduce_delete(&this, delete_evens, &udata, NULL);
	*/

	cf_info(AS_AS, "size = %d | put_count %d | odds %d | evens %d", slist_get_size(&this), put_count, odds, evens);

	slist_destroy(&this);

	cf_info(AS_AS, "success!");

	_exit(0);
}
