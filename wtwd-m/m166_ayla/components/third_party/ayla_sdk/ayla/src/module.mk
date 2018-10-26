MODULE_PATH := $(TOPDIR)/components/third_party/ayla_sdk/ayla/src

C_SRC := $(wildcard $(MODULE_PATH)/libayla/*.c)
C_SRC += $(wildcard $(MODULE_PATH)/libada/*.c)
C_SRC += $(wildcard $(MODULE_PATH)/ext/jsmn/*.c)
 
LIB_SRC := $(patsubst $(MODULE_PATH)/%, %,$(C_SRC))

LIB_ASRC :=
LIBRARY_NAME := ayla_sdk
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=
LOCAL_INC += -I$(MODULE_PATH)/include/ada
LOCAL_INC += -I$(MODULE_PATH)/include/ayla
LOCAL_INC += -I$(MODULE_PATH)/include/../include/ada
LOCAL_INC += -I$(MODULE_PATH)/include/../include/al
LOCAL_INC += -I$(MODULE_PATH)/include
LOCAL_INC += -I$(MODULE_PATH)/../include
LOCAL_INC += -I$(MODULE_PATH)/../../platform/include
LOCAL_INC += -I$(MODULE_PATH)/../../platform/ssv6266/include
LOCAL_INC += -I$(MODULE_PATH)/ext/jsmn

RELEASE_SRC := 2

$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))

