#include "../test.h"
#include "../test_common.h"

#include <stdlib.h>
#include <string.h>

#include <aerospike/as_arraylist.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_val.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

// The library's bound, which is private to it. Divergence shows up as the
// boundary test below failing on the accepted side.
#define BOUND_DEPTH 64

typedef struct {
	const char *name;
	uint32_t sz;
	uint8_t buf[16];
} bounds_case;

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

// Copies to an exact-sized allocation so that a read past the declared length
// lands outside the block, where valgrind will see it. The unpacker copies
// what it keeps, so the value outlives the buffer.
static int
unpack_exact(const uint8_t *buf, uint32_t sz, as_val **val)
{
	uint8_t *copy = malloc(sz == 0 ? 1 : sz);

	memcpy(copy, buf, sz);

	as_unpacker pk = {
			.buffer = copy,
			.offset = 0,
			.length = sz,
	};

	as_val *v = NULL;
	int ret = as_unpack_val(&pk, &v);

	free(copy);

	if (val != NULL) {
		*val = v;
	}
	else {
		as_val_destroy(v);
	}

	return ret;
}

static as_val_t
peek_exact(const uint8_t *buf, uint32_t sz)
{
	uint8_t *copy = malloc(sz == 0 ? 1 : sz);

	memcpy(copy, buf, sz);

	as_val_t type = as_unpack_buf_peek_type(copy, sz);

	free(copy);

	return type;
}

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

// Peeking classifies without validating the whole element, so what each
// form has to have is only as far as the byte the classification reads -
// need, below, not the encoding's full length.
typedef struct {
	const char *name;
	uint32_t need;
	uint32_t sz;
	uint8_t buf[8];
} peek_case;

static const peek_case peek_cases[] = {
	{ "str8",     3, 4, { 0xd9, 0x02, AS_BYTES_STRING, 'A' } },
	{ "bin8",     3, 4, { 0xc4, 0x02, AS_BYTES_BLOB, 0xff } },
	{ "str16",    4, 5, { 0xda, 0x00, 0x02, AS_BYTES_STRING, 'A' } },
	{ "bin16",    4, 5, { 0xc5, 0x00, 0x02, AS_BYTES_BLOB, 0xff } },
	{ "str32",    6, 7, { 0xdb, 0, 0, 0, 0x02, AS_BYTES_STRING, 'A' } },
	{ "bin32",    6, 7, { 0xc6, 0, 0, 0, 0x02, AS_BYTES_BLOB, 0xff } },
	{ "fixraw",   2, 3, { 0xa2, AS_BYTES_STRING, 'A' } },
	// The comparison ext types read one byte further than a plain ext.
	{ "wildcard", 3, 3, { 0xd4, 0xff, 0x00 } },
	{ "inf",      3, 3, { 0xd4, 0xff, 0x01 } },
	{ "ext",      2, 3, { 0xd4, 0x01, 0x00 } },
};

// One complete encoding per header width. Every proper prefix of each must be
// refused without reading beyond what was handed over.
static const bounds_case complete[] = {
	{ "nil",      1,  { 0xc0 } },
	{ "true",     1,  { 0xc3 } },
	{ "false",    1,  { 0xc2 } },
	{ "fixint",   1,  { 0x7f } },
	{ "negfixint",1,  { 0xe1 } },
	{ "uint8",    2,  { 0xcc, 0x7f } },
	{ "int8",     2,  { 0xd0, 0xff } },
	{ "uint16",   3,  { 0xcd, 0x12, 0x34 } },
	{ "int16",    3,  { 0xd1, 0xff, 0xff } },
	{ "uint32",   5,  { 0xce, 0x01, 0x02, 0x03, 0x04 } },
	{ "int32",    5,  { 0xd2, 0xff, 0xff, 0xff, 0xff } },
	{ "uint64",   9,  { 0xcf, 1, 2, 3, 4, 5, 6, 7, 8 } },
	{ "int64",    9,  { 0xd3, 1, 2, 3, 4, 5, 6, 7, 8 } },
	{ "float",    5,  { 0xca, 0x3f, 0x80, 0x00, 0x00 } },
	{ "double",   9,  { 0xcb, 0x3f, 0xf0, 0, 0, 0, 0, 0, 0 } },
	{ "fixstr",   3,  { 0xa2, AS_BYTES_STRING, 'A' } },
	{ "str8",     4,  { 0xd9, 0x02, AS_BYTES_STRING, 'A' } },
	{ "str16",    5,  { 0xda, 0x00, 0x02, AS_BYTES_STRING, 'A' } },
	{ "str32",    7,  { 0xdb, 0, 0, 0, 0x02, AS_BYTES_STRING, 'A' } },
	{ "bin8",     4,  { 0xc4, 0x02, AS_BYTES_BLOB, 0xff } },
	{ "bin16",    5,  { 0xc5, 0x00, 0x02, AS_BYTES_BLOB, 0xff } },
	{ "bin32",    7,  { 0xc6, 0, 0, 0, 0x02, AS_BYTES_BLOB, 0xff } },
	{ "fixarray", 3,  { 0x92, 0x01, 0x02 } },
	{ "array16",  5,  { 0xdc, 0x00, 0x02, 0x01, 0x02 } },
	{ "array32",  7,  { 0xdd, 0, 0, 0, 0x02, 0x01, 0x02 } },
	{ "fixmap",   3,  { 0x81, 0x01, 0x02 } },
	{ "map16",    5,  { 0xde, 0x00, 0x01, 0x01, 0x02 } },
	{ "map32",    7,  { 0xdf, 0, 0, 0, 0x01, 0x01, 0x02 } },
	{ "fixext1",  3,  { 0xd4, 0xff, 0x00 } },
	{ "nested",   4,  { 0x91, 0x91, 0x91, 0xc0 } },
};

