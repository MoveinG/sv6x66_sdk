LIB_SRC := ssl/ssl_cert.c
LIB_SRC += ssl/ssl_lib.c
LIB_SRC += ssl/ssl_methods.c
LIB_SRC += ssl/ssl_pkey.c
LIB_SRC += ssl/ssl_pm.c
LIB_SRC += ssl/ssl_pm_extend.c
LIB_SRC += ssl/ssl_stack.c
LIB_SRC += ssl/ssl_x509.c

LIB_ASRC :=
LIBRARY_NAME := openssl
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC += -I$(TOPDIR)/components/third_party/openssl/include
LOCAL_INC += -I$(TOPDIR)/components/third_party/openssl/include/internal
LOCAL_INC += -I$(TOPDIR)/components/third_party/openssl/include/platform
LOCAL_INC += -I$(TOPDIR)/components/third_party/mbedtls/include
LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include/lwip

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))

