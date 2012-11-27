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

#include "ttree.h"


#define TTREE_LOCATION_HEAD 0
#define TTREE_LOCATION_LEFT 1
#define TTREE_LOCATION_RIGHT 2


typedef struct ttree_s {
	uint8_t		n_elements;
	struct ttree_s	*parent;
	struct ttree_s	*left;
	struct ttree_s	*right;
	struct ttree_s	*next;
	byte			values[];
} __attribute__ ((__packed__)) ttree_node;

typedef struct {
	ttree_node		*head;
	uint16_t		left_depth;
	uint16_t		right_depth;
} __attribute__ ((__packed__)) cf_index_ttree;

typedef struct {
	char		*head;
	char 		*pos;
	uint32_t	count;
} ttree_dump_s;


int ttree_create_node(ttree_node **p_node, ttree_tdata *tdata) {
	ttree_node *node;

	node = cf_malloc(sizeof(ttree_node) + (tdata->value_sz * tdata->max_row_elements));
	if (! node)	return TTREE_ERR;

	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	node->next = NULL;
	node->n_elements = 0;

	*p_node = node;

	return TTREE_OK;
}

int ttree_create(void *v_this, void *v_tdata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	int rv = ttree_create_node(&this->head, tdata);

	if (TTREE_OK != rv) {
		return rv;
	}

	this->left_depth = 0;
	this->right_depth = 0;

	return TTREE_OK;
}

void ttree_destroy_node(ttree_node *node) {
	cf_free(node);
}

void ttree_destroy(void *v_this) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	if (! node->n_elements) {
		return;
	}

	while (node->left) {
		node = node->left;
	}

	do {
		ttree_node *next = node->next;
		ttree_destroy_node(node);
		node = next;
	} while (node);

	return;
}

static inline void *get_value_idx_1(ttree_node *node, uint32_t value_sz) {
	return (byte *)node->values + value_sz;
}

static inline void *get_value_by_index(ttree_node *node, uint8_t index, uint32_t value_sz) {
	return (byte *)node->values + (value_sz * index);
}

static inline ttree_node *get_greatest_lower_bound(ttree_node *node) {
	if (! node->left) {
		return NULL;
	}

	node = node->left;

	while (node->right) {
		node = node->right;
	}

	return node;
}

static inline ttree_node *get_least_upper_bound(ttree_node *node) {
	if (! node->right) {
		return NULL;
	}

	node = node->right;

	while (node->left) {
		node = node->left;
	}

	return node;
}

static inline void rotate_left(cf_index_ttree *this) {
	ttree_node *root = this->head;
	ttree_node *pivot = root->right;

	root->right = pivot->left;

	if (pivot->left) {
		pivot->left->parent = root;
	}

	pivot->left = root;
	root->parent = pivot;

	pivot->parent = NULL;

	this->head = pivot;

	this->left_depth++;
	this->right_depth--;
}

static inline void rotate_right(cf_index_ttree *this) {
	ttree_node *root = this->head;
	ttree_node *pivot = root->left;

	root->left = pivot->right;

	if (pivot->right) {
		pivot->right->parent = root;
	}

	pivot->right = root;
	root->parent = pivot;

	pivot->parent = NULL;

	this->head = pivot;

	this->left_depth--;
	this->right_depth++;
}

static inline void check_depth_and_rotate(cf_index_ttree *this, uint8_t location, uint16_t depth) {
	switch(location) {
		case TTREE_LOCATION_LEFT:
			if (depth > this->left_depth) {
				this->left_depth = depth;

				if (this->left_depth == this->right_depth + 2) {
					rotate_right(this);
				}
			}

			break;

		case TTREE_LOCATION_RIGHT:
			if (depth > this->right_depth) {
				this->right_depth = depth;

				if (this->right_depth == this->left_depth + 2) {
					rotate_left(this);
				}
			}

			break;

		default:
			break;
	}
}

static inline ttree_node *get_prev_node(ttree_node *node) {
	while (node) {
		if (node->parent && node->parent->right == node) {
			return node->parent;
		}

		node = node->parent;
	}

	return NULL;
}

