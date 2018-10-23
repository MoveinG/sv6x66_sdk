#ifndef __COLINK_DEFINE_H__
#define __COLINK_DEFINE_H__

#define os_printf	colinkPrintf
#define os_malloc	OS_MemAlloc
#define os_free		OS_MemFree
#define vTaskDelay	OS_MsDelay
#define vTaskDelete	OS_TaskDelete

typedef enum
{
    SIG_NETWORK_INIT                      = 1,
    SIG_NETWORK_COLINK_INIT                    = 2,
    SIG_NETWORK_JOIN_AP_ERR                  = 3,
    SIG_NETWORK_ESPTOUCH_START  = 4,
    SIG_NETWORK_ESPTOUCH_STOP   = 5,
    SIG_NETWORK_SETTING_START   = 6,
    SIG_NETWORK_SETTING_STOP   = 7,
    SIG_NETWORK_SOCKET_TEST   = 8,
    SIG_NETWORK_COLINK_PROCESS = 9,
}ColinkSignalSet;

#define DEVICE_CONFIG_START_SEC (0x7C)

#define CK_DEVICE_LEN	10
#define CK_APIKEY_LEN	36
#define CK_MODEL_LEN	10

typedef struct
{
	char			deviceid[10+1];
	char			apikey[36+1];
	unsigned char	sta_mac[6];
	unsigned char	sap_mac[6];
	char			model[10+1];
} ColinkDevice;

#define DEVICEID "100059f78c"
#define APIKEY "27d89262-c7e8-4c63-912e-80dfc2e98a57"
#define MODEL "C01-X01-GL"
#define VERDION "0.0.1"

#endif /* #ifndef __COLINK_DEFINE_H__ */
