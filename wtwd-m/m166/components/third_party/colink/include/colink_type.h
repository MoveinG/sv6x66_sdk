#ifndef __COLINK_TYPE_H__
#define __COLINK_TYPE_H__


//#include "esp_common.h"
#include "colink_link.h"

/*
typedef struct
{
    struct station_config sta_config;
    struct ip_info sap_ip_info;
    struct softap_config sap_config;

    uint32 __pad;
} CoLinkFlashParam;
*/

typedef enum 
{
    DEVICE_MODE_START          = 0,/**< 开始启动 */
    DEVICE_MODE_SETTING        = 1,/**< 进入ESP TOUCH配网模式 */
    DEVICE_MODE_SETTING_SELFAP = 2,/**< 进入AP配网模式 */
    DEVICE_MODE_WORK_NORMAL    = 3,/**< 进入正常工作模式 */
    DEVICE_MODE_UPGRADE        = 4,/**< 升级 */

    DEVICE_MODE_INVALID        = 255,/**< 初始无效的设备模式 */
}CoLinkDeviceMode;

ColinkLinkInfo colinkInfoSetting;






















#endif