int ttree_put(void *v_this, void *key, void *value, void *v_tdata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		memcpy(node->values, value, tdata->value_sz);
		node->n_elements++;

		return TTREE_OK;
	}

	uint16_t depth = 0;
	uint8_t location = TTREE_LOCATION_HEAD;

	while (true) {
		// First Value
		int cmp = memcmp(key, node->values, tdata->key_sz);

		if (! cmp) {
			ttree_node *greatest_lower_bound = get_greatest_lower_bound(node);

			if (greatest_lower_bound && greatest_lower_bound->n_elements != tdata->max_row_elements) {
				// copy to end of greatest lower bound
				memcpy(get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), value, tdata->value_sz);
				greatest_lower_bound->n_elements++;

				return TTREE_OK;
			}

			if (node->n_elements != tdata->max_row_elements) {
				// put at head of node
				memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * node->n_elements);
				memcpy(node->values, value, tdata->value_sz);
				node->n_elements++;

				return TTREE_OK;
			}

			if (node->next && node->next->n_elements != tdata->max_row_elements) {
				// put last value in next and put node value at the head of node
				memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
				memcpy(node->next->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
				node->next->n_elements++;

				memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (node->n_elements - 1));
				memcpy(node->values, value, tdata->value_sz);

				return TTREE_OK;
			}

			if (greatest_lower_bound) {
				// create right of greatest lower bound
				ttree_create_node(&greatest_lower_bound->right, tdata);
				greatest_lower_bound->right->parent = greatest_lower_bound;
				greatest_lower_bound->right->next = node;
				greatest_lower_bound->next = greatest_lower_bound->right;

				memcpy(greatest_lower_bound->right->values, value, tdata->value_sz);
				greatest_lower_bound->right->n_elements++;

				check_depth_and_rotate(this, location, depth + 1);

				return TTREE_OK;
			}

			ttree_create_node(&node->left, tdata);
			node->left->parent = node;
			node->left->next = node;


			ttree_node *prev = get_prev_node(node);

			if (prev) {
				prev->next = node->left;
			}

			memcpy(node->left->values, value, tdata->value_sz);
			node->left->n_elements++;

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_LEFT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (cmp < 0) {
			if (node->left) {
				node = node->left;
				depth++;

				if (location == TTREE_LOCATION_HEAD) {
					location = TTREE_LOCATION_LEFT;
				}

				continue;
			}

			if (node->n_elements != tdata->max_row_elements) {
				// put at head of node
				memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * node->n_elements);
				memcpy(node->values, value, tdata->value_sz);
				node->n_elements++;

				return TTREE_OK;
			}

			if (node->next && node->next->n_elements != tdata->max_row_elements) {
				// put last value in next and put node value at the head of node
				memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
				memcpy(node->next->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
				node->next->n_elements++;

				memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (node->n_elements - 1));
				memcpy(node->values, value, tdata->value_sz);

				return TTREE_OK;
			}

			ttree_create_node(&node->left, tdata);
			node->left->parent = node;
			node->left->next = node;

			ttree_node *prev = get_prev_node(node);

			if (prev) {
				prev->next = node->left;
			}

			memcpy(node->left->values, value, tdata->value_sz);
			node->left->n_elements++;

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_LEFT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (node->n_elements == 1) {
			memcpy(get_value_idx_1(node, tdata->value_sz), value, tdata->value_sz);
			node->n_elements++;

			return TTREE_OK;
		}

		// Last Value
		void *cmp_value = get_value_by_index(node, node->n_elements - 1, tdata->value_sz);
		cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (cmp >= 0) {
			if (node->n_elements != tdata->max_row_elements) {
				// put at the end of node
				memcpy(get_value_by_index(node, node->n_elements, tdata->value_sz), value, tdata->value_sz);
				node->n_elements++;

				return TTREE_OK;
			}

			if (node->right) {
				node = node->right;
				depth++;

				if (location == TTREE_LOCATION_HEAD) {
					location = TTREE_LOCATION_RIGHT;
				}

				continue;
			}

			ttree_node *greatest_lower_bound = get_greatest_lower_bound(node);

			if (node->next && node->next->n_elements != tdata->max_row_elements) {
				// put at head of next
				memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
				memcpy(node->next->values, value, tdata->value_sz);
				node->next->n_elements++;

				return TTREE_OK;
			}

			if (greatest_lower_bound && greatest_lower_bound->n_elements != tdata->max_row_elements) {
				// put first element at end of greatest_lower_bound
				memcpy(get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), node->values, tdata->value_sz);
				greatest_lower_bound->n_elements++;

				memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * (node->n_elements - 1));
				memcpy(get_value_by_index(node, node->n_elements - 1, tdata->value_sz), value, tdata->value_sz);

				return TTREE_OK;
			}

			// create new node right and insert
			ttree_create_node(&node->right, tdata);
			node->right->parent = node;
			node->right->next = node->next;
			node->next = node->right;

			memcpy(node->right->values, value, tdata->value_sz);
			node->right->n_elements++;

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_RIGHT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (node->n_elements == 2) {
			// stick the value in between the two
			void *value_idx_1 = get_value_idx_1(node, tdata->value_sz);
			memmove(get_value_by_index(node, 2, tdata->value_sz), value_idx_1, tdata->value_sz);
			memcpy(value_idx_1, value, tdata->value_sz);
			node->n_elements++;

			return TTREE_OK;
		}

		// Don't search the first and last elements.
		int16_t first = 0;
		int16_t last = node->n_elements - 3;

		while (first <= last) {
			uint8_t middle = (first + last) >> 1;
			cmp_value = get_value_by_index(node, middle + 1, tdata->value_sz);
			cmp = memcmp(key, cmp_value, tdata->key_sz);
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

		if (cmp > 0) {
			cmp_value = (byte *)cmp_value + tdata->value_sz;
		}

		// Standard case
		if (node->n_elements != tdata->max_row_elements) {
			memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (node->n_elements - first - 1));
			memcpy(cmp_value, value, tdata->value_sz);
			node->n_elements++;

			return TTREE_OK;
		}

		// Block out of space cases
		ttree_node *greatest_lower_bound = get_greatest_lower_bound(node);

		if (node->next && node->next->n_elements != tdata->max_row_elements) {
			// put last element at the head of next
			memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
			memcpy(node->next->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
			node->next->n_elements++;

			memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (node->n_elements - first - 2));
			memcpy(cmp_value, value, tdata->value_sz);

			return TTREE_OK;
		}

		if (greatest_lower_bound && greatest_lower_bound->n_elements != tdata->max_row_elements) {
			// put first element at end of greatest lower bound
			memcpy(get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), node->values, tdata->value_sz);
			greatest_lower_bound->n_elements++;

			memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * first);
			memcpy((byte *)cmp_value - tdata->value_sz, value, tdata->value_sz);

			return TTREE_OK;
		}

		if (! node->right) {
			ttree_create_node(&node->right, tdata);
			node->right->parent = node;
			node->right->next = node->next;
			node->next = node->right;

			memcpy(node->right->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
			node->right->n_elements++;

			memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (node->n_elements - first - 2));

			memcpy(cmp_value, value, tdata->value_sz);

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_RIGHT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (greatest_lower_bound) {
			// create right of greatest lower bound
			ttree_create_node(&greatest_lower_bound->right, tdata);
			greatest_lower_bound->right->parent = greatest_lower_bound;
			greatest_lower_bound->right->next = node;
			greatest_lower_bound->next = greatest_lower_bound->right;

			memcpy(greatest_lower_bound->right->values, node->values, tdata->value_sz);
			greatest_lower_bound->right->n_elements++;

			memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * first);
			memcpy((byte *)cmp_value - tdata->value_sz, value, tdata->value_sz);

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		// create left
		ttree_create_node(&node->left, tdata);
		node->left->parent = node;
		node->left->next = node;

		ttree_node *prev = get_prev_node(node);

		if (prev) {
			prev->next = node->left;
		}

		memcpy(node->left->values, node->values, tdata->value_sz);
		node->left->n_elements++;

		memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * first);
		memcpy((byte *)cmp_value - tdata->value_sz, value, tdata->value_sz);

		if (location == TTREE_LOCATION_HEAD) {
			location = TTREE_LOCATION_LEFT;
		}

		check_depth_and_rotate(this, location, depth + 1);

		return TTREE_OK;
	}

	// Will never reach here.
	return TTREE_ERR_NOTFOUND;
}

