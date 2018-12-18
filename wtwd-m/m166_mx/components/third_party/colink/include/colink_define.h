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

//#define DEVICEID "10005807e9"
//#define APIKEY "cf761fd6-5195-4b32-92f5-e1c166c5bcd7"
//#define DEVICEID "100067ae93"     //for customer mx
//#define APIKEY "b89865f6-b59a-4eb0-8593-ce9ddc9f4e74"
//#define DEVICEID "100067ae94"    //for customer mx
//#define APIKEY "920aa750-12bf-4370-82bf-bdc02043326b"
//#define DEVICEID "100067ae95"       //for test mx
//#define APIKEY "5c92e17d-05e0-4e00-bd86-37570c670b46"
#define DEVICEID "100067ae9a"       //for customer mx
#define APIKEY "d40fd178-0dc9-449e-9334-a035b76cb1ec"

//#define DEVICEID "10005807ea"
//#define APIKEY "7e4041d8-67b4-4d95-b9f4-d6c18b3df5a2"

//#define DEVICEID "10005807eb"
//#define APIKEY "fc88f17f-d75f-4c93-a48c-9847ca9e1b66"

//#define DEVICEID "10005807ec"
//#define APIKEY "a1418d58-76e9-4c0c-8fc2-d1a024aacd09"

//#define DEVICEID "10005807ed"
//#define APIKEY "e3e1f18d-1eff-411d-954e-8b2b3e614b45"

//#define DEVICEID "10005807ee"
//#define APIKEY "113035d6-38aa-4a0a-910d-1ecf28263e95"

//#define DEVICEID "10005807ef"
//#define APIKEY "4d63eba7-a484-4227-8bcd-07879e55e0ec"

//#define DEVICEID "10005807f0"
//#define APIKEY "bed8363a-c717-400a-8cad-75bb7d04734f"

#define MODEL "PSF-BTA-GL"
#define VERDION "0.0.1"

#endif /* #ifndef __COLINK_DEFINE_H__ */
