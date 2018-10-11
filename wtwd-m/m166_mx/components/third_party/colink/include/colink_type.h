#ifndef __COLINK_TYPE_H__
#define __COLINK_TYPE_H__


//#include "esp_common.h"
#include "colink_link.h"
#include "softmac/softap_func.h"

#define softap_config	SOFTAP_CUSTOM_CONFIG
//#define ip_info

typedef struct
{
	uint8_t ssid[33];
	uint8_t password[65];
	//uint8_t bssid_set;
	//uint8_t bssid[7];
} station_config;

typedef struct
{
    station_config sta_config;
    //struct ip_info sap_ip_info;
    softap_config sap_config;

    uint32_t __pad;
} CoLinkFlashParam;

typedef enum 
{
    DEVICE_MODE_START          = 0,/**< 开始启动 */
    DEVICE_MODE_SETTING        = 1,/**< 进入ESP TOUCH配网模式 */
    DEVICE_MODE_SETTING_SELFAP = 2,/**< 进入AP配网模式 */
    DEVICE_MODE_WORK_NORMAL    = 3,/**< 进入正常工作模式 */
    DEVICE_MODE_UPGRADE        = 4,/**< 升级 */

    DEVICE_MODE_INVALID        = 255,/**< 初始无效的设备模式 */
}CoLinkDeviceMode;

//ColinkLinkInfo colinkInfoSetting;






















#endif