int ttree_put_unique(void *v_this, void *key, void *value, void *v_tdata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		memcpy(node->values, value, tdata->value_sz);
		node->n_elements++;

		return TTREE_OK;
	}

	uint16_t depth = 0;
	uint8_t location = TTREE_LOCATION_HEAD;

	while (true) {
		// First Value
		int cmp = memcmp(key, node->values, tdata->key_sz);

		if (! cmp) {
			return TTREE_ERR_FOUND;
		}

		if (cmp < 0) {
			if (node->left) {
				node = node->left;
				depth++;

				if (location == TTREE_LOCATION_HEAD) {
					location = TTREE_LOCATION_LEFT;
				}

				continue;
			}

			if (node->n_elements != tdata->max_row_elements) {
				// put at head of node
				memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * node->n_elements);
				memcpy(node->values, value, tdata->value_sz);
				node->n_elements++;

				return TTREE_OK;
			}

			if (node->next && node->next->n_elements != tdata->max_row_elements) {
				// put last value in next and put node value at the head of node
				memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
				memcpy(node->next->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
				node->next->n_elements++;

				memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (node->n_elements - 1));
				memcpy(node->values, value, tdata->value_sz);

				return TTREE_OK;
			}

			ttree_create_node(&node->left, tdata);
			node->left->parent = node;
			node->left->next = node;

			ttree_node *prev = get_prev_node(node);

			if (prev) {
				prev->next = node->left;
			}

			memcpy(node->left->values, value, tdata->value_sz);
			node->left->n_elements++;

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_LEFT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (node->n_elements == 1) {
			memcpy(get_value_idx_1(node, tdata->value_sz), value, tdata->value_sz);
			node->n_elements++;

			return TTREE_OK;
		}

		// Last Value
		void *cmp_value = get_value_by_index(node, node->n_elements - 1, tdata->value_sz);
		cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			return TTREE_ERR_FOUND;
		}

		if (cmp > 0) {
			if (node->n_elements != tdata->max_row_elements) {
				// put at the end of node
				memcpy(get_value_by_index(node, node->n_elements, tdata->value_sz), value, tdata->value_sz);
				node->n_elements++;

				return TTREE_OK;
			}

			if (node->right) {
				node = node->right;
				depth++;

				if (location == TTREE_LOCATION_HEAD) {
					location = TTREE_LOCATION_RIGHT;
				}

				continue;
			}

			ttree_node *greatest_lower_bound = get_greatest_lower_bound(node);

			if (node->next && node->next->n_elements != tdata->max_row_elements) {
				// put at head of next
				memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
				memcpy(node->next->values, value, tdata->value_sz);
				node->next->n_elements++;

				return TTREE_OK;
			}

			if (greatest_lower_bound && greatest_lower_bound->n_elements != tdata->max_row_elements) {
				// put first element at end of greatest_lower_bound
				memcpy(get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), node->values, tdata->value_sz);
				greatest_lower_bound->n_elements++;

				memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * (node->n_elements - 1));
				memcpy(get_value_by_index(node, node->n_elements - 1, tdata->value_sz), value, tdata->value_sz);

				return TTREE_OK;
			}

			// create new node right and insert
			ttree_create_node(&node->right, tdata);
			node->right->parent = node;
			node->right->next = node->next;
			node->next = node->right;

			memcpy(node->right->values, value, tdata->value_sz);
			node->right->n_elements++;

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_RIGHT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (node->n_elements == 2) {
			// stick the value in between the two
			void *value_idx_1 = get_value_idx_1(node, tdata->value_sz);
			memmove(get_value_by_index(node, 2, tdata->value_sz), value_idx_1, tdata->value_sz);
			memcpy(value_idx_1, value, tdata->value_sz);
			node->n_elements++;

			return TTREE_OK;
		}

		// Don't search the first and last elements.
		int16_t first = 0;
		int16_t last = node->n_elements - 3;

		while (first <= last) {
			uint8_t middle = (first + last) >> 1;
			cmp_value = get_value_by_index(node, middle + 1, tdata->value_sz);
			cmp = memcmp(key, cmp_value, tdata->key_sz);
			if (! cmp) {
				return TTREE_ERR_FOUND;
			}
			else if (cmp < 0) {
				last = middle - 1;
			}
			else {
				first = middle + 1;
			}
		}

		if (cmp > 0) {
			cmp_value = (byte *)cmp_value + tdata->value_sz;
		}

		// Standard case
		if (node->n_elements != tdata->max_row_elements) {
			memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (node->n_elements - first - 1));
			memcpy(cmp_value, value, tdata->value_sz);
			node->n_elements++;

			return TTREE_OK;
		}

		// Block out of space cases
		ttree_node *greatest_lower_bound = get_greatest_lower_bound(node);

		if (node->next && node->next->n_elements != tdata->max_row_elements) {
			// put last element at the head of next
			memmove(get_value_idx_1(node->next, tdata->value_sz), node->next->values, tdata->value_sz * node->next->n_elements);
			memcpy(node->next->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
			node->next->n_elements++;

			memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (node->n_elements - first - 2));
			memcpy(cmp_value, value, tdata->value_sz);

			return TTREE_OK;
		}

		if (greatest_lower_bound && greatest_lower_bound->n_elements != tdata->max_row_elements) {
			// put first element at end of greatest lower bound
			memcpy(get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), node->values, tdata->value_sz);
			greatest_lower_bound->n_elements++;

			memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * first);
			memcpy((byte *)cmp_value - tdata->value_sz, value, tdata->value_sz);

			return TTREE_OK;
		}

		if (! node->right) {
			ttree_create_node(&node->right, tdata);
			node->right->parent = node;
			node->right->next = node->next;
			node->next = node->right;

			memcpy(node->right->values, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);
			node->right->n_elements++;

			memmove((byte *)cmp_value + tdata->value_sz, cmp_value, tdata->value_sz * (node->n_elements - first - 2));

			memcpy(cmp_value, value, tdata->value_sz);

			if (location == TTREE_LOCATION_HEAD) {
				location = TTREE_LOCATION_RIGHT;
			}

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		if (greatest_lower_bound) {
			// create right of greatest lower bound
			ttree_create_node(&greatest_lower_bound->right, tdata);
			greatest_lower_bound->right->parent = greatest_lower_bound;
			greatest_lower_bound->right->next = node;
			greatest_lower_bound->next = greatest_lower_bound->right;

			memcpy(greatest_lower_bound->right->values, node->values, tdata->value_sz);
			greatest_lower_bound->right->n_elements++;

			memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * first);
			memcpy((byte *)cmp_value - tdata->value_sz, value, tdata->value_sz);

			check_depth_and_rotate(this, location, depth + 1);

			return TTREE_OK;
		}

		// create left
		ttree_create_node(&node->left, tdata);
		node->left->parent = node;
		node->left->next = node;

		ttree_node *prev = get_prev_node(node);

		if (prev) {
			prev->next = node->left;
		}

		memcpy(node->left->values, node->values, tdata->value_sz);
		node->left->n_elements++;

		memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * first);
		memcpy((byte *)cmp_value - tdata->value_sz, value, tdata->value_sz);

		if (location == TTREE_LOCATION_HEAD) {
			location = TTREE_LOCATION_LEFT;
		}

		check_depth_and_rotate(this, location, depth + 1);

		return TTREE_OK;
	}

	// Will never reach here.
	return TTREE_ERR_NOTFOUND;
}

