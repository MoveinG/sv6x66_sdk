MODULE_PATH := $(TOPDIR)/components/third_party/ayla_sdk/platform/ssv6266

LIB_SRC :=  $(notdir $(wildcard $(MODULE_PATH)/*.c))

LIB_ASRC :=

LIBRARY_NAME := ayla_platform

LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC := -I$(MODULE_PATH)/../../ayla/include
LOCAL_INC := -I$(MODULE_PATH)/../../ayla/src/include
LOCAL_INC += -I$(MODULE_PATH)/include/
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include/mbedtls
LOCAL_INC += -I$(MODULE_PATH)/../../ayla/include
LOCAL_INC += -I$(MODULE_PATH)/../include/

RELEASE_SRC := 0
$(eval $(call link-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))