TEST( msgpack_bounds_truncated, "every truncation of a complete encoding is refused" )
{
	for (size_t i = 0; i < sizeof(complete) / sizeof(complete[0]); i++) {
		const bounds_case *c = &complete[i];

		for (uint32_t sz = 0; sz < c->sz; sz++) {
			int ret = unpack_exact(c->buf, sz, NULL);

			if (ret == 0) {
				info("%s truncated to %u bytes parsed", c->name, sz);
			}

			assert_int_ne(ret, 0);
		}

		int ret = unpack_exact(c->buf, c->sz, NULL);

		if (ret != 0) {
			info("%s failed at its full %u bytes", c->name, c->sz);
		}

		assert_int_eq(ret, 0);
	}
}

TEST( msgpack_bounds_overdeclared, "headers claiming more than is present are refused" )
{
	// A blob and a string declaring 200 bytes with a handful present.
	const uint8_t bin32[] = { 0xc6, 0x00, 0x00, 0x00, 0xc8, AS_BYTES_BLOB };
	const uint8_t str32[] = { 0xdb, 0x00, 0x00, 0x00, 0xc8, AS_BYTES_STRING };

	assert_int_ne(unpack_exact(bin32, sizeof(bin32), NULL), 0);
	assert_int_ne(unpack_exact(str32, sizeof(str32), NULL), 0);

	// Element counts no run of bytes this short could ever back.
	const uint8_t list32[] = { 0xdd, 0xff, 0xff, 0xff, 0xff };
	const uint8_t map32[] = { 0xdf, 0xff, 0xff, 0xff, 0xff };
	const uint8_t list16[] = { 0xdc, 0xff, 0xff };
	const uint8_t map16[] = { 0xde, 0xff, 0xff };

	assert_int_ne(unpack_exact(list32, sizeof(list32), NULL), 0);
	assert_int_ne(unpack_exact(map32, sizeof(map32), NULL), 0);
	assert_int_ne(unpack_exact(list16, sizeof(list16), NULL), 0);
	assert_int_ne(unpack_exact(map16, sizeof(map16), NULL), 0);

	// An ext size of UINT32_MAX, where a gap check of 1 + size would wrap.
	const uint8_t ext32[] = { 0xc9, 0xff, 0xff, 0xff, 0xff, 0x01, 0x02 };

	assert_int_ne(unpack_exact(ext32, sizeof(ext32), NULL), 0);

	as_unpacker pk = {
			.buffer = ext32,
			.offset = 0,
			.length = sizeof(ext32),
	};
	as_msgpack_ext ext;

	assert_int_ne(as_unpack_ext(&pk, &ext), 0);
}

static uint32_t
fill_nested_lists(uint8_t *buf, uint32_t n)
{
	memset(buf, 0x91, n);
	buf[n] = 0xc0;

	return n + 1;
}

