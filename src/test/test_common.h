#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <aerospike/as_double.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>
#include <aerospike/as_string.h>
#include <aerospike/as_val.h>

#include "test.h"

/******************************************************************************
 * atf_x_equals
 *****************************************************************************/

bool atf_val_equals(atf_test_result * test_result, as_val * actual, as_val * expected);
bool atf_integer_equals(atf_test_result * test_result, as_integer * actual, as_integer * expected);
bool atf_double_equals(atf_test_result * test_result, as_double * actual, as_double * expected);
bool atf_string_equals(atf_test_result * test_result, as_string * actual, as_string * expected);
bool atf_list_equals(atf_test_result * test_result, as_list * actual, as_list * expected);
bool atf_map_equals(atf_test_result * test_result, as_map * actual, as_map * expected);

#define assert_val_eq(ACTUAL, EXPECTED) \
	if ( atf_val_equals(__result__, (as_val *) ACTUAL, (as_val *) EXPECTED) == false ) {\
		atf_assert(__result__, #ACTUAL" == "#EXPECTED, __FILE__, __LINE__);\
	}