int ttree_get(void *v_this, void *key, void *value, void *v_tdata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		value = NULL;
		return TTREE_ERR_NOTFOUND;
	}

	while (true) {
		// First Value
		int cmp = memcmp(key, node->values, tdata->key_sz);

		if (! cmp) {
			memcpy(value, node->values, tdata->value_sz);
			return TTREE_OK;
		}

		if (cmp < 0) {
			if (! node->left) {
				value = NULL;
				return TTREE_ERR_NOTFOUND;
			}

			node = node->left;
			continue;
		}

		if (node->n_elements == 1) {
			if (! node->right) {
				value = NULL;
				return TTREE_ERR_NOTFOUND;
			}

			node = node->right;
			continue;
		}

		// Last Value
		void *cmp_value = get_value_by_index(node, node->n_elements - 1, tdata->value_sz);
		cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			memcpy(value, cmp_value, tdata->value_sz);
			return TTREE_OK;
		}

		if (cmp > 0) {
			if (! node->right) {
				value = NULL;
				return TTREE_ERR_NOTFOUND;
			}

			node = node->right;
			continue;
		}

		if (node->n_elements == 2) {
			value = NULL;
			return TTREE_ERR_NOTFOUND;
		}

		// Don't search the first and last elements.
		int16_t first = 0;
		int16_t last = node->n_elements - 3;

		while (first <= last) {
			uint8_t middle = (first + last) >> 1;
			cmp_value = get_value_by_index(node, middle + 1, tdata->value_sz);
			cmp = memcmp(key, cmp_value, tdata->key_sz);
			if (! cmp) {
				memcpy(value, cmp_value, tdata->value_sz);
				return TTREE_OK;
			}
			else if (cmp < 0) {
				last = middle - 1;
			}
			else {
				first = middle + 1;
			}
		}

		value = NULL;
		return TTREE_ERR_NOTFOUND;
	}

	// Will never reach here.
	return TTREE_ERR_NOTFOUND;
}

uint16_t get_depths_helper(ttree_node *node, uint16_t depth) {
	if (! node) {
		return depth;
	}

	depth++;
	uint16_t left_depth = get_depths_helper(node->left, depth);
	uint16_t right_depth = get_depths_helper(node->right, depth);

	return left_depth > right_depth ? left_depth : right_depth;
}

void update_metadata(cf_index_ttree *this) {
	this->left_depth = get_depths_helper(this->head->left, 0);
	this->right_depth = get_depths_helper(this->head->right, 0);
}

