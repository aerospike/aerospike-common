ROOT = .
###############################################################################
include $(ROOT)/project/settings.makefile
###############################################################################

###############################################################################
##  FLAGS                                                                    ##
###############################################################################

CFLAGS = -O3

CC_FLAGS = -std=gnu99 -g -rdynamic -Wall 
CC_FLAGS += -fPIC -fno-common -fno-strict-aliasing
CC_FLAGS += -DMARCH_$(ARCH) -D_FILE_OFFSET_BITS=64 
CC_FLAGS += -D_REENTRANT -D_GNU_SOURCE -DMEM_COUNT=1

LD_FLAGS = $(LDGLAGS) -lm -fPIC 

###############################################################################
##  OBJECTS                                                                  ##
###############################################################################

CITRUSLEAF =
CITRUSLEAF += cf_b64.o
CITRUSLEAF += cf_bits.o
CITRUSLEAF += cf_clock.o
CITRUSLEAF += cf_crypto.o
CITRUSLEAF += cf_digest.o
CITRUSLEAF += cf_ll.o
CITRUSLEAF += cf_rchash.o
CITRUSLEAF += cf_shash.o
CITRUSLEAF += cf_vector.o

AEROSPIKE =

OBJECTS = $(CITRUSLEAF:%=$(TARGET_OBJ)/citrusleaf/%) $(AEROSPIKE:%=$(TARGET_OBJ)/aerospike/%)

###############################################################################
##  MAIN TARGETS                                                             ##
###############################################################################

.PHONY: all
all: build prepare

.PHONY: prepare
prepare: $(TARGET_INCL)/citrusleaf/*.h

.PHONY: build 
build: libcf-common

.PHONY: libcf-common libcf-common.a libcf-common.so
libcf-common: libcf-common.a libcf-common.so
libcf-common.a: $(TARGET_LIB)/libcf-common.a
libcf-common.so: $(TARGET_LIB)/libcf-common.so

###############################################################################
##  BUILD TARGETS                                                            ##
###############################################################################

$(TARGET_LIB)/libcf-common.a $(TARGET_LIB)/libcf-common.so: $(OBJECTS)

$(TARGET_INCL)/citrusleaf: | $(TARGET_INCL)
	mkdir $@

$(TARGET_INCL)/citrusleaf/%.h: $(SOURCE_INCL)/citrusleaf/%.h | $(TARGET_INCL)/citrusleaf
	cp -p $^ $(TARGET_INCL)/citrusleaf

$(TARGET_INCL)/aerospike: | $(TARGET_INCL)
	mkdir $@

$(TARGET_INCL)/aerospike/%.h:: $(SOURCE_INCL)/aerospike/%.h | $(TARGET_INCL)/aerospike
	cp -p $^ $@

###############################################################################
include $(RULES)
###############################################################################