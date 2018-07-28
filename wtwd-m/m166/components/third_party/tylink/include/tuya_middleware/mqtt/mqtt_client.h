/***********************************************************
*  File: mqtt_client.h 
*  Author: nzy
*  Date: 20150526
***********************************************************/
#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H
#ifdef __cplusplus
	extern "C" {
#endif

#include "tuya_cloud_types.h"

#ifdef  __MQTT_CLIENT_GLOBALS
    #define __MQTT_CLIENT_EXT
#else
    #define __MQTT_CLIENT_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef PVOID_T MQ_HANDLE;
typedef VOID (*MQ_DATA_RECV_CB)(IN BYTE_T *data,IN UINT_T len);
typedef VOID (*MQ_CONNED_CB)(VOID);
typedef VOID (*MQ_DISCONN_CB)(VOID);
typedef VOID (*MQ_CONN_DENY_CB)(IN BYTE_T deny_times);
typedef BOOL_T (*MQ_PERMIT_CONN_CB)(VOID); // if (MQ_PERMIT_CONN_CB == NULL || return TRUE)
                                         // then permit mqtt connect

typedef struct {
    CHAR_T *subcribe_topic;
    CHAR_T *client_id;
    CHAR_T *user_name;
    CHAR_T *passwd;
}MQ_CLIENT_IF_S;

// if(op_ret == OPRT_OK) then mqtt_publish_async success else fail.
typedef VOID (*MQ_PUB_ASYNC_IFM_CB)(IN CONST OPERATE_RET op_ret,IN CONST VOID *prv_data);

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: create_mqtt_hand
*  Input: domain_tbl
*         domain_num
*         serv_port
*         mqc_if->mqtt client info 
*         s_alive->if(0 == s_alive) then set DEF_MQTT_ALIVE_TIME_S
*  Output: hand
*  Return: OPERATE_RET
***********************************************************/
__MQTT_CLIENT_EXT \
OPERATE_RET create_mqtt_hand(IN CONST CHAR_T **domain_tbl,\
                             IN CONST BYTE_T domain_num,\
                             IN CONST USHORT_T serv_port,\
                             IN CONST MQ_CLIENT_IF_S *mqc_if,\
                             IN CONST USHORT_T s_alive,\
                             IN CONST MQ_DATA_RECV_CB recv_cb,\
                             OUT MQ_HANDLE *hand);

/***********************************************************
*  Function: mqtt_register_cb
*  Input: hand
*         conn_cb 
*         dis_conn_cb
*         conn_deny_cb
*         permit_conn_cb
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQTT_CLIENT_EXT \
OPERATE_RET mqtt_register_cb(IN CONST MQ_HANDLE hand,\
                             IN CONST MQ_CONNED_CB conn_cb,\
                             IN CONST MQ_DISCONN_CB dis_conn_cb,\
                             IN CONST MQ_CONN_DENY_CB conn_deny_cb,\
                             IN CONST MQ_PERMIT_CONN_CB permit_conn_cb);

/***********************************************************
*  Function: mqtt_client_start
*  Input: hand
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQTT_CLIENT_EXT \
OPERATE_RET mqtt_client_start(IN CONST MQ_HANDLE hand);

/***********************************************************
*  Function: get_mqtt_conn_stat
*  Input: hand
*  Output: none
*  Return: BOOL_T
***********************************************************/
__MQTT_CLIENT_EXT \
BOOL_T get_mqtt_conn_stat(IN CONST MQ_HANDLE hand);

/***********************************************************
*  Function: mq_disconnect
*  Input: hand
*  Output: none
*  Return: none
***********************************************************/
__MQTT_CLIENT_EXT \
VOID mq_disconnect(IN CONST MQ_HANDLE hand);

/***********************************************************
*  Function: release_mqtt_hand
*  Input: hand
*  Output: none
*  Return: none
***********************************************************/
__MQTT_CLIENT_EXT \
VOID release_mqtt_hand(IN CONST MQ_HANDLE hand);

/***********************************************************
*  Function: mqtt_publish_async
*  Input: hand 
*         topic
*         qos if(0 == qos) then to_lmt cb prv_data useless,
*                 becase no respond wait. 
*             else if(1 == qos) then need wait respond.
*             else then do't support.
*         data
*         len
*         to_lmt timeout limit if(0 == to_lmt) then use system default limit
*         cb
*         prv_data
*  Output: hand
*  Return: OPERATE_RET
***********************************************************/
__MQTT_CLIENT_EXT \
OPERATE_RET mqtt_publish_async(IN CONST MQ_HANDLE hand,IN CONST CHAR_T *topic,IN CONST BYTE_T qos,\
                               IN CONST BYTE_T *data,IN CONST INT_T len,IN CONST UINT_T to_lmt,\
                               IN CONST MQ_PUB_ASYNC_IFM_CB cb,IN VOID *prv_data);

#ifdef __cplusplus
}
#endif
#endif

