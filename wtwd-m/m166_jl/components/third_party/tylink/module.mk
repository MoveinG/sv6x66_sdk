LIB_SRC := 
LIB_SRC += adapter_src/tuya_uni_storge.c
LIB_SRC += adapter_src/tuya_uni_network.c
LIB_SRC += adapter_src/tuya_uni_semaphore.c
LIB_SRC += adapter_src/tuya_uni_system.c
LIB_SRC += adapter_src/tuya_uni_mutex.c
LIB_SRC += adapter_src/tuya_uni_output.c
LIB_SRC += adapter_src/tuya_uni_thread.c
LIB_SRC += adapter_src/wifi_hwl.c

LIB_SRC += tuya_main.c
ifeq ($(strip $(TY_MCU_MODE)), TRUE)
LIB_SRC += tuya_device_mcu.c
else
LIB_SRC += tuya_device.c
endif

LIB_SRC += hw_table.c
LIB_SRC += driver/tuya_gpio.c
LIB_SRC += driver/tuya_led.c
LIB_SRC += driver/tuya_key.c
LIB_SRC += driver/tuya_uart.c

STATIC_LIB += $(MYDIR)/libtylink.a
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_storge.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_network.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_semaphore.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_system.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_mutex.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_output.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/tuya_uni_thread.o
STATIC_LIB += $(TOPDIR)/out/components/third_party/tylink/adapter_src/wifi_hwl.o

LIB_ASRC :=
LIBRARY_NAME := tylink
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC := -I$(TOPDIR)/components/third_party/mbedtls/include
LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include
LOCAL_INC += -I$(TOPDIR)/components/tools/ota_api

LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/common
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_adapter
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_adapter/storage
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_adapter/system
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_adapter/utilities
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_adapter/wifi_intf
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_base/sys_serv
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_iot_sdk/base_sdk
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_iot_sdk/com_sdk
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_iot_sdk/tuya_cloud
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_iot_sdk/wifi_cfg_serv
LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/tuya_iot_sdk/wifi_sdk

LOCAL_INC += -I$(TOPDIR)/components/third_party/tylink/include/driver
LOCAL_INC += -I$(PROJ_DIR)/src/cfg

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))
