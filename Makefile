include project/settings.mk
###############################################################################
##  SETTINGS                                                                 ##
###############################################################################

# Modules
MODULES := MSGPACK

# Overrride optimizations via: make O=n
O=3

# Make-local Compiler Flags
CC_FLAGS = -std=gnu99 -g -rdynamic -Wall 
CC_FLAGS += -fPIC -fno-common -fno-strict-aliasing
CC_FLAGS += -DMARCH_$(ARCH) -D_FILE_OFFSET_BITS=64 
CC_FLAGS += -D_REENTRANT -D_GNU_SOURCE -DMEM_COUNT=1

# Make-local Linker Flags
LD_FLAGS = $(LDGLAGS) -lm -fPIC 

# DEBUG Settings
ifdef DEBUG
O=0
CC_FLAGS += -pg -fprofile-arcs -ftest-coverage -g2
LD_FLAGS += -pg -fprofile-arcs -lgcov
endif

# Make-tree Compiler Flags
CFLAGS = -O$(O)

# Make-tree Linker Flags
# LDFLAGS = 

# Include Paths
INC_PATH += $(MSGPACK)/src $(CF)/include

# Library Paths
# LIB_PATH +=

###############################################################################
##  OBJECTS                                                                  ##
###############################################################################

AEROSPIKE =
AEROSPIKE += as_val.o
AEROSPIKE += as_module.o
AEROSPIKE += as_nil.o
AEROSPIKE += as_stream.o
AEROSPIKE += as_result.o
AEROSPIKE += as_aerospike.o
AEROSPIKE += as_logger.o
AEROSPIKE += as_memtracker.o
AEROSPIKE += as_buffer.o

AEROSPIKE += as_serializer.o
AEROSPIKE += as_msgpack.o
AEROSPIKE += as_msgpack_serializer.o

AEROSPIKE += as_boolean.o
AEROSPIKE += as_bytes.o
AEROSPIKE += as_integer.o
AEROSPIKE += as_list.o
AEROSPIKE += as_map.o
AEROSPIKE += as_string.o

AEROSPIKE += as_pair.o
AEROSPIKE += as_rec.o

AEROSPIKE += as_arraylist.o
AEROSPIKE += as_hashmap.o
AEROSPIKE += as_linkedlist.o
AEROSPIKE += as_stringmap.o

AEROSPIKE += as_iterator.o
AEROSPIKE += as_linkedlist_iterator.o
AEROSPIKE += as_arraylist_iterator.o
AEROSPIKE += as_hashmap_iterator.o

AEROSPIKE += internal.o


CITRUSLEAF =
CITRUSLEAF += cf_b64.o
CITRUSLEAF += cf_bits.o
CITRUSLEAF += cf_clock.o
CITRUSLEAF += cf_crypto.o
CITRUSLEAF += cf_digest.o
CITRUSLEAF += cf_hooks.o
CITRUSLEAF += cf_ll.o
CITRUSLEAF += cf_rchash.o
CITRUSLEAF += cf_shash.o
CITRUSLEAF += cf_vector.o

OBJECTS = $(CITRUSLEAF:%=$(TARGET_OBJ)/citrusleaf/%) $(AEROSPIKE:%=$(TARGET_OBJ)/aerospike/%)

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

.PHONY: libaerospike-common libaerospike-common.a libaerospike-common.so
libaerospike-common: libaerospike-common.a libaerospike-common.so
libaerospike-common.a: $(TARGET_LIB)/libaerospike-common.a
libaerospike-common.so: $(TARGET_LIB)/libaerospike-common.so


.PHONY: libaerospike-common-hooked libaerospike-common-hooked.a libaerospike-common-hooked.so
libaerospike-common-hooked: libaerospike-common-hooked.a libaerospike-common-hooked.so
libaerospike-common-hooked.a: $(TARGET_LIB)/libaerospike-common-hooked.a
libaerospike-common-hooked.so: $(TARGET_LIB)/libaerospike-common-hooked.so

###############################################################################
##  BUILD TARGETS                                                            ##
###############################################################################

# COMMON

$(TARGET_OBJ)/common/aerospike/%.o: $(SOURCE_MAIN)/aerospike/%.c $(SOURCE_INCL)/aerospike/*.h | modules 
	$(object)

$(TARGET_OBJ)/common/citrusleaf/%.o: $(SOURCE_MAIN)/citrusleaf/%.c $(SOURCE_INCL)/citrusleaf/*.h | modules 
	$(object)

$(TARGET_LIB)/libaerospike-common.so: $(AEROSPIKE:%=$(TARGET_OBJ)/common/aerospike/%) $(CITRUSLEAF:%=$(TARGET_OBJ)/common/citrusleaf/%) | modules 
	$(library)

$(TARGET_LIB)/libaerospike-common.a: $(AEROSPIKE:%=$(TARGET_OBJ)/common/aerospike/%) $(CITRUSLEAF:%=$(TARGET_OBJ)/common/citrusleaf/%) | modules 
	$(archive)

# HOOKED COMMON

$(TARGET_OBJ)/common-hooked/citrusleaf/%.o: CC_FLAGS += -DEXTERNAL_LOCKS
$(TARGET_OBJ)/common-hooked/citrusleaf/%.o: $(SOURCE_MAIN)/citrusleaf/%.c $(SOURCE_INCL)/citrusleaf/*.h | modules
	$(object)

$(TARGET_LIB)/libaerospike-common-hooked.so: $(CITRUSLEAF:%=$(TARGET_OBJ)/common-hooked/citrusleaf/%) | modules 
	$(library)

$(TARGET_LIB)/libaerospike-common-hooked.a: $(CITRUSLEAF:%=$(TARGET_OBJ)/common-hooked/citrusleaf/%) | modules 
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
