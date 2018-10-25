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


#define DEVICEID "10005807e9"
#define APIKEY "cf761fd6-5195-4b32-92f5-e1c166c5bcd7"
#define MODEL "PSF-BTA-GL"
#define VERDION "0.0.1"

#endif /* #ifndef __COLINK_DEFINE_H__ */