# Citrusleaf Foundation
# Makefile

DIR_INCLUDE = ../include
DIR_OBJECT = ../obj
DIR_TARGET = ../lib

HEADERS = alloc.h atomic.h cf.h digest.h fault.h lock.h queue.h rb.h socket.h
SOURCES = alloc.c fault.c queue.c rb.c socket.c
TARGET = libcf.a

SYSTEM = $(shell uname -s)
ifeq (${SYSTEM},Linux)
	SOURCES += event_epoll.c
else
	SOURCES += event_kevent.c
endif

CC = gcc
CFLAGS = -m64 -g -fno-common -fomit-frame-pointer -O3 -std=gnu99 -Wall -D_REENTRANT
CFLAGS += -MMD
LDFLAGS = $(CFLAGS)
INCLUDES = -I$(DIR_INCLUDE)

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
	$(CC) $(CFLAGS)  -o $@ -c $(INCLUDES) $<
