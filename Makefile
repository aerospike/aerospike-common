include project/settings.mk
###############################################################################
##  SETTINGS                                                                 ##
###############################################################################

# Modules
MODULES :=

# Override optimizations via: make O=n
O = 3

# Make-local Compiler Flags
CC_FLAGS = -std=gnu99 -g -Wall -fPIC -O$(O)
CC_FLAGS += -fno-common -fno-strict-aliasing -finline-functions
CC_FLAGS += -march=nocona -DMARCH_$(ARCH)
CC_FLAGS += -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_GNU_SOURCE $(EXT_CFLAGS)

PREPRO_SUFFIX = .cpp
ifeq ($(PREPRO),1)
  SUFFIX = $(PREPRO_SUFFIX)
  CC_FLAGS += -E -DPREPRO=$(PREPRO) -DGEN_TAG=$(GEN_TAG)"\
"
endif

ifeq ($(OS),Darwin)
CC_FLAGS += -D_DARWIN_UNLIMITED_SELECT
else
CC_FLAGS += -rdynamic
endif

ifneq ($(CF), )
CC_FLAGS += -I$(CF)/include
endif

# Linker flags
LD_FLAGS = $(LDFLAGS)

ifeq ($(OS),Darwin)
LD_FLAGS += -undefined dynamic_lookup
endif

# DEBUG Settings
ifdef DEBUG
O=0
CC_FLAGS += -pg -fprofile-arcs -ftest-coverage -g2
LD_FLAGS += -pg -fprofile-arcs -lgcov
endif

# Include Paths
# INC_PATH +=

# Library Paths
# LIB_PATH +=

###############################################################################
##  OBJECTS                                                                  ##
###############################################################################

AEROSPIKE-OBJECTS =
AEROSPIKE-OBJECTS += as_module.o
AEROSPIKE-OBJECTS += as_nil.o
AEROSPIKE-OBJECTS += as_result.o
AEROSPIKE-OBJECTS += as_aerospike.o
AEROSPIKE-OBJECTS += as_memtracker.o
AEROSPIKE-OBJECTS += as_buffer.o
AEROSPIKE-OBJECTS += as_pair.o
AEROSPIKE-OBJECTS += as_stream.o
AEROSPIKE-OBJECTS += as_iterator.o
AEROSPIKE-OBJECTS += as_timer.o

AEROSPIKE-OBJECTS += internal.o

# serializers
AEROSPIKE-OBJECTS += as_serializer.o
AEROSPIKE-OBJECTS += as_msgpack.o
AEROSPIKE-OBJECTS += as_msgpack_serializer.o

# types
AEROSPIKE-OBJECTS += as_boolean.o
AEROSPIKE-OBJECTS += as_bytes.o
AEROSPIKE-OBJECTS += as_double.o
AEROSPIKE-OBJECTS += as_integer.o
AEROSPIKE-OBJECTS += as_list.o
AEROSPIKE-OBJECTS += as_map.o
AEROSPIKE-OBJECTS += as_rec.o
AEROSPIKE-OBJECTS += as_string.o
AEROSPIKE-OBJECTS += as_string_builder.o
AEROSPIKE-OBJECTS += as_val.o

# arraylist
AEROSPIKE-OBJECTS += as_arraylist.o
AEROSPIKE-OBJECTS += as_arraylist_hooks.o
AEROSPIKE-OBJECTS += as_arraylist_iterator.o
AEROSPIKE-OBJECTS += as_arraylist_iterator_hooks.o

# hashmap
AEROSPIKE-OBJECTS += as_hashmap.o
AEROSPIKE-OBJECTS += as_hashmap_hooks.o
AEROSPIKE-OBJECTS += as_hashmap_iterator.o
AEROSPIKE-OBJECTS += as_hashmap_iterator_hooks.o

AEROSPIKE-OBJECTS += as_log.o
AEROSPIKE-OBJECTS += as_vector.o
AEROSPIKE-OBJECTS += as_password.o
AEROSPIKE-OBJECTS += as_buffer_pool.o
AEROSPIKE-OBJECTS += as_thread_pool.o
AEROSPIKE-OBJECTS += crypt_blowfish.o

CITRUSLEAF-OBJECTS =
CITRUSLEAF-OBJECTS += cf_alloc.o
CITRUSLEAF-OBJECTS += cf_b64.o
CITRUSLEAF-OBJECTS += cf_bits.o
CITRUSLEAF-OBJECTS += cf_clock.o
CITRUSLEAF-OBJECTS += cf_crypto.o
CITRUSLEAF-OBJECTS += cf_digest.o
CITRUSLEAF-OBJECTS += cf_hooks.o
CITRUSLEAF-OBJECTS += cf_ll.o
CITRUSLEAF-OBJECTS += cf_queue.o
CITRUSLEAF-OBJECTS += cf_queue_priority.o
CITRUSLEAF-OBJECTS += cf_random.o
CITRUSLEAF-OBJECTS += cf_rchash.o
CITRUSLEAF-OBJECTS += cf_shash.o
CITRUSLEAF-OBJECTS += cf_vector.o

