MODULE_PATH := $(TOPDIR)/components/third_party/ayla_sdk/examples/ledevb

LIB_SRC :=  $(notdir $(wildcard $(MODULE_PATH)/*.c))

# LIB_SRC :=  $(notdir $(C_SRC))
LIB_ASRC :=

LIBRARY_NAME := ayla_ledevb

LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC := -I$(MODULE_PATH)/../include
LOCAL_INC += -I$(MODULE_PATH)/include/
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include/mbedtls
LOCAL_INC += -I$(MODULE_PATH)/../../ayla/include
LOCAL_INC += -I$(MODULE_PATH)/../../ayla/src/include
LOCAL_INC += -I$(MODULE_PATH)/../../platform/include
LOCAL_INC += -I$(MODULE_PATH)/../../platform/ssv6266/include

$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))
