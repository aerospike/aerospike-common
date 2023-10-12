###############################################################################
##  BUILD VARIABLES                                                          ##
###############################################################################

export CFLAGS =
export LDFLAGS =
export ARFLAGS =

###############################################################################
##  BUILD ENVIRONMENT                                                        ##
###############################################################################

NAME = $(shell basename $(CURDIR))
OS = $(shell uname)
ARCH = $(shell uname -m)

PROJECT = project
SOURCE  = src
TARGET  = target
LIBRARIES =

###############################################################################
##  BUILD TOOLS                                                              ##
###############################################################################

ifeq ($(OS),Darwin)
  DYNAMIC_SUFFIX=dylib
  DYNAMIC_FLAG=-dynamiclib
else
  DYNAMIC_SUFFIX=so
  DYNAMIC_FLAG=-shared
endif

CC ?= cc
CC_FLAGS =

LD ?= cc
LD_FLAGS =

AR ?= ar
AR_FLAGS =

###############################################################################
##  SOURCE PATHS                                                             ##
###############################################################################

SOURCE_PATH = $(SOURCE)
SOURCE_MAIN = $(SOURCE_PATH)/main
SOURCE_INCL = $(SOURCE_PATH)/include
SOURCE_TEST = $(SOURCE_PATH)/test

VPATH = $(SOURCE_MAIN) $(SOURCE_INCL)

LIB_PATH = 
INC_PATH = $(SOURCE_INCL)

###############################################################################
##  TARGET PATHS                                                             ##
###############################################################################

ifeq ($(shell test -e $(PROJECT)/target.$(DISTRO_NAME)-$(DISTRO_VERS)-$(ARCH).makefile && echo 1), 1)
PLATFORM = $(DISTRO_NAME)-$(DISTRO_VERS)-$(ARCH)
include $(PROJECT)/target.$(PLATFORM).makefile
else
ifeq ($(shell test -e $(PROJECT)/target.$(DISTRO_NAME)-$(ARCH).makefile && echo 1), 1)
PLATFORM = $(DISTRO_NAME)-$(ARCH)
include $(PROJECT)/target.$(PLATFORM).makefile
else
ifeq ($(shell test -e $(PROJECT)/target.$(OS)-$(ARCH).makefile && echo 1), 1)
PLATFORM = $(OS)-$(ARCH)
include $(PROJECT)/target.$(PLATFORM).makefile
else
ifeq ($(shell test -e project/target.$(OS)-noarch.makefile && echo 1), 1)
PLATFORM = $(OS)-noarch
include $(PROJECT)/target.$(PLATFORM).makefile
else
PLATFORM = $(OS)-$(ARCH)
endif
endif
endif
endif

TARGET_PATH = $(TARGET)
TARGET_BASE = $(TARGET_PATH)/$(PLATFORM)
TARGET_BIN 	= $(TARGET_BASE)/bin
TARGET_DOC 	= $(TARGET_BASE)/doc
TARGET_LIB 	= $(TARGET_BASE)/lib
TARGET_OBJ 	= $(TARGET_BASE)/obj
TARGET_M4	= $(TARGET_BASE)/m4
TARGET_SRC 	= $(TARGET_BASE)/src
TARGET_INCL = $(TARGET_BASE)/include
TARGET_TEST = $(TARGET_BASE)/test

###############################################################################
##  FUNCTIONS                                                                ##
###############################################################################

define executable
	@if [ ! -d `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(strip $(CC) \
		$(addprefix -I, $(INC_PATH)) \
		$(addprefix -L, $(LIB_PATH)) \
		$(addprefix -l, $(LIBRARIES)) \
		$(CC_FLAGS) \
		$(CFLAGS) \
		-o $@ \
		$(filter %.o %.a %.so, $^) \
		$(LD_FLAGS) \
	)
endef

define archive
	@if [ ! -d `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(strip $(AR) \
		rcs \
		$(AR_FLAGS) \
		$(ARFLAGS) \
		$@ \
		$(filter %.o, $^) \
	)
endef

define library
	@if [ ! -d `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(strip $(CC) $(DYNAMIC_FLAG) \
		$(addprefix -I, $(INC_PATH)) \
		$(addprefix -L, $(LIB_PATH)) \
		$(addprefix -l, $(LIBRARIES)) \
		$(LD_FLAGS) \
		$(LDFLAGS) \
		-o $@ \
		$(filter %.o, $^) \
	)
endef

define object
	@if [ ! -d `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(strip $(CC) \
		$(addprefix -I, $(INC_PATH)) \
		$(addprefix -L, $(LIB_PATH)) \
		$(CC_FLAGS) \
		$(CFLAGS) \
		-o $@ \
		-c $(filter %.c %.cpp, $^)  \
	)
endef