OBJECTS =
OBJECTS += $(AEROSPIKE-OBJECTS:%=$(TARGET_OBJ)/common/aerospike/%) 
OBJECTS += $(CITRUSLEAF-OBJECTS:%=$(TARGET_OBJ)/common/citrusleaf/%)

HOOKED-OBJECTS = $(CITRUSLEAF-OBJECTS:%=$(TARGET_OBJ)/common-hooked/citrusleaf/%)

###############################################################################
##  MAIN TARGETS                                                             ##
###############################################################################

.PHONY: all
all: build prepare

.PHONY: prepare
prepare: $(TARGET_INCL)/citrusleaf/*.h $(TARGET_INCL)/aerospike/*.h

.PHONY: build 
build: libaerospike-common libaerospike-common-hooked

.PHONY: build-clean
build-clean:
	@rm -rf $(TARGET_BIN)
	@rm -rf $(TARGET_LIB)
	@rm -rf $(TARGET_OBJ)

.PHONY: libaerospike-common libaerospike-common.a libaerospike-common.$(DYNAMIC_SUFFIX)
libaerospike-common: libaerospike-common.a libaerospike-common.$(DYNAMIC_SUFFIX)
libaerospike-common.a: $(TARGET_LIB)/libaerospike-common.a
libaerospike-common.$(DYNAMIC_SUFFIX): $(TARGET_LIB)/libaerospike-common.$(DYNAMIC_SUFFIX)


.PHONY: libaerospike-common-hooked libaerospike-common-hooked.a libaerospike-common-hooked.$(DYNAMIC_SUFFIX)
libaerospike-common-hooked: libaerospike-common-hooked.a libaerospike-common-hooked.$(DYNAMIC_SUFFIX)
libaerospike-common-hooked.a: $(TARGET_LIB)/libaerospike-common-hooked.a
libaerospike-common-hooked.$(DYNAMIC_SUFFIX): $(TARGET_LIB)/libaerospike-common-hooked.$(DYNAMIC_SUFFIX)

###############################################################################
##  BUILD TARGETS                                                            ##
###############################################################################

# COMMON

$(TARGET_OBJ)/common/aerospike/%.o: $(SOURCE_MAIN)/aerospike/%.c $(SOURCE_INCL)/aerospike/*.h | modules 
	$(object)

$(TARGET_OBJ)/common/citrusleaf/%.o: $(SOURCE_MAIN)/citrusleaf/%.c $(SOURCE_INCL)/citrusleaf/*.h | modules 
	$(object)

$(TARGET_LIB)/libaerospike-common.$(DYNAMIC_SUFFIX): $(OBJECTS) | modules 
	$(library)

$(TARGET_LIB)/libaerospike-common.a: $(OBJECTS) | modules 
	$(archive)

# HOOKED COMMON

$(TARGET_OBJ)/common-hooked/citrusleaf/%.o: CC_FLAGS += -DEXTERNAL_LOCKS
$(TARGET_OBJ)/common-hooked/citrusleaf/%.o: $(SOURCE_MAIN)/citrusleaf/%.c $(SOURCE_INCL)/citrusleaf/*.h | modules
	$(object)

$(TARGET_LIB)/libaerospike-common-hooked.$(DYNAMIC_SUFFIX): $(HOOKED-OBJECTS) | modules 
	$(library)

$(TARGET_LIB)/libaerospike-common-hooked.a: $(HOOKED-OBJECTS) | modules 
	$(archive)

# COMMON HEADERS

$(TARGET_INCL)/citrusleaf: | $(TARGET_INCL)
	mkdir $@

$(TARGET_INCL)/citrusleaf/%.h: $(SOURCE_INCL)/citrusleaf/%.h | $(TARGET_INCL)/citrusleaf
	cp -p $^ $(TARGET_INCL)/citrusleaf

$(TARGET_INCL)/aerospike: | $(TARGET_INCL)
	mkdir $@

$(TARGET_INCL)/aerospike/%.h:: $(SOURCE_INCL)/aerospike/%.h | $(TARGET_INCL)/aerospike
	cp -p $^ $(TARGET_INCL)/aerospike

###############################################################################
include project/modules.mk project/test.mk project/rules.mk
