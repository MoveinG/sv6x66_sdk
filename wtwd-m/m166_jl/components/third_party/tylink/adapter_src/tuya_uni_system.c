/***********************************************************
*  File: uni_system.c
*  Author: nzy
*  Date: 120427
***********************************************************/
#define _UNI_SYSTEM_GLOBAL
#include <stdio.h>
#include "tuya_uni_system.h"
#include "FreeRTOS.h"
#include "osal.h"
#include "wdt/drv_wdt.h"
#include "uni_log.h"
#include "wifi_api.h"
#include "sys/intc.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define SERIAL_NUM_LEN 32
#define UINT UINT_T

/***********************************************************
*************************variable define********************
***********************************************************/
STATIC CHAR_T serial_no[SERIAL_NUM_LEN+1] = {0};

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: GetSystemTickCount 
*  Input: none
*  Output: none
*  Return: system tick count
***********************************************************/
UINT_T tuya_GetSystemTickCount(VOID)
{
    return (UINT)OS_GetSysTick();
}

/***********************************************************
*  Function: GetTickRateMs 
*  Input: none
*  Output: none
*  Return: tick rate spend how many ms
***********************************************************/
UINT_T tuya_GetTickRateMs(VOID)
{
    return (UINT)portTICK_RATE_MS;
}

/***********************************************************
*  Function: SystemSleep 
*  Input: msTime
*  Output: none 
*  Return: none
*  Date: 120427
***********************************************************/
VOID tuya_SystemSleep(IN CONST TIME_MS msTime)
{
    OS_MsDelay(msTime);
}

/***********************************************************
*  Function: SystemIsrStatus->direct system interrupt status
*  Input: none
*  Output: none 
*  Return: BOOL_T
***********************************************************/
BOOL_T tuya_SystemIsrStatus(VOID)
{
    if(0 != intc_irq_status()) {
        return TRUE;
    }

    return FALSE;
}

/***********************************************************
*  Function: SystemReset 
*  Input: msTime
*  Output: none 
*  Return: none
*  Date: 120427
***********************************************************/
VOID tuya_SystemReset(VOID)
{
	drv_wdt_init();
	drv_wdt_enable(SYS_WDT, 100);
}

/***********************************************************
*  Function: SysMalloc 
*  Input: reqSize
*  Output: none 
*  Return: VOID *
***********************************************************/
VOID *tuya_SysMalloc(IN CONST DWORD_T reqSize)
{
    return OS_MemAlloc(reqSize);
}

/***********************************************************
*  Function: SysFree 
*  Input: msTime
*  Output: none 
*  Return: none
***********************************************************/
VOID tuya_SysFree(IN PVOID_T pReqMem)
{
    return OS_MemFree(pReqMem);
}

/***********************************************************
*  Function: SysGetHeapSize 
*  Input: none
*  Output: none 
*  Return: INT_T-> <0 means don't support to get heapsize
***********************************************************/
INT_T tuya_SysGetHeapSize(VOID)
{
    return (INT_T)OS_MemRemainSize();
}

/***********************************************************
*  Function: GetSerialNo 
*  Input: none
*  Output: none 
*  Return: CHAR_T *->serial number
***********************************************************/
CHAR_T *tuya_GetSerialNo(VOID)
{
	//获取mac地址
    static CHAR_T serno[14];
    uint8_t ssid[33], ssidlen, key[65], keylen, mac[6];

    serno[0] = 0;
    if(get_wifi_config(ssid, &ssidlen, key, &keylen, mac, 6) == 0)
    {
        sprintf((char*)serno, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    return serno;
}

/***********************************************************
*  Function: system_get_rst_info 
*  Input: none
*  Output: none 
*  Return: CHAR_T *->reset reason
***********************************************************/
CHAR_T *tuya_system_get_rst_info(VOID)
{
	//获取系统重启原因
	//返回值是字符串：eg：  ”rst reason is ：1“ 
	return "NOT_SUPPORTED";
}



