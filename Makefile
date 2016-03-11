###############################################################################
##  SETTINGS                                                                 ##
###############################################################################
include project/settings.mk

# Modules
MODULES :=

# Override optimizations via: make O=n
O = 3

# Make-local Compiler Flags
CC_FLAGS = -std=gnu99 -g -Wall -fPIC -O$(O)
CC_FLAGS += -fno-common -fno-strict-aliasing
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
ifneq ($(wildcard /usr/local/opt/openssl/include),)
  CC_FLAGS += -I/usr/local/opt/openssl/include
endif
else
CC_FLAGS += -finline-functions -rdynamic
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

###############################################################################
##  OBJECTS                                                                  ##
###############################################################################

AEROSPIKE-OBJECTS =
AEROSPIKE-OBJECTS += as_aerospike.o
AEROSPIKE-OBJECTS += as_arraylist.o
AEROSPIKE-OBJECTS += as_arraylist_hooks.o
AEROSPIKE-OBJECTS += as_arraylist_iterator.o
AEROSPIKE-OBJECTS += as_arraylist_iterator_hooks.o
AEROSPIKE-OBJECTS += as_boolean.o
AEROSPIKE-OBJECTS += as_buffer.o
AEROSPIKE-OBJECTS += as_buffer_pool.o
AEROSPIKE-OBJECTS += as_bytes.o
AEROSPIKE-OBJECTS += as_double.o
AEROSPIKE-OBJECTS += as_geojson.o
AEROSPIKE-OBJECTS += as_hashmap.o
AEROSPIKE-OBJECTS += as_hashmap_hooks.o
AEROSPIKE-OBJECTS += as_hashmap_iterator.o
AEROSPIKE-OBJECTS += as_hashmap_iterator_hooks.o
AEROSPIKE-OBJECTS += as_integer.o
AEROSPIKE-OBJECTS += as_iterator.o
AEROSPIKE-OBJECTS += as_list.o
AEROSPIKE-OBJECTS += as_log.o
AEROSPIKE-OBJECTS += as_map.o
AEROSPIKE-OBJECTS += as_memtracker.o
AEROSPIKE-OBJECTS += as_module.o
AEROSPIKE-OBJECTS += as_msgpack.o
AEROSPIKE-OBJECTS += as_msgpack_serializer.o
AEROSPIKE-OBJECTS += as_nil.o
AEROSPIKE-OBJECTS += as_pair.o
AEROSPIKE-OBJECTS += as_password.o
AEROSPIKE-OBJECTS += as_queue.o
AEROSPIKE-OBJECTS += as_random.o
AEROSPIKE-OBJECTS += as_rec.o
AEROSPIKE-OBJECTS += as_result.o
AEROSPIKE-OBJECTS += as_serializer.o
AEROSPIKE-OBJECTS += as_stream.o
AEROSPIKE-OBJECTS += as_string.o
AEROSPIKE-OBJECTS += as_string_builder.o
AEROSPIKE-OBJECTS += as_thread_pool.o
AEROSPIKE-OBJECTS += as_timer.o
AEROSPIKE-OBJECTS += as_val.o
AEROSPIKE-OBJECTS += as_vector.o
AEROSPIKE-OBJECTS += crypt_blowfish.o

CITRUSLEAF-OBJECTS =
CITRUSLEAF-OBJECTS += cf_alloc.o
CITRUSLEAF-OBJECTS += cf_b64.o
CITRUSLEAF-OBJECTS += cf_clock.o
CITRUSLEAF-OBJECTS += cf_crypto.o
CITRUSLEAF-OBJECTS += cf_digest.o
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

HEADER-SRC = $(shell find $(SOURCE_INCL) -type f -name "*.h")
HEADER-TRG = $(patsubst $(SOURCE_INCL)/%.h, $(TARGET_INCL)/%.h, $(HEADER-SRC))

###############################################################################
##  MAIN TARGETS                                                             ##
###############################################################################

.PHONY: all
all: build prepare

.PHONY: build 
build: libaerospike-common

.PHONY: prepare
prepare: $(HEADER-TRG)

.PHONY: clean
clean:
	@rm -rf $(TARGET)

.PHONY: libaerospike-common libaerospike-common.a libaerospike-common.$(DYNAMIC_SUFFIX)
libaerospike-common: libaerospike-common.a libaerospike-common.$(DYNAMIC_SUFFIX)
libaerospike-common.a: $(TARGET_LIB)/libaerospike-common.a
libaerospike-common.$(DYNAMIC_SUFFIX): $(TARGET_LIB)/libaerospike-common.$(DYNAMIC_SUFFIX)

###############################################################################
##  BUILD TARGETS                                                            ##
###############################################################################

$(TARGET_OBJ)/common/aerospike/%.o: $(SOURCE_MAIN)/aerospike/%.c $(SOURCE_INCL)/aerospike/*.h
	$(object)

$(TARGET_OBJ)/common/citrusleaf/%.o: $(SOURCE_MAIN)/citrusleaf/%.c $(SOURCE_INCL)/citrusleaf/*.h
	$(object)

$(TARGET_LIB)/libaerospike-common.$(DYNAMIC_SUFFIX): $(OBJECTS)
	$(library)

$(TARGET_LIB)/libaerospike-common.a: $(OBJECTS)
	$(archive)

$(TARGET_INCL)/%.h: $(SOURCE_INCL)/%.h
	@mkdir -p $(@D)
	cp $< $@

###############################################################################
include project/test.mk project/rules.mk
