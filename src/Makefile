# Citrusleaf Foundation
# Makefile

DEPTH = ../..
include $(DEPTH)/make_in/Makefile.in

DIR_INCLUDE = ../include
DIR_OBJECT = ../obj
DIR_TARGET = ../lib

HEADERS = alloc.h atomic.h cf.h digest.h fault.h lock.h queue.h rb.h socket.h msg.h bbhash.h rchash.h
SOURCES = alloc.c fault.c queue.c rb.c socket.c id.c msg.c bbhash.c clock.c rchash.c
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
	$(CC) $(CFLAGS) -MMD -o $@ -c $(INCLUDES) $<
