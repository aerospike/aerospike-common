# Citrusleaf Foundation
# Makefile

DEPTH = ../..
include $(DEPTH)/make_in/Makefile.in

DIR_INCLUDE = ../include
DIR_OBJECT = ../obj
DIR_TARGET = ../lib

HEADERS = alloc.h atomic.h cf.h clock.h digest.h fault.h lock.h queue.h rb.h socket.h msg.h 
HEADERS += rchash.h shash.h dynbuf.h timer.h ll.h vector.h cf_str.h hist.h bits.h olock.h
HEADERS += rcrb.h cf_random.h meminfo.h arenax.h b64.h rbuffer.h vmapx.h
HEADERS += ttree.h slist.h cf_index.h hist_track.h xmem.h

SOURCES = alloc.c fault.c queue.c rb.c socket.c id.c msg.c clock.c rchash.c sort.c 
SOURCES += shash.c cf_str.c dynbuf.c timer.c ll.c vector.c daemon.c hist.c bits.c olock.c
SOURCES += rcrb.c cf_random.c meminfo.c arenax.c b64.c rbuffer.c vmapx.c
SOURCES += ttree.c slist.c cf_index.c hist_track.c xmem.c

TARGET = libcf.a

ifeq ($(MEM_COUNT),1)
  LIBRARIES += -lm
  CFLAGS += -DMEM_COUNT
endif

OBJECTS = $(SOURCES:%.c=$(DIR_OBJECT)/%.o)
DEPENDENCIES = $(OBJECTS:%.o=%.d)

all: libcf.a

libcf.a: $(call objects, $(OBJECTS)) | $(TARGET_LIB)
	$(call archive, $(empty), $(empty), $(empty), $(empty))

-include $(DEPENDENCIES)

$(DIR_OBJECT)/%.o: %.c
	$(CC) $(CFLAGS_NATIVE) -MMD -o $@ -c $(INCLUDES) $<