TEST( msgpack_bounds_depth, "nesting is admitted to the bound and no further" )
{
	// The bound counts containers, so it is the innermost list that decides,
	// not the scalar it holds.
	uint8_t buf[BOUND_DEPTH + 2];
	as_val *val = NULL;

	assert_int_eq(unpack_exact(buf, fill_nested_lists(buf, BOUND_DEPTH),
			&val), 0);
	assert_not_null(val);
	assert_int_eq(as_val_type(val), AS_LIST);
	as_val_destroy(val);

	assert_int_ne(unpack_exact(buf, fill_nested_lists(buf,
			BOUND_DEPTH + 1), NULL), 0);
}

TEST( msgpack_bounds_depth_size_cmp, "size and compare share the bound" )
{
	uint8_t buf[BOUND_DEPTH + 2];
	uint32_t ok_sz = fill_nested_lists(buf, BOUND_DEPTH);

	as_unpacker pk = { .buffer = buf, .offset = 0, .length = ok_sz };

	assert_int_eq(as_unpack_size(&pk), (int64_t)ok_sz);

	uint8_t deep[BOUND_DEPTH + 3];
	uint32_t bad_sz = fill_nested_lists(deep, BOUND_DEPTH + 1);

	pk = (as_unpacker){ .buffer = deep, .offset = 0, .length = bad_sz };
	assert_true(as_unpack_size(&pk) < 0);

	as_unpacker ok1 = { .buffer = buf, .offset = 0, .length = ok_sz };
	as_unpacker ok2 = { .buffer = buf, .offset = 0, .length = ok_sz };

	assert_int_eq(as_unpack_compare(&ok1, &ok2), MSGPACK_COMPARE_EQUAL);

	as_unpacker pk1 = { .buffer = deep, .offset = 0, .length = bad_sz };
	as_unpacker pk2 = { .buffer = deep, .offset = 0, .length = bad_sz };

	assert_int_eq(as_unpack_compare(&pk1, &pk2), MSGPACK_COMPARE_ERROR);
}

TEST( msgpack_bounds_partial, "a container that runs out frees what it built" )
{
	// A list promised two elements and given one.
	const uint8_t short_list[] = { 0x92, 0x01 };

	assert_int_ne(unpack_exact(short_list, sizeof(short_list), NULL), 0);

	// A map whose key parses and whose value is a container that does not,
	// so the key is still held when the failure arrives.
	const uint8_t short_value[] = { 0x81, 0x01, 0x92 };

	assert_int_ne(unpack_exact(short_value, sizeof(short_value), NULL), 0);

	// The same, nested, so several levels are each holding a key.
	const uint8_t nested[] = { 0x81, 0x01, 0x81, 0x02, 0x81, 0x03, 0x92 };

	assert_int_ne(unpack_exact(nested, sizeof(nested), NULL), 0);

	const uint32_t depth = 60;
	uint8_t *buf = malloc(depth);

	memset(buf, 0x91, depth);

	assert_int_ne(unpack_exact(buf, depth, NULL), 0);

	free(buf);
}

TEST( msgpack_bounds_map_flags, "map flags still select the container built" )
{
	// A map carries its flags as an ext element paired with a nil, counted
	// among its elements - so two elements here, one of them real.
	const uint8_t preserve_order[] = {
			0x82, 0xc7, 0x00, AS_PACKED_MAP_FLAG_PRESERVE_ORDER, 0xc0,
			0x01, 0x02
	};
	const uint8_t k_ordered[] = {
			0x82, 0xc7, 0x00, AS_PACKED_MAP_FLAG_K_ORDERED, 0xc0,
			0x01, 0x02
	};

	as_val *val = NULL;

	// Preserving order gives back the pairs flattened into a list.
	assert_int_eq(unpack_exact(preserve_order, sizeof(preserve_order), &val), 0);
	assert_not_null(val);
	assert_int_eq(as_val_type(val), AS_LIST);
	assert_int_eq(as_list_size(as_list_fromval(val)), 2);
	assert_int_eq(as_val_type(as_list_get(as_list_fromval(val), 0)), AS_INTEGER);
	as_val_destroy(val);

	val = NULL;
	assert_int_eq(unpack_exact(k_ordered, sizeof(k_ordered), &val), 0);
	assert_not_null(val);
	assert_int_eq(as_val_type(val), AS_MAP);
	assert_int_eq(as_map_size(as_map_fromval(val)), 1);
	as_val_destroy(val);
}

