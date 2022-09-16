###############################################################################
##  TEST FLAGS                                                       		 ##
###############################################################################

TEST_VALGRIND = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes -v

TEST_CFLAGS = -I$(TARGET_INCL)

ifeq ($(OS),Darwin)
  ifneq ($(wildcard /opt/homebrew/opt/openssl/lib),)
    # Mac new homebrew openssl lib path
    TEST_LDFLAGS = -L/opt/homebrew/opt/openssl/lib
  else
    # Mac old homebrew openssl lib path
    TEST_LDFLAGS = -L/usr/local/opt/openssl/lib
  endif
  TEST_LDFLAGS += -lssl -lcrypto -lpthread -lm
else
  TEST_LDFLAGS = -lssl -lcrypto -lpthread -lm -lrt
endif

TEST_DEPS =
TEST_DEPS += $(TARGET_LIB)/libaerospike-common.a

###############################################################################
##  TEST OBJECTS                                                       		 ##
###############################################################################

TEST_AEROSPIKE = common.c
TEST_AEROSPIKE += test.c
TEST_AEROSPIKE += test_common.c
TEST_AEROSPIKE += types/*.c
TEST_AEROSPIKE += msgpack/*.c

TEST_SOURCE = $(wildcard $(addprefix $(SOURCE_TEST)/, $(TEST_AEROSPIKE)))

TEST_OBJECT = $(patsubst %.c,%.o,$(subst $(SOURCE_TEST)/,$(TARGET_TEST)/,$(TEST_SOURCE)))

###############################################################################
##  TEST TARGETS                                                      		 ##
###############################################################################

.PHONY: test
test: test-build
	$(TARGET_TEST)/common

.PHONY: test-valgrind
test-valgrind: test-build
	valgrind $(TEST_VALGRIND) $(TARGET_TEST)/common 1>&2 2>test-valgrind

.PHONY: test-build
test-build: $(TARGET_TEST)/common

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

$(TARGET_TEST)/common: CFLAGS = $(TEST_CFLAGS)
$(TARGET_TEST)/common: LDFLAGS = $(TEST_DEPS) $(TEST_LDFLAGS)
$(TARGET_TEST)/common: $(TEST_OBJECT) $(wildcard $(TARGET_OBJ)/*) | build prepare
	$(executable)
