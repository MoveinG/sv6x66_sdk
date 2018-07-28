/***********************************************************
*  File: uni_semaphore.c
*  Author: nzy
*  Date: 120427
***********************************************************/
#define _UNI_SEMAPHORE_GLOBAL
#include "tuya_uni_semaphore.h"
#include "uni_log.h"
//#include "mem_pool.h"
//#include "FreeRTOS.h"
//#include "task.h"
#include "osal.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef struct
{
    OsSemaphore sem;
}SEM_MANAGE,*P_SEM_MANAGE;


/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: CreateSemaphore 创建一个信号量
*  Input: reqSize->申请的内存大小
*  Output: none
*  Return: NULL失败，非NULL成功
*  Date: 120427
***********************************************************/
STATIC SEM_HANDLE CreateSemaphore(VOID)
{
    P_SEM_MANAGE pSemManage;
    
    pSemManage = (P_SEM_MANAGE)OS_MemAlloc(SIZEOF(SEM_MANAGE));

    return (SEM_HANDLE)pSemManage;
}

/***********************************************************
*  Function: InitSemaphore
*  Input: semHandle->信号量操作句柄
*         semCnt
*         sem_max->no use for linux os
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
STATIC OPERATE_RET InitSemaphore(IN CONST SEM_HANDLE semHandle,IN CONST UINT_T semCnt,\
                                 IN CONST UINT_T sem_max)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;
        
    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    if(OS_SemInit(&pSemManage->sem, sem_max, semCnt) == OS_FAILED)
        return OPRT_INIT_SEM_FAILED;

    return OPRT_OK;
}

OPERATE_RET tuya_CreateAndInitSemaphore(SEM_HANDLE *pHandle, IN CONST UINT_T semCnt, \
                          IN CONST UINT_T sem_max)
{
    if(NULL == pHandle)
    {
        PR_ERR("invalid param");
        return OPRT_INVALID_PARM;
    }

    *pHandle = CreateSemaphore();
    if(*pHandle == NULL)
    {
        PR_ERR("malloc fails");
        return OPRT_MALLOC_FAILED;
    }

    OPERATE_RET ret = InitSemaphore(*pHandle, semCnt, sem_max);
    if(ret != OPRT_OK) {
        OS_MemFree(*pHandle);
        *pHandle = NULL;
        PR_ERR("semaphore init fails %d %d", semCnt, sem_max);
        return ret;
    }

    return OPRT_OK;
}

/***********************************************************
*  Function: WaitSemaphore
*  Input: semHandle->信号量操作句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_WaitSemaphore(IN CONST SEM_HANDLE semHandle)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;

    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    if(OS_SemWait(pSemManage->sem, portMAX_DELAY) != OS_SUCCESS)
        return OPRT_WAIT_SEM_FAILED;

    return OPRT_OK;
}

/***********************************************************
*  Function: PostSemaphore
*  Input: semHandle->信号量操作句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_PostSemaphore(IN CONST SEM_HANDLE semHandle)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;

    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

	if(OS_SemSignal(pSemManage->sem) == OS_FAILED)
        return OPRT_POST_SEM_FAILED;

    return OPRT_OK;
}

/***********************************************************
*  Function: ReleaseSemaphore
*  Input: semHandle->信号量操作句柄
*  Output: none
*  Return: OPERATE_RET
*  Date: 120427
***********************************************************/
OPERATE_RET tuya_ReleaseSemaphore(IN CONST SEM_HANDLE semHandle)
{
    if(!semHandle)
        return OPRT_INVALID_PARM;

    P_SEM_MANAGE pSemManage;
    pSemManage = (P_SEM_MANAGE)semHandle;

    OS_SemDelete(pSemManage->sem);
    OS_MemFree(semHandle); // 释放信号量管理结构

    return OPRT_OK;
}