TEST( msgpack_bounds_deep_mixed, "alternating list and map nesting tears down" )
{
	// Each level is a one-pair map whose value is a one-element list, so the
	// teardown walk has to cross container types the whole way down.
	const uint32_t levels = 30;
	uint8_t *buf = malloc(levels * 3 + 1);
	uint32_t sz = 0;

	for (uint32_t i = 0; i < levels; i++) {
		buf[sz++] = 0x81; // fixmap, 1 pair
		buf[sz++] = 0x01; // key
		buf[sz++] = 0x91; // fixarray, 1 element - the value
	}

	buf[sz++] = 0xc0;

	as_val *val = NULL;

	assert_int_eq(unpack_exact(buf, sz, &val), 0);
	assert_not_null(val);
	assert_int_eq(as_val_type(val), AS_MAP);

	as_val_destroy(val);
	free(buf);
}

TEST( msgpack_bounds_wide_and_deep, "a deep spine with siblings tears down" )
{
	// Siblings at every level push the teardown's worklist past whatever it
	// holds on the stack.
	const uint32_t levels = 60;
	uint8_t *buf = malloc(levels * 3 + 1);
	uint32_t sz = 0;

	for (uint32_t i = 0; i < levels; i++) {
		buf[sz++] = 0x93; // fixarray, 3 elements
		buf[sz++] = 0x01;
		buf[sz++] = 0x02; // two leaves, then the spine continues
	}

	buf[sz++] = 0xc0;

	as_val *val = NULL;

	assert_int_eq(unpack_exact(buf, sz, &val), 0);
	assert_not_null(val);
	assert_int_eq(as_val_type(val), AS_LIST);
	assert_int_eq(as_list_size(as_list_fromval(val)), 3);

	as_val_destroy(val);
	free(buf);
}

TEST( msgpack_bounds_shared_subtree, "a held child outlives its parent" )
{
	const uint8_t nested[] = { 0x91, 0x92, 0x01, 0x02 }; // [[1, 2]]
	as_val *val = NULL;

	assert_int_eq(unpack_exact(nested, sizeof(nested), &val), 0);
	assert_not_null(val);

	as_val *child = as_list_get(as_list_fromval(val), 0);

	assert_not_null(child);
	assert_int_eq(as_val_type(child), AS_LIST);

	as_val_reserve(child);
	as_val_destroy(val);

	// The parent is gone; the extra reference must have kept the child whole.
	assert_int_eq(as_val_type(child), AS_LIST);
	assert_int_eq(as_list_size(as_list_fromval(child)), 2);
	assert_int_eq(as_val_type(as_list_get(as_list_fromval(child), 0)),
			AS_INTEGER);

	as_val_destroy(child);
}

TEST( msgpack_bounds_wide_shallow, "breadth is not what the bound limits" )
{
	// One parent over many empty lists - depth 2, however wide the level gets.
	const uint32_t n = 200000;
	uint8_t *buf = malloc(5 + n);
	uint32_t sz = 0;

	buf[sz++] = 0xdd;
	buf[sz++] = (uint8_t)(n >> 24);
	buf[sz++] = (uint8_t)(n >> 16);
	buf[sz++] = (uint8_t)(n >> 8);
	buf[sz++] = (uint8_t)n;

	memset(buf + sz, 0x90, n);
	sz += n;

	as_val *val = NULL;

	assert_int_eq(unpack_exact(buf, sz, &val), 0);
	assert_not_null(val);
	assert_int_eq(as_val_type(val), AS_LIST);
	assert_int_eq(as_list_size(as_list_fromval(val)), n);

	as_val_destroy(val);
	free(buf);
}

TEST( msgpack_bounds_peek_truncated, "peeking a truncated header is refused" )
{
	for (size_t i = 0; i < sizeof(peek_cases) / sizeof(peek_cases[0]); i++) {
		const peek_case *c = &peek_cases[i];

		for (uint32_t sz = 0; sz < c->need; sz++) {
			as_val_t type = peek_exact(c->buf, sz);

			if (type != AS_UNDEF) {
				info("%s truncated to %u peeked as %d", c->name, sz, type);
			}

			assert_int_eq(type, AS_UNDEF);
		}

		// Classifying still works from need onward, so a peek that refused
		// everything would not pass this.
		for (uint32_t sz = c->need; sz <= c->sz; sz++) {
			as_val_t type = peek_exact(c->buf, sz);

			if (type == AS_UNDEF) {
				info("%s at %u peeked as undefined", c->name, sz);
			}

			assert_int_ne(type, AS_UNDEF);
		}
	}
}

