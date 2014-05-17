#include "test.h"

PLAN( common ) {

    /**
     * types - tests types
     */
    plan_add( types_boolean );
    plan_add( types_integer );
    plan_add( types_string );
    plan_add( types_bytes );
    plan_add( types_arraylist );
    plan_add( types_hashmap );
    plan_add( types_nil );
    plan_add( types_vector );

    /**
     * msgpack - tests msgpack
     */
	plan_add( msgpack_roundtrip );
}
