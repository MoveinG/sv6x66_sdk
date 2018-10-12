LIB_SRC := 
#LIB_SRC += MQTTConnectClient.c
#LIB_SRC += MQTTConnectServer.c
#LIB_SRC += MQTTDeserializePublish.c
#LIB_SRC += MQTTPacket.c
#LIB_SRC += MQTTSerializePublish.c
#LIB_SRC += MQTTSNConnectClient.c
#LIB_SRC += MQTTSNConnectServer.c
#LIB_SRC += MQTTSNDeserializePublish.c
#LIB_SRC += MQTTSNPacket.c
#LIB_SRC += MQTTSNSearchClient.c
#LIB_SRC += MQTTSNSearchServer.c
#LIB_SRC += MQTTSNSerializePublish.c
#LIB_SRC += MQTTSNSubscribeClient.c
#LIB_SRC += MQTTSNSubscribeServer.c
#LIB_SRC += MQTTSNUnsubscribeClient.c
#LIB_SRC += MQTTSNUnsubscribeServer.c
#LIB_SRC += MQTTSubscribeClient.c
#LIB_SRC += MQTTSubscribeServer.c
#LIB_SRC += MQTTUnsubscribeClient.c
#LIB_SRC += MQTTUnsubscribeServer.c
#LIB_SRC += xlink_aes.c
#LIB_SRC += xlink_cloud.c
#LIB_SRC += xlink_flash.c
#LIB_SRC += xlink_local.c
#LIB_SRC += xlink_md5.c
#LIB_SRC += xlink_queue.c
#LIB_SRC += xlink_sample_DH.c
#LIB_SRC += xlink_sdk_process.c

LIB_SRC += xlink_sdk.c linux_flash.c

STATIC_LIB += $(MYDIR)/libXlinkV5.a

LIB_ASRC :=
LIBRARY_NAME := Xlink
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC := -I$(TOPDIR)/components/third_party/xlink
LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include/lwip
LOCAL_INC += -I$(PROJ_DIR)/src/cfg

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))
