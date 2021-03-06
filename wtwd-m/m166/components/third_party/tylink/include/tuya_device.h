/***********************************************************
*  File: tuya_device.h
*  Author: nzy
*  Date: 20170909
***********************************************************/
#ifndef _TUYA_DEVICE_H
#define _TUYA_DEVICE_H

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _TUYA_DEVICE_GLOBAL
    #define _TUYA_DEVICE_EXT 
#else
    #define _TUYA_DEVICE_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
#define PRODUCT_KEY "AmTkt36gXlhjSxCH" //"DCfFKwjz8rxAe3fF"
#define DEV_SW_VERSION USER_SW_VER

#define APP_BIN_NAME	"mac_atcmd"
#define USER_SW_VER		"0.0.3"
/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: pre_device_init
*  Input: none
*  Output: none
*  Return: none
*  Note: to initialize device before device_init
***********************************************************/
_TUYA_DEVICE_EXT \
VOID pre_device_init(VOID);

/***********************************************************
*  Function: device_init 
*  Input: none
*  Output: none
*  Return: none
***********************************************************/
_TUYA_DEVICE_EXT \
OPERATE_RET device_init(VOID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif



