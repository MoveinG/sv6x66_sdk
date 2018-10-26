MODULE_PATH := $(TOPDIR)/components/third_party/ayla_sdk/ayla/test/altest



C_SRC := $(wildcard $(MODULE_PATH)/testcases/*.c)
C_SRC += $(wildcard $(MODULE_PATH)/*.c)
LIB_SRC := $(patsubst $(MODULE_PATH)/%, %,$(C_SRC))

LIB_ASRC :=
LIBRARY_NAME := ayla_test
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=
LOCAL_INC += -I$(MODULE_PATH)/include/ada
LOCAL_INC += -I$(MODULE_PATH)/include/ayla
LOCAL_INC += -I$(MODULE_PATH)/include/../include/ada
LOCAL_INC += -I$(MODULE_PATH)/include/../include/al
# $(error $(LIB_SRC))

RELEASE_SRC := 2

$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))