int ttree_delete(void *v_this, void *key, void *v_tdata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		return TTREE_ERR_NOTFOUND;
	}

	// set these as necessary
	ttree_node *greatest_lower_bound = NULL;
	ttree_node *least_upper_bound = NULL;
	bool deleted_node = false;

	int rv = TTREE_ERR_NOTFOUND;

	while (true) {
		// First Value
		int cmp = memcmp(key, node->values, tdata->key_sz);

		if (! cmp) {
			if (node->n_elements == 1 && ! node->left && ! node->right && node->parent) {
				node->n_elements--;

				if (node->parent->left == node) {
					ttree_node *prev = get_prev_node(node);

					if (prev) {
						prev->next = node->next;
					}

					node->parent->left = NULL;
				}
				else {
					node->parent->next = node->next;
					node->parent->right = NULL;
				}

				ttree_destroy_node(node);
				deleted_node = true;
			}
			else {
				greatest_lower_bound = get_greatest_lower_bound(node);

				if (! greatest_lower_bound) {
					memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * (node->n_elements - 1));

					least_upper_bound = get_least_upper_bound(node);

					if (! least_upper_bound) {
						node->n_elements--;
					}
				}
			}

			rv = TTREE_OK;

			break;
		}

		if (cmp < 0) {
			if (! node->left) {
				break;
			}

			node = node->left;
			continue;
		}

		if (node->n_elements == 1) {
			if (! node->right) {
				break;
			}

			node = node->right;
			continue;
		}

		// Last Value
		void *cmp_value = get_value_by_index(node, node->n_elements - 1, tdata->value_sz);
		cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			least_upper_bound = get_least_upper_bound(node);

			if (! least_upper_bound) {
				greatest_lower_bound = get_greatest_lower_bound(node);

				if (greatest_lower_bound) {
					memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (node->n_elements - 1));
					memcpy(node->values, get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements - 1, tdata->value_sz), tdata->value_sz);
					greatest_lower_bound->n_elements--;
				}
				else {
					node->n_elements--;
				}
			}

			rv = TTREE_OK;
			break;
		}

		if (cmp > 0) {
			if (! node->right) {
				break;
			}

			node = node->right;
			continue;
		}

		if (node->n_elements == 2) {
			break;
		}

		// Don't search the first and last elements.
		int16_t first = 0;
		int16_t last = node->n_elements - 3;

		while (first <= last) {
			uint8_t middle = (first + last) >> 1;
			cmp_value = get_value_by_index(node, middle + 1, tdata->value_sz);
			cmp = memcmp(key, cmp_value, tdata->key_sz);
			if (! cmp) {
				if (((middle + 1) << 1) < (node->n_elements + 1)) {
					greatest_lower_bound = get_greatest_lower_bound(node);

					if (greatest_lower_bound) {
						memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (middle + 1));
					}
					else {
						memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (node->n_elements - middle - 2));

						least_upper_bound = get_least_upper_bound(node);
						if (! least_upper_bound) {
							node->n_elements--;
						}
					}
				}
				else {
					least_upper_bound = get_least_upper_bound(node);

					if (least_upper_bound) {
						memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (node->n_elements - middle - 2));
					}
					else {
						greatest_lower_bound = get_greatest_lower_bound(node);

						if (greatest_lower_bound) {
							memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (middle + 1));
						}
						else {
							memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (node->n_elements - middle - 2));
							node->n_elements--;
						}
					}
				}

				rv = TTREE_OK;
				break;
			}
			else if (cmp < 0) {
				last = middle - 1;
			}
			else {
				first = middle + 1;
			}
		}

		break;
	}

	if (rv == TTREE_OK) {
		if (greatest_lower_bound) {
			// move last in greatest_lower_bound to head of node
			greatest_lower_bound->n_elements--;
			memcpy(node->values, get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), tdata->value_sz);

			if (! greatest_lower_bound->n_elements) {
				ttree_node *glb_of_glb = get_greatest_lower_bound(greatest_lower_bound);

				if (glb_of_glb) {
					glb_of_glb->next = node;

					if (node->left == greatest_lower_bound) {
						// greatest lower bound is node->left
						node->left->left->parent = node;
						node->left = node->left->left;
					}
					else {
						// greatest lower bound is some levels right of node->left
						greatest_lower_bound->left->parent = greatest_lower_bound->parent;
						greatest_lower_bound->parent->right = greatest_lower_bound->left;
					}
				}
				else if (greatest_lower_bound->parent->left == greatest_lower_bound) {
					// greatest lower bound is leaf node at node->left
					ttree_node *prev = get_prev_node(greatest_lower_bound);

					if (prev) {
						prev->next = greatest_lower_bound->next;
					}

					greatest_lower_bound->parent->left = NULL;
				}
				else {
					// greatest lower bound is some levels right of node->left
					greatest_lower_bound->parent->next = node;
					greatest_lower_bound->parent->right = NULL;
				}

				ttree_destroy_node(greatest_lower_bound);
				deleted_node = true;
			}
		}
		else if (least_upper_bound) {
			// move first in least_upper_bound to tail of node
			least_upper_bound->n_elements--;
			memcpy(get_value_by_index(node, node->n_elements - 1, tdata->value_sz), least_upper_bound->values, tdata->value_sz);
			memmove(least_upper_bound->values, get_value_idx_1(least_upper_bound, tdata->value_sz), tdata->value_sz * least_upper_bound->n_elements);

			if (! least_upper_bound->n_elements) {
				ttree_node *lub_of_lub = get_least_upper_bound(least_upper_bound);

				if (lub_of_lub) {
					// least upper bound has its own least upper bound
					node->next = lub_of_lub;

					if (node->right == least_upper_bound) {
						// least upper bound is node->right
						node->right->right->parent = node;
						node->right = node->right->right;
					}
					else {
						// least upper bound is some levels left of node->right
						least_upper_bound->right->parent = least_upper_bound->parent;
						least_upper_bound->parent->left = least_upper_bound->right;
					}
				}
				else if (least_upper_bound->parent->right == least_upper_bound) {
					// least upper bound is leaf node at node->right
					least_upper_bound->parent->next = least_upper_bound->next;
					least_upper_bound->parent->right = NULL;
				}
				else {
					// least upper bound is some levels left of node->right
					node->next = least_upper_bound->parent;
					least_upper_bound->parent->left = NULL;
				}

				ttree_destroy_node(least_upper_bound);
				deleted_node = true;
			}
		}
	}

	if (deleted_node) {
		update_metadata(this);

		while (this->left_depth > (this->right_depth + 1)) {
			rotate_right(this);
		}

		while (this->right_depth > (this->left_depth + 1)) {
			rotate_left(this);
		}
	}

	return rv;
}

