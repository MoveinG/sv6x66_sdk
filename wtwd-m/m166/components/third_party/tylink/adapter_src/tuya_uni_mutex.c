/***********************************************************
*  File: uni_mutex.c
*  Author: 
*  Date: 
***********************************************************/
#define _UNI_MUTEX_GLOBAL
#include "tuya_uni_mutex.h"
//#include "FreeRTOS.h"
//#include "task.h"
//#include "semphr.h"
#include "osal.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef OsMutex THRD_MUTEX;

typedef struct
{
    THRD_MUTEX mutex;
}MUTEX_MANAGE,*P_MUTEX_MANAGE;

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/

/***********************************************************
*  Function: CreateMutexAndInit 创建一个互斥量并初始化
*  Input: none
*  Output: pMutexHandle->新建的互斥量句柄
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_CreateMutexAndInit(OUT MUTEX_HANDLE *pMutexHandle)
{
    if(!pMutexHandle)
        return OPRT_INVALID_PARM;
    //printf("%s, p=0x%x, *p=0x%x\n", __func__, pMutexHandle, *pMutexHandle);
    #if 0
    P_MUTEX_MANAGE pMutexManage;
    pMutexManage = (P_MUTEX_MANAGE)OS_MemAlloc(SIZEOF(MUTEX_MANAGE));
    if(!(pMutexManage))
        return OPRT_MALLOC_FAILED;
    
	if(OS_MutexInit(&pMutexManage->mutex) != OS_SUCCESS)
        return OPRT_INIT_MUTEX_FAILED;

    *pMutexHandle = (MUTEX_HANDLE)pMutexManage;
    #else
	if(OS_MutexInit(pMutexHandle) != OS_SUCCESS)
        return OPRT_INIT_MUTEX_FAILED;
    #endif
    //printf("%s, p=0x%x, *p=0x%x\n", __func__, pMutexHandle, *pMutexHandle);
    return OPRT_OK;
}

/***********************************************************
*  Function: MutexLock 加锁
*  Input: mutexHandle->互斥量句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_MutexLock(IN CONST MUTEX_HANDLE mutexHandle)
{
    if(!mutexHandle)
        return OPRT_INVALID_PARM;
    //printf("%s, m=0x%x\n", __func__, mutexHandle);
    #if 0
    P_MUTEX_MANAGE pMutexManage;
    pMutexManage = (P_MUTEX_MANAGE)mutexHandle;
    
    if(OS_MutexLock(pMutexManage->mutex) != OS_SUCCESS)
        return OPRT_MUTEX_LOCK_FAILED;
    #else
    if(OS_MutexLock(mutexHandle) != OS_SUCCESS)
        return OPRT_MUTEX_LOCK_FAILED;
    #endif 
    return OPRT_OK;
}

/***********************************************************
*  Function: MutexTryLock 互斥量尝试加锁
*  如该互斥量已加锁，则返回OPRT_MUTEX_LOCK_BUSY,否则加锁
*  Input: mutexHandle->互斥量句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_MutexTryLock(IN CONST MUTEX_HANDLE mutexHandle)
{
    return OPRT_COM_ERROR;
}

/***********************************************************
*  Function: MutexUnLock 解锁
*  Input: mutexHandle->互斥量句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_MutexUnLock(IN CONST MUTEX_HANDLE mutexHandle)
{
    if(!mutexHandle)
        return OPRT_INVALID_PARM;
    //printf("%s, m=0x%x\n", __func__, mutexHandle);
    #if 0
    P_MUTEX_MANAGE pMutexManage;
    pMutexManage = (P_MUTEX_MANAGE)mutexHandle;
    
	if(OS_MutexUnLock(pMutexManage->mutex) != OS_SUCCESS)
        return OPRT_MUTEX_UNLOCK_FAILED;
    #else
	if(OS_MutexUnLock(mutexHandle) != OS_SUCCESS)
        return OPRT_MUTEX_UNLOCK_FAILED;
    #endif 
    return OPRT_OK;
}

/***********************************************************
*  Function: ReleaseMutexManage 释放互斥锁管理结构资源
*  Input: mutexHandle->互斥量句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_ReleaseMutex(IN CONST MUTEX_HANDLE mutexHandle)
{
    if(!mutexHandle)
        return OPRT_INVALID_PARM;
    //printf("%s, m=0x%x\n", __func__, mutexHandle);
    #if 0
    P_MUTEX_MANAGE pMutexManage;
    pMutexManage = (P_MUTEX_MANAGE)mutexHandle;
    
    OS_MutexDelete(pMutexManage->mutex);

    OS_MemFree(mutexHandle);
    #else
    OS_MutexDelete(mutexHandle);
    #endif 
    return OPRT_OK;
}