// A map's leading ext pair carries its order flags, and the pair's value is
// skipped rather than built. The skip walks the same budget as the map, so a
// container parked there counts toward the bound like anything else.
static uint32_t
fill_meta_map(uint8_t *buf, uint32_t nest)
{
	uint32_t sz = 0;

	buf[sz++] = 0x81; // fixmap, one pair - the metadata
	buf[sz++] = 0xd4; // fixext1 key
	buf[sz++] = 0x00;
	buf[sz++] = 0x00;

	sz += fill_nested_lists(buf + sz, nest); // the pair's value

	return sz;
}

TEST( msgpack_bounds_meta_depth, "a map's metadata value shares the bound" )
{
	uint8_t buf[BOUND_DEPTH + 8];

	// The map is one level, so the value may nest to one short of the bound.
	assert_int_eq(unpack_exact(buf, fill_meta_map(buf, BOUND_DEPTH - 1), NULL),
			0);
	assert_int_ne(unpack_exact(buf, fill_meta_map(buf, BOUND_DEPTH), NULL), 0);
}

TEST( msgpack_bounds_skip_depth, "a skipped element shares the bound" )
{
	// Comparing lists of unequal length skips the excess, and that skip is
	// walked at the depth it sits in rather than from zero.
	uint8_t shallow[BOUND_DEPTH + 8];
	uint8_t deep[BOUND_DEPTH + 8];
	uint32_t shallow_sz = 0;
	uint32_t deep_sz = 0;

	shallow[shallow_sz++] = 0x91; // one element
	deep[deep_sz++] = 0x92;       // two, so the second is skipped
	shallow[shallow_sz++] = 0x01;
	deep[deep_sz++] = 0x01;

	uint32_t base_sz = deep_sz;

	// The list holding the skipped element is itself a level, so one short of
	// the bound still skips and the shorter list compares less.
	deep_sz = base_sz + fill_nested_lists(deep + base_sz, BOUND_DEPTH - 1);

	as_unpacker ok1 = { .buffer = shallow, .offset = 0, .length = shallow_sz };
	as_unpacker ok2 = { .buffer = deep, .offset = 0, .length = deep_sz };

	assert_int_eq(as_unpack_compare(&ok1, &ok2), MSGPACK_COMPARE_LESS);

	deep_sz = base_sz + fill_nested_lists(deep + base_sz, BOUND_DEPTH);

	as_unpacker pk1 = { .buffer = shallow, .offset = 0, .length = shallow_sz };
	as_unpacker pk2 = { .buffer = deep, .offset = 0, .length = deep_sz };

	assert_int_eq(as_unpack_compare(&pk1, &pk2), MSGPACK_COMPARE_ERROR);
}

TEST( msgpack_bounds_mismatch_skip_depth, "a mismatched skip shares the bound" )
{
	// Unequal types end the comparison, and both elements are then skipped
	// from where they sit rather than from zero.
	uint8_t deep[BOUND_DEPTH + 8];
	uint8_t scalar[] = { 0x91, 0x01 };

	for (uint32_t nest = BOUND_DEPTH - 1; nest <= BOUND_DEPTH; nest++) {
		uint32_t sz = 0;

		deep[sz++] = 0x91;
		sz += fill_nested_lists(deep + sz, nest);

		as_unpacker pk1 = { .buffer = deep, .offset = 0, .length = sz };
		as_unpacker pk2 = {
				.buffer = scalar, .offset = 0, .length = sizeof(scalar)
		};

		msgpack_compare_t expect = nest == BOUND_DEPTH ?
				MSGPACK_COMPARE_ERROR : MSGPACK_COMPARE_GREATER;

		assert_int_eq(as_unpack_compare(&pk1, &pk2), expect);
	}
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( msgpack_bounds, "as_msgpack unpack bounds" )
{
	suite_add( msgpack_bounds_truncated );
	suite_add( msgpack_bounds_overdeclared );
	suite_add( msgpack_bounds_depth );
	suite_add( msgpack_bounds_depth_size_cmp );
	suite_add( msgpack_bounds_peek_truncated );
	suite_add( msgpack_bounds_meta_depth );
	suite_add( msgpack_bounds_skip_depth );
	suite_add( msgpack_bounds_mismatch_skip_depth );
	suite_add( msgpack_bounds_partial );
	suite_add( msgpack_bounds_map_flags );
	suite_add( msgpack_bounds_deep_mixed );
	suite_add( msgpack_bounds_wide_and_deep );
	suite_add( msgpack_bounds_shared_subtree );
	suite_add( msgpack_bounds_wide_shallow );
}
