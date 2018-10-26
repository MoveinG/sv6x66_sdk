#
# Copyright 2017 Ayla Networks, Inc.  All rights reserved.
#

#
# Toolchain and dependency setup
#
# For cross compilation, define:
#   TOOLCHAIN_DIR - where to find standard libraries for the target plaform
#   ARCH - the compiler prefix
#   EXTLIB_DIR - where to find required external libraries
#
# For native compilation, these variables can be left undefined
#
QUIET ?= @

ARCH ?= native
PLATFORM ?= linux

ifneq ($(TOOLCHAIN_DIR),)
	TOOLCHAIN_DIR := $(abspath $(TOOLCHAIN_DIR))
	TOOL_DIR = $(TOOLCHAIN_DIR)/bin/
	TARGET_CFLAGS += -I$(TOOLCHAIN_DIR)/include
	TARGET_LDFLAGS += -L$(TOOLCHAIN_DIR)/lib
endif
ifneq ($(ARCH),native)
	export ARCH
	export TOOL_ROOT = $(TOOL_DIR)$(ARCH)
	export CC = $(TOOL_ROOT)-gcc
	export AR = $(TOOL_ROOT)-ar
endif

#
# Install directory
#
# INSTALL_ROOT can be defined externally to
# customize the make install behavior

INSTALL_ROOT ?= $(SRC)/build/$(ARCH)
INSTALL_ROOT := $(abspath $(INSTALL_ROOT))

INSTALL := install -D

#
# Build directories
#
BUILD_ROOT = $(SRC)/build/$(ARCH)
BUILD = $(BUILD_ROOT)/$(DIR)

#
# Find 'checkpatch_ayla' code' style checking script
#
CSTYLE := $(wildcard $(SRC)/../../../../../../../../util/checkpatch_ayla)
ifeq ($(CSTYLE),)
CSTYLE := $(shell which checkpatch_ayla)
endif

#
# Compiler setup
#
DEFINES += AL_HAVE_SIZE_T AL_HAVE_SSIZE_T

DEFINES += ADA_BUILD_FINAL
DEFINES += ADA_BUILD_LOG_LOCK
DEFINES += ADA_BUILD_LOG_NO_BUF
DEFINES += ADA_BUILD_LOG_SERIAL
DEFINES += ADA_BUILD_LOG_THREAD_CT=8
DEFINES += ADA_BUILD_ANS_SUPPORT
DEFINES += CONF_NO_ID_FILE

ifeq ($(TYPE),RELEASE)
DEFINES += NDEBUG
TARGET_CFLAGS += -O2
else
TARGET_CFLAGS += -O0 -g -ggdb
endif
TARGET_CFLAGS += -Wall -Wuninitialized -Wunused -Werror -std=gnu99
TARGET_CFLAGS += $(addprefix -D,$(sort $(DEFINES)))

#
# Build files
#
OBJS = $(SOURCES:%.c=$(BUILD)/%.o)
DEPS = $(SOURCES:%.c=$(BUILD)/%.d)

#
# Style checking
#
STYLE_OK := $(wildcard .style_ok)
ifneq ($(STYLE_OK),)
FILE_IGNORE := $(sort $(shell cat .style_ok))
DIR_IGNORE := $(filter %/*,$(FILE_IGNORE))
ifneq ($(DIR_IGNORE),)
FILE_IGNORE := $(filter-out $(DIR_IGNORE), $(FILE_IGNORE))
FILE_IGNORE += $(shell find $(DIR_IGNORE) -type f)
endif
endif
STYLE_CHECK_C := $(filter-out $(FILE_IGNORE), $(SOURCES))
STYLE_CHECK_H := $(filter-out $(FILE_IGNORE),\
	$(foreach dir,$(CSTYLE_HEADER_DIR) .,\
	$(shell find $(dir) -type f -name '*.h')))
CSTYLES :=
ifneq ($(CSTYLE),)
CSTYLES += $(STYLE_CHECK_C:%.c=$(BUILD)/%.cs)
CSTYLES += $(STYLE_CHECK_H:%.h=$(BUILD)/%.hcs)
endif

ADA_LIBS = ada ayla plat-$(PLATFORM)

# Generates a separate section for each function, so it removes unused functions
TARGET_CFLAGS += -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -Wl,--gc-sections

# Setup search directory for header files and library files
TARGET_CFLAGS += -I$(SRC)/examples/include
TARGET_CFLAGS += -I$(SRC)/platform/$(PLATFORM)/include -I$(SRC)/platform/include
TARGET_CFLAGS += -I$(SRC)/ayla/include -I$(SRC)/ayla/src/include
TARGET_CFLAGS += -I$(SRC)/ayla/src/ext/jsmn
TARGET_LDFLAGS += -L$(SRC)/ayla/lib/$(ARCH)

# Cross compile x86 code on x64 version linux
ifeq (native,$(ARCH))
TARGET_CFLAGS += -m32
TARGET_LDFLAGS += -m32
endif

# Point to required libraries.
# Some circular dependencies exist between libayla and libal
TARGET_LDLIBS += -Wl,-start-group $(addprefix -l,$(ADA_LIBS)) -Wl,-end-group

# Libraries listed by target makefiles in LIBS variable
TARGET_LDLIBS += $(foreach lib,$(LIBS),-l$(lib))

# Common standard libs
TARGET_LDLIBS += -lssl -lcrypto -lpthread -lrt -lm -lanl -ldl

# External library directories listed in EXTLIB_DIR variable
ifdef EXTLIB_DIR
TARGET_CFLAGS += $(foreach path,$(EXTLIB_DIR),-I$(path)/include/$(ARCH) -I$(path)/include)
TARGET_LDFLAGS += $(foreach path,$(EXTLIB_DIR),-L$(path)/lib/$(ARCH))
endif

#
# Override common implicit variables to preserve defined values
#
override CFLAGS += $(TARGET_CFLAGS)
override LDFLAGS += $(TARGET_LDFLAGS)
override LDLIBS += $(TARGET_LDLIBS)
