//
// Created by huangjinqiang on 2017/3/23.
//

#ifndef _XLINK_SDK_H_
#define _XLINK_SDK_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include<stdio.h>
#include<string.h>
#include "xlink_debug.h"


//define xlink datatype,modify by different system
#define xlink_int8   char
#define xlink_uint8  unsigned char
#define xlink_int16  short
#define xlink_uint16 unsigned short
#define xlink_int32  int
#define xlink_uint32 unsigned int
#define xlink_int64  long long
#define xlink_uint64 unsigned long long
#define xlink_double double
#define xlink_null   NULL
#define xlink_enum   enum
#define xlink_union  union
#define xlink_memcpy memcpy
#define xlink_memset memset
#define xlink_memcmp memcmp
#define xlink_sizeof sizeof
#define xlink_strlen strlen
#define xlink_sprintf sprintf
#define xlink_delay_ms

//define xlink sdk version
#define XLINK_SDK_VERSION 5353
#define XLINK_FUNTION

//define xlink_sdk
#define XLINK_DEV_NAME_MAX 16
#define XLINK_DEV_MAC_LENGTH_MIN 1
#define XLINK_DEV_MAC_LENGTH_MAX 32
#define XLINK_SYS_PARA_LENGTH (1024*3)
#define XLINK_PACKET_LENGTH_MAX 1000

//define EVENT_TYPE_CONNECT_STATUS
#define EVENT_DISCONNECTED_SERVER   0
#define EVENT_CONNECTED_SERVER   1
#define EVENT_SERVER_REJECT_DEVICE_REQUEST   2


typedef enum
{
	DP_BYTE  = 0x00,
	DP_INT16 = 0x10,
	DP_UINT16 = 0x20,
	DP_INT32 = 0x30,
	DP_UINT32 = 0x40,
	DP_INT64 = 0x50,
	DP_UINT64 = 0x60,
	DP_FLOAT = 0x70,
	DP_DOUBLE = 0x80,
	DP_STRING = 0x90,
	DP_BINARY = 0XA0
}DATAPOINTTYPE;

//xlink addr
typedef struct xlink_addr_t {
    xlink_int32 socket;
    xlink_uint32 ip;
    xlink_uint16 port;
} xlink_addr_t;


//xlink sdk configuration struct
typedef struct xlink_sdk_instance_t {
    //device product id
    xlink_uint8 *dev_pid;
    //device product key
    xlink_uint8 *dev_pkey;
    //device name
    xlink_uint8 dev_name[XLINK_DEV_NAME_MAX + 1];
    //device mac
    xlink_uint8 dev_mac[XLINK_DEV_MAC_LENGTH_MAX];
    //device mac length
    xlink_uint8 dev_mac_length;
    //device firmware version
    xlink_uint16 dev_firmware_version;
    //enable cloud
    xlink_uint8 cloud_enable;
    //enable local
    xlink_uint8 local_enable;
    //user certification
    xlink_uint8 *certificate_id;
    //user certification length
    xlink_int16 certificate_id_length;
    //user certification key
    xlink_uint8 *certificate_key;
    //user certification key
    xlink_int16 certificate_key_length;
    //log level
    xlink_uint8 log_level;
    //log enable
    xlink_uint8 log_enable;
    //cloud rec buffer
    xlink_uint8 *cloud_rec_buffer;
    //cloud rec buffer length;
    xlink_int32 cloud_rec_buffer_length;
    //system para,user don't care
    xlink_uint8 sdk_para[XLINK_SYS_PARA_LENGTH];
	//device sn length         new add
	xlink_uint16 dev_sn_length;
	//device sn
	xlink_uint8 *dev_sn;
} xlink_sdk_instance_t;

//xlink sdk event enum
typedef enum {
    EVENT_TYPE_STATUS = 0,
    EVENT_TYPE_REQ_DATETIME,
    EVENT_TYPE_REQ_DATETIME_CB,
    EVENT_TYPE_UPGRADE_CB,
    EVENT_TYPE_UPGRADE_COMPLETE,
    EVENT_TYPE_REQUEST_CB,
    EVENT_TYPE_NOTIFY,
	EVENT_TYPE_PRODUCTION_TEST
} xlink_enum_event_type_t;

//xlink device connect station struct
typedef struct xlink_status_t {
    xlink_uint8 status;
} xlink_status_t;

//xlink sdk datetime struct
typedef struct xlink_datetime_t {
    xlink_uint16 year;
    xlink_uint8 month;
    xlink_uint8 day;
    xlink_uint8 week;
    xlink_uint8 hour;
    xlink_uint8 min;
    xlink_uint8 second;
    xlink_int16 zone;
} xlink_datetime_t;

//xlink sdk upgrade struct
typedef struct xlink_upgrade_t {
    xlink_int32 device_id;
    xlink_uint8 flag;
    xlink_uint16 firmware_version;
    xlink_uint32 file_size;
    xlink_uint8 *url;
    xlink_int16 url_length;
    xlink_uint8 *hash;
    xlink_int16 hash_length;
} xlink_upgrade_t;

//xlink sdk upgrade complete struct
typedef struct xlink_upgrade_complete_t {
    xlink_uint8 flag;
    xlink_uint8 status;
    xlink_uint16 last_version;
    xlink_uint16 current_version;
} xlink_upgrade_complete_t;

//xlink sdk request call back struct
typedef struct xlink_request_cb_t {
    xlink_uint16 messageid;
    xlink_uint32 value;
} xlink_request_cb_t;

//xlink notify struct
typedef struct xlink_sdk_notify_t {
    xlink_uint8 from_type;//1:server,2:devcie,3:app
    xlink_int32 from_id;
    xlink_uint8 *message;
    xlink_int16 message_length;
} xlink_sdk_notify_t;

