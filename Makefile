include project/build.makefile

CFLAGS =  -D_FILE_OFFSET_BITS=64 -std=gnu99 -D_REENTRANT -D_GNU_SOURCE 
CFLAGS += -g -O3 -fno-common -fno-strict-aliasing -rdynamic  -Wall $(AS_CFLAGS) -D MARCH_$(ARCH)

SOURCES = alloc.c fault.c queue.c rb.c socket.c id.c msg.c clock.c rchash.c sort.c 
SOURCES += shash.c cf_str.c dynbuf.c timer.c ll.c vector.c daemon.c hist.c bits.c olock.c
SOURCES += rcrb.c cf_random.c meminfo.c arenax.c b64.c rbuffer.c vmapx.c
SOURCES += ttree.c slist.c cf_index.c hist_track.c xmem.c

OBJECTS = $(SOURCES:%.c=%.o)

ifeq ($(MEM_COUNT),1)
  LDFLAGS += -lm
  CFLAGS += -DMEM_COUNT
endif

libcf.a: $(call objects, $(OBJECTS)) | $(TARGET_LIB)
	$(call archive, $(empty), $(empty), $(empty), $(empty))

all: libcf.a
