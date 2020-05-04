#include <citrusleaf/cf_clock.h>
#include "test.h"

static bool
before(atf_plan* plan)
{
	return cf_clock_init();
}

PLAN(common) {

	plan_before(before);

    plan_add(types_boolean);
    plan_add(types_integer);
    plan_add(types_double);
    plan_add(types_string);
    plan_add(types_bytes);
    plan_add(types_arraylist);
    plan_add(types_hashmap);
    plan_add(types_nil);
    plan_add(types_queue);
	plan_add(types_queue_mt);

    plan_add(password);
    plan_add(string_builder);
	plan_add(random_numbers);
	
	plan_add(msgpack_roundtrip);
	plan_add(msgpack_direct);
}