int ttree_get_and_delete(void *v_this, void *key, void *value, void *v_tdata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		return TTREE_ERR_NOTFOUND;
	}

	// set these as necessary
	ttree_node *greatest_lower_bound = NULL;
	ttree_node *least_upper_bound = NULL;
	bool deleted_node = false;

	int rv = TTREE_ERR_NOTFOUND;

	while (true) {
		// First Value
		int cmp = memcmp(key, node->values, tdata->key_sz);

		if (! cmp) {
			memcpy(value, node->values, tdata->value_sz);

			if (node->n_elements == 1 && ! node->left && ! node->right && node->parent) {
				node->n_elements--;

				if (node->parent->left == node) {
					ttree_node *prev = get_prev_node(node);

					if (prev) {
						prev->next = node->next;
					}

					node->parent->left = NULL;
				}
				else {
					node->parent->next = node->next;
					node->parent->right = NULL;
				}

				ttree_destroy_node(node);
				deleted_node = true;
			}
			else {
				greatest_lower_bound = get_greatest_lower_bound(node);

				if (! greatest_lower_bound) {
					memmove(node->values, get_value_idx_1(node, tdata->value_sz), tdata->value_sz * (node->n_elements - 1));

					least_upper_bound = get_least_upper_bound(node);

					if (! least_upper_bound) {
						node->n_elements--;
					}
				}
			}

			rv = TTREE_OK;

			break;
		}

		if (cmp < 0) {
			if (! node->left) {
				break;
			}

			node = node->left;
			continue;
		}

		if (node->n_elements == 1) {
			if (! node->right) {
				break;
			}

			node = node->right;
			continue;
		}

		// Last Value
		void *cmp_value = get_value_by_index(node, node->n_elements - 1, tdata->value_sz);
		cmp = memcmp(key, cmp_value, tdata->key_sz);

		if (! cmp) {
			memcpy(value, get_value_by_index(node, node->n_elements - 1, tdata->value_sz), tdata->value_sz);

			least_upper_bound = get_least_upper_bound(node);

			if (! least_upper_bound) {
				greatest_lower_bound = get_greatest_lower_bound(node);

				if (greatest_lower_bound) {
					memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (node->n_elements - 1));
					memcpy(node->values, get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements - 1, tdata->value_sz), tdata->value_sz);
					greatest_lower_bound->n_elements--;
				}
				else {
					node->n_elements--;
				}
			}

			rv = TTREE_OK;
			break;
		}

		if (cmp > 0) {
			if (! node->right) {
				break;
			}

			node = node->right;
			continue;
		}

		if (node->n_elements == 2) {
			break;
		}

		// Don't search the first and last elements.
		int16_t first = 0;
		int16_t last = node->n_elements - 3;

		while (first <= last) {
			uint8_t middle = (first + last) >> 1;
			cmp_value = get_value_by_index(node, middle + 1, tdata->value_sz);
			cmp = memcmp(key, cmp_value, tdata->key_sz);
			if (! cmp) {
				memcpy(value, cmp_value, tdata->value_sz);

				if (((middle + 1) << 1) < (node->n_elements + 1)) {
					greatest_lower_bound = get_greatest_lower_bound(node);

					if (greatest_lower_bound) {
						memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (middle + 1));
					}
					else {
						memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (node->n_elements - middle - 2));

						least_upper_bound = get_least_upper_bound(node);
						if (! least_upper_bound) {
							node->n_elements--;
						}
					}
				}
				else {
					least_upper_bound = get_least_upper_bound(node);

					if (least_upper_bound) {
						memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (node->n_elements - middle - 2));
					}
					else {
						greatest_lower_bound = get_greatest_lower_bound(node);

						if (greatest_lower_bound) {
							memmove(get_value_idx_1(node, tdata->value_sz), node->values, tdata->value_sz * (middle + 1));
						}
						else {
							memmove(cmp_value, (byte *)cmp_value + tdata->value_sz, tdata->value_sz * (node->n_elements - middle - 2));
							node->n_elements--;
						}
					}
				}

				rv = TTREE_OK;
				break;
			}
			else if (cmp < 0) {
				last = middle - 1;
			}
			else {
				first = middle + 1;
			}
		}

		break;
	}

	if (rv == TTREE_OK) {
		if (greatest_lower_bound) {
			// move last in greatest_lower_bound to head of node
			greatest_lower_bound->n_elements--;
			memcpy(node->values, get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements, tdata->value_sz), tdata->value_sz);

			if (! greatest_lower_bound->n_elements) {
				ttree_node *glb_of_glb = get_greatest_lower_bound(greatest_lower_bound);

				if (glb_of_glb) {
					glb_of_glb->next = node;

					if (node->left == greatest_lower_bound) {
						// greatest lower bound is node->left
						node->left->left->parent = node;
						node->left = node->left->left;
					}
					else {
						// greatest lower bound is some levels right of node->left
						greatest_lower_bound->left->parent = greatest_lower_bound->parent;
						greatest_lower_bound->parent->right = greatest_lower_bound->left;
					}
				}
				else if (greatest_lower_bound->parent->left == greatest_lower_bound) {
					// greatest lower bound is leaf node at node->left
					ttree_node *prev = get_prev_node(greatest_lower_bound);

					if (prev) {
						prev->next = greatest_lower_bound->next;
					}

					greatest_lower_bound->parent->left = NULL;
				}
				else {
					// greatest lower bound is some levels right of node->left
					greatest_lower_bound->parent->next = node;
					greatest_lower_bound->parent->right = NULL;
				}

				ttree_destroy_node(greatest_lower_bound);
				deleted_node = true;
			}
		}
		else if (least_upper_bound) {
			// move first in least_upper_bound to tail of node
			least_upper_bound->n_elements--;
			memcpy(get_value_by_index(node, node->n_elements - 1, tdata->value_sz), least_upper_bound->values, tdata->value_sz);
			memmove(least_upper_bound->values, get_value_idx_1(least_upper_bound, tdata->value_sz), tdata->value_sz * least_upper_bound->n_elements);

			if (! least_upper_bound->n_elements) {
				ttree_node *lub_of_lub = get_least_upper_bound(least_upper_bound);

				if (lub_of_lub) {
					// least upper bound has its own least upper bound
					node->next = lub_of_lub;

					if (node->right == least_upper_bound) {
						// least upper bound is node->right
						node->right->right->parent = node;
						node->right = node->right->right;
					}
					else {
						// least upper bound is some levels left of node->right
						least_upper_bound->right->parent = least_upper_bound->parent;
						least_upper_bound->parent->left = least_upper_bound->right;
					}
				}
				else if (least_upper_bound->parent->right == least_upper_bound) {
					// least upper bound is leaf node at node->right
					least_upper_bound->parent->next = least_upper_bound->next;
					least_upper_bound->parent->right = NULL;
				}
				else {
					// least upper bound is some levels left of node->right
					node->next = least_upper_bound->parent;
					least_upper_bound->parent->left = NULL;
				}

				ttree_destroy_node(least_upper_bound);
				deleted_node = true;
			}
		}
	}

	if (deleted_node) {
		update_metadata(this);

		while (this->left_depth > (this->right_depth + 1)) {
			rotate_right(this);
		}

		while (this->right_depth > (this->left_depth + 1)) {
			rotate_left(this);
		}
	}

	return rv;
}

