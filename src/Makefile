# Citrusleaf Foundation
# Makefile

DEPTH = ../..
include $(DEPTH)/make_in/Makefile.in

DIR_INCLUDE = ../include
DIR_OBJECT = ../obj
DIR_TARGET = ../lib

HEADERS = alloc.h atomic.h cf.h clock.h digest.h fault.h lock.h queue.h rb.h socket.h msg.h 
HEADERS += rchash.h shash.h dynbuf.h timer.h ll.h vector.h cf_str.h hist.h bits.h olock.h
HEADERS += rcrb.h cf_random.h arena.h meminfo.h arenah.h arena.h b64.h rbuffer.h

SOURCES = alloc.c fault.c queue.c rb.c socket.c id.c msg.c clock.c rchash.c sort.c 
SOURCES += shash.c cf_str.c dynbuf.c timer.c ll.c vector.c daemon.c hist.c bits.c olock.c
SOURCES += rcrb.c cf_random.c arena.c meminfo.c arenah.c arena.c b64.c rbuffer.c

TARGET = libcf.a

INCLUDES = -I$(DIR_CF_INCLUDE)

OBJECTS = $(SOURCES:%.c=$(DIR_OBJECT)/%.o)
DEPENDENCIES = $(OBJECTS:%.o=%.d)

.PHONY: all
all: libcf

.PHONY: clean
clean:
	/bin/rm -f $(OBJECTS) $(DIR_TARGET)/$(TARGET)

.PHONY: depclean
depclean: clean
	/bin/rm -f $(DEPENDENCIES)

.PHONY: libcf
libcf: $(OBJECTS)
	ar rs $(DIR_TARGET)/$(TARGET) $(OBJECTS)

-include $(DEPENDENCIES)

$(DIR_OBJECT)/%.o: %.c
	$(CC) $(CFLAGS_NATIVE) -MMD -o $@ -c $(INCLUDES) $<
