LIB_SRC := nopoll.c
LIB_SRC += nopoll_conn_opts.c
LIB_SRC += nopoll_decl.c
LIB_SRC += nopoll_io.c
LIB_SRC += nopoll_log.c
LIB_SRC += nopoll_msg.c
LIB_SRC += nopoll_conn.c
LIB_SRC += nopoll_ctx.c
LIB_SRC += nopoll_listener.c
LIB_SRC += nopoll_loop.c
#LIB_SRC += nopoll_win32.c
LIB_SRC += nopoll-regression-client.c

LIB_ASRC :=
LIBRARY_NAME := nopoll
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include/lwip
LOCAL_INC += -I$(TOPDIR)/components/third_party/iperf3.0
LOCAL_INC += -I$(TOPDIR)/components/third_party/openssl/include
LOCAL_INC += -I$(TOPDIR)/components/osal/freertos/kernel/Source/include

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))