int ttree_reduce(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		return TTREE_OK;
	}

	while (node->left) {
		node = node->left;
	}

	do {
		void *loop_end = get_value_by_index(node, node->n_elements, tdata->value_sz);
		void *value = node->values;

		do {
			reduce_fn(value, value, udata);
			value = (byte *)value + tdata->value_sz;
		} while (value != loop_end);

		node = node->next;
	} while (node);

	return TTREE_OK;
}

int ttree_reduce_delete(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	ttree_tdata *tdata = (ttree_tdata *)v_tdata;

	if (! node->n_elements) {
		return TTREE_OK;
	}

	while (node->left) {
		node = node->left;
	}

	bool deleted_nodes = false;
	ttree_node *prev = NULL;

	do {
		void *loop_end = get_value_by_index(node, node->n_elements, tdata->value_sz);
		void *value = node->values;

		do {
			if (TTREE_REDUCE_DELETE == reduce_fn(value, value, udata)) {
				void *move_from = (byte *)value + tdata->value_sz;

				if (move_from != loop_end) {
					memmove(value, move_from, (byte *)loop_end - (byte *)value - tdata->value_sz);
				}

				node->n_elements--;

				loop_end = (byte *)loop_end - tdata->value_sz;
			}
			else {
				value = (byte *)value + tdata->value_sz;
			}
		} while (value != loop_end);

		// if nothing was deleted and node is full, continue
		if (node->n_elements == tdata->max_row_elements) {
			prev = node;
			node = node->next;
			continue;
		}

		// delete empty leaf node
		if (! node->n_elements && ! node->left && ! node->right && node->parent) {
			if (node->parent->left == node) {
				ttree_node *prev = get_prev_node(node);

				if (prev) {
					prev->next = node->next;
				}

				node->parent->left = NULL;
			}
			else {
				node->parent->next = node->next;
				node->parent->right = NULL;
			}

			ttree_node *next = node->next;

			ttree_destroy_node(node);
			deleted_nodes = true;

			node = next;

			continue;
		}

		ttree_node *greatest_lower_bound = get_greatest_lower_bound(node);

		while (greatest_lower_bound) {
			// move elements from greatest lower bound to node
			uint8_t space_left_in_node = tdata->max_row_elements - node->n_elements;
			uint8_t n_elements_to_move = space_left_in_node > greatest_lower_bound->n_elements ? greatest_lower_bound->n_elements : space_left_in_node;

			memmove(get_value_by_index(node, n_elements_to_move, tdata->value_sz), node->values, tdata->value_sz * node->n_elements);

			memcpy(node->values, get_value_by_index(greatest_lower_bound, greatest_lower_bound->n_elements - n_elements_to_move, tdata->value_sz), tdata->value_sz * n_elements_to_move);
			node->n_elements += n_elements_to_move;
			greatest_lower_bound->n_elements -= n_elements_to_move;

			if (greatest_lower_bound->n_elements) {
				break;
			}

			ttree_node *glb_of_glb = get_greatest_lower_bound(greatest_lower_bound);

			if (glb_of_glb) {
				glb_of_glb->next = node;

				if (node->left == greatest_lower_bound) {
					// greatest lower bound is node->left
					node->left->left->parent = node;
					node->left = node->left->left;
				}
				else {
					// greatest lower bound is some levels right of node->left
					greatest_lower_bound->left->parent = greatest_lower_bound->parent;
					greatest_lower_bound->parent->right = greatest_lower_bound->left;
				}
			}
			else if (greatest_lower_bound->parent->left == greatest_lower_bound) {
				// greatest lower bound is leaf node at node->left
				ttree_node *prev = get_prev_node(greatest_lower_bound);

				if (prev) {
					prev->next = greatest_lower_bound->next;
				}

				greatest_lower_bound->parent->left = NULL;
			}
			else {
				// greatest lower bound is some levels right of node->left
				greatest_lower_bound->parent->next = node;
				greatest_lower_bound->parent->right = NULL;
			}

			ttree_destroy_node(greatest_lower_bound);
			deleted_nodes = true;

			if (node->n_elements == tdata->max_row_elements) {
				break;
			}

			greatest_lower_bound = get_greatest_lower_bound(node);
		}

		if (node->n_elements == tdata->max_row_elements) {
			node = node->next;
			continue;
		}

		ttree_node *least_upper_bound = get_least_upper_bound(node);

		while (least_upper_bound) {

			// these elements have not yet been reduced, so let's reduce over as many as we need to before copying the leftovers
			uint8_t space_left_in_node = tdata->max_row_elements - node->n_elements;
			uint8_t n_elements_to_move = space_left_in_node > least_upper_bound->n_elements ? least_upper_bound->n_elements : space_left_in_node;
			uint8_t n_elements_not_deleted = 0;

			void *loop_end = get_value_by_index(least_upper_bound, least_upper_bound->n_elements, tdata->value_sz);
			void *value = least_upper_bound->values;

			do {
				if (TTREE_REDUCE_DELETE == reduce_fn(value, value, udata)) {
					void *move_from = (byte *)value + tdata->value_sz;

					if (move_from != loop_end) {
						memmove(value, move_from, (byte *)loop_end - (byte *)value - tdata->value_sz);
					}

					least_upper_bound->n_elements--;

					loop_end = (byte *)loop_end - tdata->value_sz;
				}
				else {
					value = (byte *)value + tdata->value_sz;
					n_elements_not_deleted++;
				}
			} while (value != loop_end && n_elements_not_deleted != n_elements_to_move);

			if (n_elements_to_move > n_elements_not_deleted) {
				n_elements_to_move = n_elements_not_deleted;
			}

			// move elements from greatest lower bound to node
			memcpy(get_value_by_index(node, node->n_elements, tdata->value_sz), least_upper_bound->values, tdata->value_sz * n_elements_to_move);
			memmove(least_upper_bound->values, get_value_by_index(least_upper_bound, n_elements_to_move, tdata->value_sz), tdata->value_sz * (least_upper_bound->n_elements - n_elements_to_move));
			node->n_elements += n_elements_to_move;
			least_upper_bound->n_elements -= n_elements_to_move;

			if (least_upper_bound->n_elements) {
				break;
			}

			ttree_node *lub_of_lub = get_least_upper_bound(least_upper_bound);

			if (lub_of_lub) {
				node->next = lub_of_lub;

				if (node->right == least_upper_bound) {
					// least upper bound is node->right
					node->right->right->parent = node;
					node->right = node->right->right;
				}
				else {
					// least upper bound is some levels left of node->left
					least_upper_bound->right->parent = least_upper_bound->parent;
					least_upper_bound->parent->left = least_upper_bound->right;
				}
			}
			else if (least_upper_bound->parent->right == least_upper_bound) {
				// least upper bound is leaf node at node->right

				node->next = least_upper_bound->next;
				least_upper_bound->parent->right = NULL;
			}
			else {
				// least upper bound is some levels left of node->right
				node->next = least_upper_bound->next;
				least_upper_bound->parent->left = NULL;
			}

			ttree_destroy_node(least_upper_bound);
			deleted_nodes = true;

			if (node->n_elements == tdata->max_row_elements) {
				break;
			}

			least_upper_bound = get_least_upper_bound(node);
		}

		prev = node;
		node = node->next;
	} while (node);

	if (deleted_nodes) {
		update_metadata(this);

		while (this->left_depth > (this->right_depth + 1)) {
			rotate_right(this);
		}

		while (this->right_depth > (this->left_depth + 1)) {
			rotate_left(this);
		}
	}

	return TTREE_OK;
}

