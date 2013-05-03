###############################################################################
##  TEST FLAGS                                                       		 ##
###############################################################################

TEST_VALGRIND = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes -v

TEST_CFLAGS =  -DMEM_COUNT=1
TEST_CFLAGS += -I$(TARGET_INCL)
TEST_CFLAGS += -Imodules/common/$(TARGET_INCL)

TEST_LDFLAGS = -lssl -lcrypto -llua -lpthread -lm -lrt 

TEST_DEPS =
TEST_DEPS += modules/common/$(TARGET_OBJ)/client/*.o 
TEST_DEPS += modules/common/$(TARGET_OBJ)/shared/*.o 
TEST_DEPS += $(MSGPACK_PATH)/src/.libs/libmsgpackc.a

###############################################################################
##  TEST OBJECTS                                                       		 ##
###############################################################################

TEST_TYPES = 
TEST_TYPES += types/types_integer
TEST_TYPES += types/types_string
TEST_TYPES += types/types_bytes
TEST_TYPES += types/types_arraylist
TEST_TYPES += types/types_linkedlist
TEST_TYPES += types/types_hashmap

TEST_STREAM = 
TEST_STREAM += stream/stream_basics
TEST_STREAM += stream/stream_udf

TEST_RECORD = 
TEST_RECORD += record/record_basics
TEST_RECORD += record/record_udf
TEST_RECORD += record/bytes_udf

TEST_UTIL = 
TEST_UTIL += util/consumer_stream
TEST_UTIL += util/producer_stream
TEST_UTIL += util/map_rec
TEST_UTIL += util/test_aerospike
TEST_UTIL += util/test_logger
#TEST_UTIL += util/test_memtracker

TEST_MOD_LUA = mod_lua_test
TEST_MOD_LUA += $(TEST_UTIL) 
TEST_MOD_LUA += $(TEST_TYPES) 
TEST_MOD_LUA += $(TEST_STREAM)
TEST_MOD_LUA += $(TEST_RECORD) 

###############################################################################
##  TEST TARGETS                                                      		 ##
###############################################################################

.PHONY: test
test: test-build
	@$(TARGET_BIN)/test/mod_lua_test

.PHONY: test-valgrind
test-valgrind: test-build
	valgrind $(TEST_VALGRIND) $(TARGET_BIN)/test/mod_lua_test 1>&2 2>mod_lua_test-valgrind

.PHONY: test-build
test-build: test/mod_lua_test

.PHONY: test-clean
test-clean: 
	@rm -rf $(TARGET_BIN)/test
	@rm -rf $(TARGET_OBJ)/test

$(TARGET_OBJ)/test/%/%.o: CFLAGS = $(TEST_CFLAGS)
$(TARGET_OBJ)/test/%/%.o: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_OBJ)/test/%/%.o: $(SOURCE_TEST)/%/%.c
	$(object)

$(TARGET_OBJ)/test/%.o: CFLAGS = $(TEST_CFLAGS)
$(TARGET_OBJ)/test/%.o: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_OBJ)/test/%.o: $(SOURCE_TEST)/%.c
	$(object)

.PHONY: test/mod_lua_test
test/mod_lua_test: $(TARGET_BIN)/test/mod_lua_test
$(TARGET_BIN)/test/mod_lua_test: CFLAGS = $(TEST_CFLAGS)
$(TARGET_BIN)/test/mod_lua_test: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_BIN)/test/mod_lua_test: $(TEST_MOD_LUA:%=$(TARGET_OBJ)/test/%.o) $(TARGET_OBJ)/test/test.o $(wildcard $(TARGET_OBJ)/*) | modules build prepare
	$(executable) $(TEST_DEPS)
