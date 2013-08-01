###############################################################################
##  TEST FLAGS                                                       		 ##
###############################################################################

TEST_VALGRIND = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes -v

TEST_CFLAGS =  -DMEM_COUNT=1
TEST_CFLAGS += -I$(TARGET_INCL)

TEST_LDFLAGS = -lssl -lcrypto -llua -lpthread -lm -lrt 

TEST_DEPS =
# TEST_DEPS += $(MSGPACK)/src/.libs/libmsgpackc.a
TEST_DEPS += $(TARGET_LIB)/libaerospike-common.a
TEST_DEPS += $(addprefix $(MSGPACK)/src/.libs/, unpack.o objectc.o version.o vrefbuffer.o zone.o)

###############################################################################
##  TEST OBJECTS                                                       		 ##
###############################################################################

TEST_TYPES = 
TEST_TYPES += types/types_boolean
TEST_TYPES += types/types_integer
TEST_TYPES += types/types_string
TEST_TYPES += types/types_bytes
TEST_TYPES += types/types_arraylist
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
	@$(TARGET_TEST)/common

.PHONY: test-valgrind
test-valgrind: test-build
	valgrind $(TEST_VALGRIND) $(TARGET_TEST)/common 1>&2 2>test-valgrind

.PHONY: test-build
test-build: test/common

.PHONY: test-clean
test-clean: 
	@rm -rf $(TARGET_TEST)

$(TARGET_TEST)/%/%.o: CFLAGS = $(TEST_CFLAGS)
$(TARGET_TEST)/%/%.o: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_TEST)/%/%.o: $(SOURCE_TEST)/%/%.c
	$(object)

$(TARGET_TEST)/%.o: CFLAGS = $(TEST_CFLAGS)
$(TARGET_TEST)/%.o: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_TEST)/%.o: $(SOURCE_TEST)/%.c
	$(object)

.PHONY: test/common
test/common: $(TARGET_TEST)/common
$(TARGET_TEST)/common: CFLAGS = $(TEST_CFLAGS)
$(TARGET_TEST)/common: LDFLAGS += $(TEST_LDFLAGS)
$(TARGET_TEST)/common: $(TESTS:%=$(TARGET_TEST)/%.o) $(TARGET_TEST)/test.o $(wildcard $(TARGET_OBJ)/*) | modules build prepare
	$(executable) $(TEST_DEPS)
