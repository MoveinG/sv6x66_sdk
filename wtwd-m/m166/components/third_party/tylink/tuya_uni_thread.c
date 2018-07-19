/***********************************************************
*  File: uni_thread.c
*  Author: nzy
*  Date: 120427
***********************************************************/
#define _UNI_THREAD_GLOBAL
#include "tuya_uni_thread.h"
//#include "uni_mutex.h"
//#include "uni_pointer.h"
//#include "uni_log.h"
//#include "FreeRTOS.h"
//#include "task.h"
#include "osal.h"
//#include <string.h>

/***********************************************************
*************************micro define***********************
***********************************************************/
// thread id
typedef OsTaskHandle THREAD;
#if 0
// 定义线程管理结构
typedef struct
{
    THREAD thrdID; // 线程ID
    INT_T thrdRunSta; // 线程执行状态
    P_THRD_FUNC pThrdFunc; // 线程执行函数
    PVOID_T pThrdFuncArg; // 线程执行函数参数句柄

    P_CONSTRUCT_FUNC enter;    // 构造
    P_EXTRACT_FUNC exit;        // 析构

    LIST_HEAD node;
}THRD_MANAGE,*P_THRD_MANAGE;

typedef struct {
    LIST_HEAD list;
    MUTEX_HANDLE mutex;
}DEL_THRD_MAG_S;

typedef PVOID_T THRD_RET_T;
#define THRD_RETURN return
/***********************************************************
*************************variable define********************
***********************************************************/
STATIC DEL_THRD_MAG_S *s_del_thrd_mag = NULL;

/***********************************************************
*************************function define********************
***********************************************************/
STATIC void __WrapRunFunc(IN VOID *pArg);
STATIC VOID __inner_del_thread(IN CONST THREAD thrdID);

STATIC OPERATE_RET __cr_and_init_del_thrd_mag(VOID)
{
    if(NULL != s_del_thrd_mag) {
        return OPRT_OK;
    }

    DEL_THRD_MAG_S *tmp_del_thrd_mag = Malloc(SIZEOF(DEL_THRD_MAG_S));
    if(NULL == tmp_del_thrd_mag)
    {
        PR_ERR("Malloc Del Mgr Fails");
        return OPRT_MALLOC_FAILED;
    }
    memset(tmp_del_thrd_mag,0,SIZEOF(DEL_THRD_MAG_S));
    INIT_LIST_HEAD(&(tmp_del_thrd_mag->list));

    OPERATE_RET op_ret = OPRT_OK;
    op_ret = CreateMutexAndInit(&(tmp_del_thrd_mag->mutex));
    if(OPRT_OK != op_ret)
    {
        PR_ERR("Create Mutex Fails");
        Free(tmp_del_thrd_mag);
        return op_ret;
    }
    
    s_del_thrd_mag = tmp_del_thrd_mag;

    return OPRT_OK;
}

STATIC VOID __add_del_thrd_node(THRD_MANAGE *thrd)
{
    if(NULL == thrd || NULL == s_del_thrd_mag)
    {
        return;
    }

    PR_DEBUG("add thead to delete list");

    MutexLock(s_del_thrd_mag->mutex);
    thrd->thrdRunSta = STATE_STOP;
    list_add_tail(&(thrd->node),&(s_del_thrd_mag->list));
    MutexUnLock(s_del_thrd_mag->mutex);
}

// need to port in different os
STATIC VOID __inner_del_thread(IN CONST THREAD thrdID)
{
    PR_DEBUG("real delete thread:0x%x",thrdID);

    // delete thread process 
    vTaskDelete(thrdID);
}

STATIC VOID __free_all_del_thrd_node(VOID)
{
    if(NULL == s_del_thrd_mag)
    {
        return;
    }

    THRD_MANAGE *tmp_node = NULL;

    MutexLock(s_del_thrd_mag->mutex);
    while(list_empty( &(s_del_thrd_mag->list) ) != TRUE)
    {
        PR_DEBUG("del list not empty...deleting");
        tmp_node = list_entry(s_del_thrd_mag->list.next, THRD_MANAGE, node);
        THREAD thrdID = tmp_node->thrdID;
        DeleteNode(tmp_node,node);
		Free(tmp_node);
        MutexUnLock(s_del_thrd_mag->mutex);
        __inner_del_thread(thrdID);
        MutexLock(s_del_thrd_mag->mutex);
    }
    MutexUnLock(s_del_thrd_mag->mutex);
}
#endif

