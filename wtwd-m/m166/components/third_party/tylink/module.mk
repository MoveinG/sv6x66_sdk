LIB_SRC := 
LIB_SRC += tuya_uni_storge.c

LIB_SRC += tuya_uni_network.c
LIB_SRC += tuya_uni_semaphore.c
LIB_SRC += tuya_uni_system.c
LIB_SRC += tuya_uni_mutex.c
LIB_SRC += tuya_uni_output.c
LIB_SRC += tuya_uni_thread.c

LIB_SRC += wifi_hwl.c

LIB_ASRC :=
LIBRARY_NAME := tylink
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC := -I$(TOPDIR)/components/third_party/tylink
LOCAL_INC := -I$(TOPDIR)/components/osal
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include
LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))
