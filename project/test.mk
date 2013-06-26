###############################################################################
##  TEST FLAGS                                                       		 ##
###############################################################################

TEST_VALGRIND = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes -v

TEST_CFLAGS =  -DMEM_COUNT=1
TEST_CFLAGS += -I$(TARGET_INCL)

TEST_LDFLAGS = -lssl -lcrypto -llua -lpthread -lm -lrt 

TEST_DEPS =
TEST_DEPS += $(MSGPACK)/src/.libs/libmsgpackc.a
TEST_DEPS += $(TARGET_LIB)/libaerospike-common.a

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

TEST_UTIL = 
TEST_UTIL += util/cf_alloc

TESTS = common
TESTS += $(TEST_UTIL)
TESTS += $(TEST_TYPES) 

###############################################################################
##  TEST TARGETS                                                      		 ##
###############################################################################

.PHONY: test
test: test-build
	@$(TARGET_BIN)/test/common

.PHONY: test-valgrind
test-valgrind: test-build
	valgrind $(TEST_VALGRIND) $(TARGET_BIN)/test/common 1>&2 2>test-valgrind

.PHONY: test-build
test-build: test/common

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

.PHONY: test/common
test/common: $(TARGET_BIN)/test/common
$(TARGET_BIN)/test/common: CFLAGS = $(TEST_CFLAGS)
$(TARGET_BIN)/test/common: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_BIN)/test/common: $(TESTS:%=$(TARGET_OBJ)/test/%.o) $(TARGET_OBJ)/test/test.o $(wildcard $(TARGET_OBJ)/*) | modules build prepare
	$(executable) $(TEST_DEPS)
