include project/build.makefile

CFLAGS += -std=gnu99 -g -O3 -fno-common -fno-strict-aliasing -rdynamic  -Wall $(AS_CFLAGS) -DMARCH_$(ARCH) -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_GNU_SOURCE 

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
SHARED += cf_clock.o
SHARED += cf_digest.o
SHARED += cf_ll.o
SHARED += cf_rchash.o
SHARED += cf_shash.o
SHARED += cf_vector.o

ifeq ($(MEM_COUNT),1)
  LDFLAGS += -lm
  CFLAGS += -DMEM_COUNT
endif


# $(TARGET_OBJ)/client/%.o: $(SOURCE_MAIN)/client/%.c | $(TARGET_OBJ)
# 	$(call object, src/include/client)

# $(TARGET_OBJ)/server/%.o: $(SOURCE_MAIN)/server/%.c | $(TARGET_OBJ) 
# 	$(call object, src/include/server)

libcf-client.a: $(call objects, $(CLIENT), client) | $(TARGET_LIB)
	$(call archive, $(empty), $(empty), $(empty), $(empty))

libcf-server.a: $(call objects, $(SERVER), server) | $(TARGET_LIB)
	$(call archive, $(empty), $(empty), $(empty), $(empty))

libcf.a: libcf-client.a libcf-server.a

prepare: 
	mkdir -p $(TARGET_INCL)/client/citrusleaf
	cp $(SOURCE_INCL)/client/*.h $(TARGET_INCL)/client/citrusleaf/.
	cp $(SOURCE_INCL)/*.h $(TARGET_INCL)/client/.
	mkdir -p $(TARGET_INCL)/server
	cp $(SOURCE_INCL)/server/*.h $(TARGET_INCL)/server/.
	mkdir -p $(TARGET_INCL)/shared
	cp $(SOURCE_INCL)/*.h $(TARGET_INCL)/shared/.

all: libcf.a prepare
