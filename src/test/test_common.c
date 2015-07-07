#include "test_common.h"

/******************************************************************************
 * atf_x_equals
 *****************************************************************************/

bool atf_val_equals(atf_test_result * __result__, as_val * actual, as_val * expected)
{
	if ( actual == expected ) {
		return true;
	}

	bassert_int_eq(as_val_type(actual), as_val_type(expected));
	switch(as_val_type(actual)) {
		case AS_INTEGER:
			bassert(atf_integer_equals(__result__, as_integer_fromval(actual), as_integer_fromval(expected)));
			break;
		case AS_DOUBLE:
			bassert(atf_double_equals(__result__, as_double_fromval(actual), as_double_fromval(expected)));
			break;
		case AS_STRING:
			bassert(atf_string_equals(__result__, as_string_fromval(actual), as_string_fromval(expected)));
			break;
		case AS_LIST:
			bassert(atf_list_equals(__result__, as_list_fromval(actual), as_list_fromval(expected)));
			break;
		case AS_MAP:
			bassert(atf_map_equals(__result__, as_map_fromval(actual), as_map_fromval(expected)));
			break;
		default:
			error("Unhandled val type");
	}
	return true;
}

bool atf_integer_equals(atf_test_result * __result__, as_integer * actual, as_integer * expected)
{
	bassert_int_eq( as_integer_get(actual), as_integer_get(expected) );
	return true;
}

bool atf_double_equals(atf_test_result * __result__, as_double * actual, as_double * expected)
{
	bassert_double_eq( as_double_get(actual), as_double_get(expected) );
	return true;
}

bool atf_string_equals(atf_test_result * __result__, as_string * actual, as_string * expected)
{
	bassert_string_eq( as_string_get(actual), as_string_get(expected) );
	return true;
}

bool atf_list_equals(atf_test_result * __result__, as_list * actual, as_list * expected)
{
	bassert_int_eq( as_list_size(actual), as_list_size(expected) );
	uint32_t n = as_list_size(actual);
	for ( int i = 0; i < n; i++ ) {
		as_val * v1 = as_list_get(actual, i);
		as_val * v2 = as_list_get(expected, i);
		bassert(atf_val_equals(__result__, v1, v2));
	}
	return true;
}

typedef struct {
	as_map * actual;
	atf_test_result * result;
} atf_map_equals_foreach_data;

static bool atf_map_equals_foreach(const as_val * key, const as_val * expected, void * udata)
{
	atf_map_equals_foreach_data * data = (atf_map_equals_foreach_data *) udata;
	as_val * actual = as_map_get(data->actual, (as_val *) key);
	return atf_val_equals(data->result, (as_val *) actual, (as_val *) expected);
}

bool atf_map_equals(atf_test_result * __result__, as_map * actual, as_map * expected)
{
	atf_map_equals_foreach_data data = {
		.actual = actual,
		.result = __result__
	};
	bassert_int_eq( as_map_size(actual), as_map_size(expected) );
	bassert(as_map_foreach(expected, atf_map_equals_foreach, &data));
	return true;
}