/***********************************************************
*  Function: CreateAndStart 创建并正常运行，在其他线程中可进行JOIN处理
*  Input: enter->construct
*         exit->extract
*         pThrdFunc->线程执行函数
*         pThrdFuncArg->线程执行函数指针
*         thrd_param-> yes/none
*  Output: pThrdHandle->生成的线程句柄
*  Return: none
***********************************************************/
OPERATE_RET tuya_CreateAndStart(OUT THRD_HANDLE *pThrdHandle,\
                           IN CONST P_CONSTRUCT_FUNC enter,\
                           IN CONST P_EXTRACT_FUNC exit,\
                           IN CONST P_THRD_FUNC pThrdFunc,\
                           IN CONST PVOID_T pThrdFuncArg,\
                           IN CONST THRD_PARAM_S *thrd_param)
{
    if(!pThrdHandle || !pThrdFunc)
    {
        PR_ERR("Invalid Param");
        return OPRT_INVALID_PARM;
    }

    BaseType_t ret;
    ret = OS_TaskCreate(pThrdFunc, thrd_param->thrdname, thrd_param->stackDepth, pThrdFuncArg, thrd_param->priority, pThrdHandle);
	if (ret == 0) {
        PR_ERR("xTaskCreate %d", ret);
        *pThrdHandle = NULL;
        return OPRT_THRD_CR_FAILED;
    }

    return OPRT_OK;
}

#if 0
STATIC void __WrapRunFunc(IN PVOID_T pArg)
{
    __free_all_del_thrd_node();

    // THRD_RET_T pRet;
    P_THRD_MANAGE pThrdManage = (P_THRD_MANAGE)pArg;

    if(pThrdManage->enter)
    {
        pThrdManage->enter();
    }

    pThrdManage->thrdRunSta = STATE_RUNNING;
    pThrdManage->pThrdFunc(pThrdManage->pThrdFuncArg);

    // must call <DeleteThrdHandle> to delete thread
    THRD_STA status = STATE_EMPTY;
    do {
        MutexLock(s_del_thrd_mag->mutex);
        status = pThrdManage->thrdRunSta;
        MutexUnLock(s_del_thrd_mag->mutex);

        SystemSleep(200);
    }while(status != STATE_STOP);

    if(pThrdManage->exit)
    {
        pThrdManage->exit();
    }

    __free_all_del_thrd_node();

    THRD_RETURN;
}
#endif

/***********************************************************
*  Function: GetThrdSta
*  Input: thrdHandle->线程句柄
*  Output: none
*  Return: THRD_STA
***********************************************************/
THRD_STA tuya_GetThrdSta(IN CONST THRD_HANDLE thrdHandle)
{
    if(NULL == thrdHandle)
    {
        PR_ERR("Invalid Param");
        return -1;
    }

    return STATE_RUNNING;
}

/***********************************************************
*  Function: DeleteThrdHandle
*  Input: thrdHandle->线程句柄  
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET tuya_DeleteThrdHandle(IN CONST THRD_HANDLE thrdHandle)
{
    if(NULL == thrdHandle) {
        PR_ERR("Invalid Param");
        return OPRT_INVALID_PARM;
    }

	OS_TaskDelete(thrdHandle);
    return OPRT_OK;
}

#if 0
/***********************************************************
*  Function: __GetThrdId
*  Input: thrdHandle->线程句柄 
*  Output: none
*  Return: -1->失败 其他->线程ID
***********************************************************/
STATIC THREAD __GetThrdId(IN CONST THRD_HANDLE thrdHandle)
{
    P_THRD_MANAGE pThrdManage;
    pThrdManage = (P_THRD_MANAGE)thrdHandle;

    return pThrdManage->thrdID;
}

/***********************************************************
*  Function: __GetSelfId
*  Input: thrdHandle->线程句柄 
*  Output: none
*  Return: -1->失败 其他->线程ID
***********************************************************/
STATIC THREAD __GetSelfId(VOID)
{
    xTaskHandle task_handle = xTaskGetCurrentTaskHandle();
    if(NULL == task_handle) {
        return (THREAD)-1;
    }

    return task_handle;
}
#endif

/***********************************************************
*  Function: ThreadRunSelfSpace
*  Input: thrdHandle->线程句柄 
*  Output: bl
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET tuya_ThreadRunSelfSpace(IN CONST THRD_HANDLE thrdHandle,OUT BOOL_T *bl)
{
    if(NULL == thrdHandle || NULL == bl) {
        return OPRT_INVALID_PARM;
    }

	OsTaskHandle cur_handle = OS_TaskGetCurrHandle();
    if(cur_handle == thrdHandle) {
        *bl = TRUE;
    }else {
        *bl = FALSE;
    }

    return OPRT_OK;
}