typedef enum{
    EVENT_TYPE_ENTER_PDCT_TEST_SUCCESS = 0,
    EVENT_TYPE_ENTER_PDCT_TEST_FAIL,
    EVENT_TYPE_PDCT_TEST_END_SUCCESS,
    EVENT_TYPE_PDCT_TEST_END_FAIL,
    EVENT_TYPE_PDCT_GET_RSSI,
    EVENT_TYPE_PDCT_GET_CUSTOM_TEST_DATA,
} xlink_pdct_event_type;



typedef struct xlink_pdct_cb_t {
    //struct xlink_addr_t **addr_t;
    xlink_int32 index;
    xlink_uint16 packetid;
    xlink_uint16 messageid;
    xlink_pdct_event_type pdct_event_t;
}xlink_pdct_cb_t;

//xlink sdk event struct
typedef struct xlink_sdk_event_t {
    xlink_enum_event_type_t enum_event_type_t;
    xlink_union {
        struct xlink_status_t status;
        struct xlink_datetime_t datetime_t;
        struct xlink_upgrade_t upgrade_t;
        struct xlink_upgrade_complete_t upgrade_complete_t;
        struct xlink_request_cb_t request_cb_t;
        struct xlink_sdk_notify_t notify_t;
        struct xlink_pdct_cb_t pdct_cb_t;
    } event_struct_t;
} xlink_sdk_event_t;

/*add by kaven*/
//xlink sdk location struct
typedef struct xlink_location_t {
    xlink_double longitude;
    xlink_double latitude;
    xlink_uint64 timestamp;
    xlink_uint8 timestamp_flag;//if timestamp is not use,timestamp_flag must be 0
    xlink_uint16 address_length;//if address is not use,address_length must be 0
    xlink_uint8 *address;
} xlink_location_t;

#define TICKS_DIFF(cur, prev) ((cur) >= (prev)) ? ((cur)-(prev)) : ((0xFFFFFFFF-(prev))+(cur)+1)
#define BREAK_UINT32( var, ByteNum ) \
          (unsigned char)((xlink_uint32)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((xlink_uint32)((xlink_uint32)((Byte0) & 0x00FF) \
          + ((xlink_uint32)((Byte1) & 0x00FF) << 8) \
          + ((xlink_uint32)((Byte2) & 0x00FF) << 16) \
          + ((xlink_uint32)((Byte3) & 0x00FF) << 24)))


//interface function
//sdk function
XLINK_FUNTION extern xlink_int32 xlink_sdk_init(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern xlink_int32 xlink_sdk_uninit(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern xlink_int32 xlink_sdk_connect_cloud(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern xlink_int32 xlink_sdk_disconnect_cloud(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern void xlink_sdk_process(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern xlink_int32 xlink_sdk_reset(struct xlink_sdk_instance_t **sdk_instance);
//sdk call back function
XLINK_FUNTION extern xlink_int32 xlink_write_flash_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNTION extern xlink_int32 xlink_read_flash_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
XLINK_FUNTION extern xlink_int32 xlink_send_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
XLINK_FUNTION extern xlink_int32 xlink_get_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
XLINK_FUNTION extern xlink_int32 xlink_set_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNTION extern void xlink_event_cb(struct xlink_sdk_instance_t **sdk_instance, const struct xlink_sdk_event_t **event_t);
XLINK_FUNTION extern xlink_uint32 xlink_get_ticktime_ms_cb(struct xlink_sdk_instance_t **sdk_instance);
//sdk send data function
XLINK_FUNTION extern xlink_int32 xlink_update_datapoint(struct xlink_sdk_instance_t **sdk_instance, xlink_uint16 *messageid, const xlink_uint8 **data, xlink_int32 datamaxlength, xlink_uint8 flag);
XLINK_FUNTION extern xlink_int32 xlink_request_event(struct xlink_sdk_instance_t **sdk_instance, xlink_uint16 *messageid, struct xlink_sdk_event_t **event_t);
XLINK_FUNTION extern xlink_int32 xlink_receive_data(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
XLINK_FUNTION extern xlink_int32 xlink_get_device_id(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern xlink_int32 xlink_get_device_key(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer);
XLINK_FUNTION extern xlink_int32 xlink_report_log(struct xlink_sdk_instance_t** sdk_instance, xlink_uint8 log_level, xlink_uint8** data, xlink_uint32 datalength);
/*add by kaven*/
XLINK_FUNTION extern xlink_int32 xlink_upload_location_data(struct xlink_sdk_instance_t **sdk_instance, struct xlink_location_t **xlink_location, xlink_uint16 *message_id);
//XLINK_FUNTION extern xlink_int32 xlink_production_mode_end(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNTION extern xlink_int32 xlink_production_test_end(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNTION extern xlink_int32 xlink_production_test_start(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNTION extern xlink_int32 xlink_production_reply_rssi(struct xlink_sdk_instance_t **sdk_instance,
                                                             xlink_int32 index,xlink_uint16 packetid,xlink_uint16 messageid,
                                                             xlink_uint16 result, xlink_int16 rssi, xlink_uint16 AP_STA);
XLINK_FUNTION extern xlink_int32 xlink_production_reply_custom_test_data(struct xlink_sdk_instance_t **sdk_instance,
                                                                  xlink_int32 index,xlink_uint16 packetid,xlink_uint16 messageid,
                                                                  xlink_uint16 result,xlink_uint8 **data, xlink_int32 length);
/*add by kaven*/

void xlink_demo_loop(void);
#ifdef  __cplusplus
}
#endif

#endif //_XLINK_SDK_H_
