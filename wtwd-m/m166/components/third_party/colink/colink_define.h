#ifndef __COLINK_DEFINE_H__
#define __COLINK_DEFINE_H__

#define os_printf	printf
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


#define DEVICEID "100037ed0e"
#define APIKEY "8333ef42-4031-464e-8dd5-972874746b29"
#define MODEL "PSF-BU1-GL"
#define VERDION "1.0.0"

#endif /* #ifndef __COLINK_DEFINE_H__ */