uint32_t ttree_get_size(void *v_this) {
	cf_index_ttree *this = (cf_index_ttree *)v_this;
	ttree_node *node = this->head;

	if (! node->n_elements) {
		return 0;
	}

	while (node->left) {
		node = node->left;
	}

	uint32_t size = 0;

	do {
		size += node->n_elements;
		node = node->next;
	} while (node);

	return size;
}


int ttree_dump(void *key, void *value, void *udata) {
	ttree_dump_s *ttd = (ttree_dump_s *)udata;

	cf_str_itoa(*(int *)value, ttd->pos, 10);
	ttd->pos += strlen(ttd->pos);
	memset(ttd->pos, ',', 1);
	ttd->pos += 1;

	memset(ttd->pos, 0, 1);

	ttd->count++;

	if (! (ttd->count % 100)) {
		cf_info(AS_DRV_SSD, "%s", ttd->head);
		ttd->pos = ttd->head;
		ttd->count = 0;
	}

	return 0;
}

#if 0
static int delete_odds(void *key, void *value, void *udata) {
	int v = *(int *)value;
	if (v % 2) {
		return TTREE_REDUCE_DELETE;
	}

	return TTREE_OK;
}

static int delete_evens(void *key, void *value, void *udata) {
	if (*(int *)value % 2) {
		return TTREE_OK;
	}

	return TTREE_REDUCE_DELETE;
}


void ttree_test() {
	ttree_tdata tdata;
	tdata.key_sz = sizeof(int);
	tdata.value_sz = sizeof(int);
	tdata.max_row_elements = 32;

	cf_index_ttree this;
	ttree_create(&this, &tdata);

	ttree_dump_s ttd;
	ttd.head = cf_malloc(1024 * 1024);

	int put_count = 0;
	int odds = 0;
	int evens = 0;

	srand(22);
	for (int i = 0; i < 10000; i++) {
		int r = rand() % 100;

		if (rand() % 2) {
			ttree_get(&this, &r, &r, &tdata);
		}
		else {
			ttree_put(&this, &r, &r, &tdata);
			put_count++;

			if (r % 2) {
				odds++;
			}
			else {
				evens++;
			}
		}
	}

	cf_info(AS_AS, "size = %d | put_count %d | odds %d | evens %d", ttree_get_size(&this), put_count, odds, evens);

	/*
	srand(22);
	for (int i = 0; i < 10000; i++) {
		int r = rand() % 100;

		if (rand() % 2) {
			continue;
		}
		else {
			ttree_delete(&this, &r, &tdata);
		}
	}
	*/

	//memset(ttd.head, 0, 1024*1024);
	//ttd.pos = ttd.head;
	//ttd.count = 0;
	//ttree_reduce(&this, ttree_dump, &tdata, &ttd);
	//cf_info(AS_DRV_SSD, "%s", ttd.head);

	ttree_reduce_delete(&this, delete_odds, &tdata, NULL);

	cf_info(AS_AS, "size = %d | put_count %d | odds %d | evens %d", ttree_get_size(&this), put_count, odds, evens);

	ttree_reduce_delete(&this, delete_evens, &tdata, NULL);

	//memset(ttd.head, 0, 1024*1024);
	//ttd.pos = ttd.head;
	//ttd.count = 0;
	//ttree_reduce(&this, ttree_dump, &tdata, &ttd);
	//cf_info(AS_DRV_SSD, "%s", ttd.head);

	cf_info(AS_AS, "size = %d | put_count %d | odds %d | evens %d", ttree_get_size(&this), put_count, odds, evens);

	ttree_destroy(&this);

	cf_info(AS_AS, "success!");

	_exit(0);
}
#endif
