/***********************************************************
*  File: tuya_gpio.h
*  Author: nzy
*  Date: 20171115
***********************************************************/
#ifndef __TUYA_GPIO_H
    #define __TUYA_GPIO_H

    #include "tuya_cloud_types.h"

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef  __TUYA_GPIO_GLOBALS
    #define __TUYA_GPIO_EXT
#else
    #define __TUYA_GPIO_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef enum {
    TY_GPIOA_0 = 0,
    TY_GPIOA_1,
    TY_GPIOA_2,
    TY_GPIOA_3,
    TY_GPIOA_4,
    TY_GPIOA_5,
    TY_GPIOA_6,
    TY_GPIOA_7,
    TY_GPIOA_8,
    TY_GPIOA_9,
    TY_GPIOA_10,
    TY_GPIOA_11,
    TY_GPIOA_12,
    TY_GPIOA_13,
    TY_GPIOA_14,
    TY_GPIOA_15,
    TY_GPIOA_16,
    TY_GPIOA_17,
    TY_GPIOA_18,
    TY_GPIOA_19,
    TY_GPIOA_20,
    TY_GPIOA_21,
    TY_GPIOA_22,
    TY_GPIOA_23,
    TY_GPIOA_24,
    TY_GPIOA_25,
    TY_GPIOA_26,
    TY_GPIOA_27,
    TY_GPIOA_28,
    TY_GPIOA_29,
    TY_GPIOA_30,
    TY_GPIOA_31,
    TY_GPIOB_0 = 32,
    TY_GPIOB_1,
    TY_GPIOB_2,
    TY_GPIOB_3,
    TY_GPIOB_4,
    TY_GPIOB_5,
    TY_GPIOB_6,
    TY_GPIOB_7,
    TY_GPIOB_8,
}TY_GPIO_PORT_E;


/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: tuya_gpio_inout_set
*  Input: port
*         in->TRUE:in
*             FALSE:out
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_GPIO_EXT \
OPERATE_RET tuya_gpio_inout_set(IN CONST TY_GPIO_PORT_E port,IN CONST BOOL_T in);

/***********************************************************
*  Function: tuya_gpio_inout_set
*  Input: port
*         in->TRUE:in
*             FALSE:out
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_GPIO_EXT \
INT_T tuya_gpio_read(IN CONST TY_GPIO_PORT_E port);

/***********************************************************
*  Function: tuya_gpio_write
*  Input: port
*         high->TRUE:high
*               FALSE:low
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_GPIO_EXT \
OPERATE_RET tuya_gpio_write(IN CONST TY_GPIO_PORT_E port,IN CONST BOOL_T high);

#ifdef __cplusplus
}
#endif
#endif

