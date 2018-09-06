LIB_SRC := 
LIB_SRC += colink_sysadapter.c
LIB_SRC += colink_setting.c
LIB_SRC += colink_socket.c
LIB_SRC += colink_network.c
LIB_SRC += colink_upgrade.c
LIB_SRC += colink_global.c

LIB_SRC += esp_touch.c

STATIC_LIB += $(MYDIR)/libcolink.a
STATIC_LIB += $(MYDIR)/libcolinkcJSON.a
STATIC_LIB += $(TOPDIR)/out/components/third_party/colink/colink_socket.o

LIB_ASRC :=
LIBRARY_NAME := colink
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC := -I$(TOPDIR)/components/third_party/colink/include
LOCAL_INC += -I$(TOPDIR)/components/osal
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include
LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include
LOCAL_INC += -I$(TOPDIR)/components/tools/ota_api

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))
