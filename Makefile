include project/build.makefile

###############################################################################
##  FLAGS                                                                    ##
###############################################################################

CFLAGS = -O3

CC_FLAGS = $(CFLAGS) -std=gnu99 -g -rdynamic -Wall 
CC_FLAGS += -fPIC -fno-common -fno-strict-aliasing
CC_FLAGS += -DMARCH_$(ARCH) -D_FILE_OFFSET_BITS=64 
CC_FLAGS += -D_REENTRANT -D_GNU_SOURCE -DMEM_COUNT=1

LD_FLAGS = $(LDGLAGS) -lm -fPIC 

###############################################################################
##  OBJECTS                                                                  ##
###############################################################################

SERVER = 
SERVER += alloc.o
SERVER += arenax.o
SERVER += bits.o
SERVER += b64.o
SERVER += cf_index.o
SERVER += cf_random.o
SERVER += cf_str.o
SERVER += clock.o
SERVER += daemon.o
SERVER += dynbuf.o
SERVER += fault.o
SERVER += hist.o
SERVER += hist_track.o
SERVER += id.o
SERVER += ll.o
SERVER += meminfo.o
SERVER += msg.o
SERVER += olock.o
SERVER += queue.o
SERVER += queue_priority.o
SERVER += rb.o
SERVER += rbuffer.o
SERVER += rchash.o
SERVER += rcrb.o
SERVER += shash.o
SERVER += slist.o
SERVER += socket.o
SERVER += sort.o
SERVER += timer.o
SERVER += ttree.o
SERVER += vector.o
SERVER += vmapx.o
SERVER += xmem.o

CLIENT = 
CLIENT += cf_alloc.o
CLIENT += cf_average.o
CLIENT += cf_b64.o
CLIENT += cf_clock.o
CLIENT += cf_digest.o
CLIENT += cf_hist.o
CLIENT += cf_hooks.o
CLIENT += cf_ll.o
CLIENT += cf_log.o
CLIENT += cf_queue.o
CLIENT += cf_queue_priority.o
CLIENT += cf_random.o
CLIENT += cf_rchash.o
CLIENT += cf_service.o
CLIENT += cf_shash.o
CLIENT += cf_socket.o
CLIENT += cf_vector.o

SHARED =
SHARED += cf_b64.o
SHARED += cf_bits.o
SHARED += cf_clock.o
SHARED += cf_digest.o
SHARED += cf_ll.o
SHARED += cf_rchash.o
SHARED += cf_shash.o
SHARED += cf_vector.o

###############################################################################
##  MAIN TARGETS                                                             ##
###############################################################################

.PHONY: all
all: build prepare

.PHONY: build 
build: libcf-client.a libcf-server.a libcf-shared.a

.PHONY: prepare
prepare: $(TARGET_INCL)

.PHONY: libcf-client.a libcf-server.a libcf-shared.a
libcf-client.a: $(TARGET_LIB)/libcf-client.a
libcf-server.a: $(TARGET_LIB)/libcf-server.a
libcf-shared.a: $(TARGET_LIB)/libcf-shared.a

.PHONY: client
client: libcf-client.a libcf-shared.a prepare

###############################################################################
##  BUILD TARGETS                                                            ##
###############################################################################

$(TARGET_LIB)/libcf-client.a: $(CLIENT:%=$(TARGET_OBJ)/client/%) | $(TARGET_LIB)
	$(archive)

$(TARGET_LIB)/libcf-server.a: $(SERVER:%=$(TARGET_OBJ)/server/%) | $(TARGET_LIB)
	$(archive)

$(TARGET_LIB)/libcf-shared.a: $(SHARED:%=$(TARGET_OBJ)/shared/%) | $(TARGET_LIB)
	$(archive)

$(TARGET_INCL):
	@mkdir -p $(TARGET_INCL)
	cp -p $(SOURCE_INCL)/*.h $(TARGET_INCL)/.
	@mkdir -p $(TARGET_INCL)/citrusleaf
	cp -p $(SOURCE_INCL)/client/*.h $(TARGET_INCL)/citrusleaf/.
	@mkdir -p $(TARGET_INCL)/server
	cp -p $(SOURCE_INCL)/server/*.h $(TARGET_INCL)/server/.


