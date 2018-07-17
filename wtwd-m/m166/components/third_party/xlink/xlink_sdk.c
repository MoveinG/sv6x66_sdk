//
// Created by huangjinqiang on 2017/3/30.
//
#include<string.h>
#include<stdio.h>
#include<netdb.h>
#include<unistd.h>
//#include<arpa/inet.h>
#include <pthread.h>
//#include <sys/time.h>

#include "xlink_sdk.h"
#include "linux_flash.h"

int  XLINK_DEBUG_SDK_ON = 1;
int  XLINK_DEBUG_CLOUD_ON = 0;
int  XLINK_DEBUG_LOCAL_ON = 0;

struct xlink_datetime_t g_pTime;
unsigned char	IsAlreadyConnectToCloud = 0;
unsigned char	isConnectedServer = 0;
unsigned char	updata_number_of_crowd_flag = 0;
volatile unsigned short udp_dpmessage_id,tcp_dpmessage_id;

unsigned char g_recbuf[1024];

//#define FILE_NAME "sdk.flash"

#define XLINK_CERTIFY_ID	"591929aca26e6a07dec2edd4"//useless
#define XLINK_CERTIFY_KEY	"b7409d07-0612-440e-95dc-3e9cb9478ab2"//useless

#define XLINK_PID	"1607d2b673811f411607d2b673815001"
#define XLINK_PKEY	"1602bc73fb7c80a7396485409a036a4f"
//xlink_uint8 xlink_mac[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

volatile struct xlink_sdk_instance_t g_sdk_instance_t;
volatile struct xlink_sdk_instance_t *g_sdk_instance_p=NULL;

volatile static struct xlink_location_t g_location_t;
volatile static struct xlink_location_t *g_location_p;
unsigned char server_reject = 0;
int gsocket = -1;
int gudpsocket = -1;
unsigned char g_cloud_rec_buffer[2048];


static OsTaskHandle xlink_sdk_task=NULL;
static OsTaskHandle xlink_tcp_task=NULL;
//XLINK_FLASH_T flsh_t;

struct location_msg_wait_t{
    xlink_uint16 location_msgid;
    xlink_uint8 location_wait;
};

struct location_msg_wait_t location_msg_wait;
static int g_app_run = 1;

extern int get_Switch_status(void);
extern void Ctrl_Switch_cb(int open);

xlink_uint8 app_data_number_of_crowd;//数据端点数据

int xlink_tcp_send_data(char *data, int datalength);

int xlink_udp_send_data(char *data, int datalength, const xlink_addr_t **addr_t);

int ConnectTCPSever(char *domain, int port);

int CreatUDPSever();

//////////////////////////////////////////////////
void xlink_get_mac_addr(xlink_uint8 *mac_addr, int len)
{
	if(mac_addr && len >= 6) 
	{
    	void *cfg_handle = wifi_cfg_init();
	    wifi_cfg_get_addr1(cfg_handle, mac_addr);
	    wifi_cfg_deinit(cfg_handle);
	}
}

xlink_int32
xlink_write_flash_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength) {
    xlink_uint8 *_data_p = xlink_null;
    _data_p = (xlink_uint8 *) (*data);
    Xlink_Flash_Write(/*&flsh_t, 0, 0, */_data_p, datalength);
	xlink_debug_sdk("info.\r\n");
	print_array_debug(_data_p,datalength);
    return datalength;
}

xlink_int32
xlink_read_flash_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength) {
    xlink_uint8 *_data_p = xlink_null;
    _data_p = (xlink_uint8 *) (*buffer);
    Xlink_Flash_Read(/*&flsh_t, 0, 0, */_data_p, datamaxlength);
	xlink_debug_sdk("info.\r\n");
	print_array_debug(_data_p,datamaxlength);
    return datamaxlength;
}


XLINK_FUNTION xlink_int32 xlink_send_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength,
                          const xlink_addr_t **addr_t, xlink_uint8 flag) {
    xlink_uint8 *__data_p = xlink_null;
    xlink_int32 ret = -1;
    __data_p = (xlink_uint8 *) (*data);
    if (flag) {
        ret = xlink_tcp_send_data((char *) (__data_p), datalength);
    } else {
        __data_p = (xlink_uint8 *) (*data);
        ret = xlink_udp_send_data((char *) (__data_p), datalength, addr_t);
    }
    return ret;
}

xlink_int32
xlink_get_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength) {
    xlink_uint8 *buffer_p = xlink_null;	
  	int ii=0;
	unsigned char temp_xink_data_point[30];
  	xlink_debug_sdk("in\r\n");
	memset(temp_xink_data_point,0,sizeof(temp_xink_data_point));
	ii=0;

	temp_xink_data_point[ii++] = 0;
	temp_xink_data_point[ii++] = DP_BYTE;
	temp_xink_data_point[ii++] = 0x01;
	//temp_xink_data_point[ii++] = BREAK_UINT32(app_data_number_of_crowd, 3);
	//temp_xink_data_point[ii++] = BREAK_UINT32(app_data_number_of_crowd, 2);
	//temp_xink_data_point[ii++] = BREAK_UINT32(app_data_number_of_crowd, 1);
	temp_xink_data_point[ii++] = app_data_number_of_crowd;

    buffer_p = *buffer;
    memcpy(buffer_p, temp_xink_data_point, ii);
	if(XLINK_DEBUG_SDK_ON){
		print_array_debug(buffer_p, ii);
	}
    return ii;
}

xlink_int32
xlink_set_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength) {
    xlink_uint8 *buffer_p = xlink_null;
    xlink_debug_sdk("datalength=%d.\r\n", datalength);	
    if (datalength > 3) {
        buffer_p = (xlink_uint8 *) *data;
		if(XLINK_DEBUG_SDK_ON){
			print_array_debug(buffer_p,datalength);
		}
		if(buffer_p[0] == 0 && buffer_p[1] == DP_BYTE && buffer_p[2] == 0x01)
	    {
	    	//app_data_number_of_crowd = BUILD_UINT32(buffer_p[6], buffer_p[5], buffer_p[4], buffer_p[3]);
			app_data_number_of_crowd = buffer_p[3];
			Ctrl_Switch_cb(app_data_number_of_crowd);
			xlink_debug_sdk("app_data_number_of_crowd=%u.\r\n", app_data_number_of_crowd);
			OS_EnterCritical();
			updata_number_of_crowd_flag = 1;
			OS_ExitCritical();
		}        
    }
    return datalength;
}


XLINK_FUNTION void xlink_event_cb(struct xlink_sdk_instance_t **sdk_instance, const struct xlink_sdk_event_t **event_t) {
    switch ((*event_t)->enum_event_type_t) {
        case EVENT_TYPE_STATUS:
            xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_CONNECT_STATION,state=%d\r\n", (*event_t)->event_struct_t.status.status);
            if ( (*event_t)->event_struct_t.status.status == EVENT_DISCONNECTED_SERVER ) {
                //SDK通知用户与服务器断开
                close(gsocket);
                gsocket = -1;
                //调用断开服务器接口
                isConnectedServer = 0;
                xlink_sdk_disconnect_cloud((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
            } else if( (*event_t)->event_struct_t.status.status == EVENT_SERVER_REJECT_DEVICE_REQUEST ){
                //SDK通知用户与服务器断开
                close(gsocket);
                gsocket = -1;
                //调用断开服务器接口
                xlink_debug_sdk("tcp disconnect to cloud.\r\n");
				isConnectedServer = 0;
                xlink_sdk_disconnect_cloud((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
                server_reject = 1;
                //do not create socket until restart
            } else {
                //通知用户连接上服务器
                IsAlreadyConnectToCloud = 1;
				isConnectedServer = 1;
            }
            break;
        case EVENT_TYPE_REQ_DATETIME_CB:
			g_pTime.year = (*event_t)->event_struct_t.datetime_t.year;
        	g_pTime.month = (*event_t)->event_struct_t.datetime_t.month;
        	g_pTime.day = (*event_t)->event_struct_t.datetime_t.day;
        	g_pTime.hour = (*event_t)->event_struct_t.datetime_t.hour;
        	g_pTime.min = (*event_t)->event_struct_t.datetime_t.min;
        	g_pTime.second = (*event_t)->event_struct_t.datetime_t.second;
        	g_pTime.week = (*event_t)->event_struct_t.datetime_t.week;
        	g_pTime.zone = (*event_t)->event_struct_t.datetime_t.zone;
            xlink_debug_sdk(
                    "xlink_event_cb,tpye=EVENT_TYPE_REQ_DATETIME_CB,year=%d,month=%d,day=%d,week=%d,hour=%d,min=%d,second=%d,zone=%d\r\n",
                    (*event_t)->event_struct_t.datetime_t.year, (*event_t)->event_struct_t.datetime_t.month,
                    (*event_t)->event_struct_t.datetime_t.day,
                    (*event_t)->event_struct_t.datetime_t.week, (*event_t)->event_struct_t.datetime_t.hour,
                    (*event_t)->event_struct_t.datetime_t.min,
                    (*event_t)->event_struct_t.datetime_t.second, (*event_t)->event_struct_t.datetime_t.zone);
            break;
        case EVENT_TYPE_UPGRADE_CB:
            xlink_debug_sdk(
                    "xlink_event_cb,tpye=EVENT_TYPE_REQ_DATETIME_CB,filesize=%d,firware version=%d,url lengt=%d,url=%80s,hash length=%d,hash=%32s.\r\n",
                    (*event_t)->event_struct_t.upgrade_t.file_size,
                    (*event_t)->event_struct_t.upgrade_t.firmware_version,
                    (*event_t)->event_struct_t.upgrade_t.url_length, (*event_t)->event_struct_t.upgrade_t.url,
                    (*event_t)->event_struct_t.upgrade_t.hash_length, (*event_t)->event_struct_t.upgrade_t.hash);
            break;
        case EVENT_TYPE_NOTIFY:
            xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_NOTIFY.\r\n");
            break;
        case EVENT_TYPE_REQUEST_CB:
            xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_REQUEST_CB,messageid=%d,value=%d.\r\n",
                            (*event_t)->event_struct_t.request_cb_t.messageid,
                            (*event_t)->event_struct_t.request_cb_t.value);
            if( location_msg_wait.location_wait == 1 ) {
                xlink_debug_sdk("xlocation_msg_wait.location_msgid = %d.\r\n",location_msg_wait.location_msgid);
                if ((*event_t)->event_struct_t.request_cb_t.messageid == location_msg_wait.location_msgid) {
                    xlink_debug_sdk("location upload successfully\r\n");
                    location_msg_wait.location_wait = 0;
                }
            }
            break;
        case EVENT_TYPE_PRODUCTION_TEST:
            xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t=%d,\r\n",
                            (*event_t)->event_struct_t.pdct_cb_t.pdct_event_t);
            switch ((*event_t)->event_struct_t.pdct_cb_t.pdct_event_t){
                case EVENT_TYPE_ENTER_PDCT_TEST_SUCCESS:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t = EVENT_TYPE_ENTER_PDCT_TEST_SUCCESS,\r\n");
                    //getNowTime();
                    break;
                case EVENT_TYPE_ENTER_PDCT_TEST_FAIL:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t = EVENT_TYPE_ENTER_PDCT_TEST_FAIL,\r\n");
                    //getNowTime();
                    break;
                case EVENT_TYPE_PDCT_TEST_END_SUCCESS:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t = EVENT_TYPE_PDCT_TEST_END_SUCCESS,\r\n");
                    xlink_debug_sdk("tcp disconnect to cloud.\r\n");
                    close(gsocket);
                    gsocket = -1;
                    //调用断开服务器接口
                    xlink_sdk_disconnect_cloud((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
                    //server_reject = 1;
                    //do not create socket until restart
                    //getNowTime();
                    break;
                case EVENT_TYPE_PDCT_TEST_END_FAIL:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t = EVENT_TYPE_PDCT_TEST_END_FAIL,\r\n");
                    //getNowTime();
                    break;
                case EVENT_TYPE_PDCT_GET_RSSI:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t = EVENT_TYPE_PDCT_GET_RSSI,\r\n");
                    //struct xlink_addr_t * test;
                    //test = *(*event_t)->event_struct_t.pdct_cb_t.addr_t;
                    //xlink_debug_sdk("ip = %d, port = %d,socket = %d.index = %d\r\n",test->ip,test->port,test->socket,(*event_t)->event_struct_t.pdct_cb_t.index);
                    xlink_debug_sdk("index = %d\r\n",(*event_t)->event_struct_t.pdct_cb_t.index);
                    xlink_production_reply_rssi(sdk_instance,(*event_t)->event_struct_t.pdct_cb_t.index,
                                                (*event_t)->event_struct_t.pdct_cb_t.packetid,(*event_t)->event_struct_t.pdct_cb_t.messageid,0,-50,0);
                    break;
                case EVENT_TYPE_PDCT_GET_CUSTOM_TEST_DATA:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST,pdct_event_t = EVENT_TYPE_PDCT_GET_CUSTOM_TEST_DATA,\r\n");
                    xlink_production_reply_custom_test_data(sdk_instance,(*event_t)->event_struct_t.pdct_cb_t.index,
                                                            (*event_t)->event_struct_t.pdct_cb_t.packetid,(*event_t)->event_struct_t.pdct_cb_t.messageid,0,xlink_null,0);
                    break;
                default:
                    xlink_debug_sdk("xlink_event_cb,tpye=EVENT_TYPE_PRODUCTION_TEST, but pdct_event_t is not valid,\r\n");
            }
            break;
        default:
            break;
    }
}

XLINK_FUNTION xlink_uint32 xlink_get_ticktime_ms_cb(struct xlink_sdk_instance_t **sdk_instance) {
    //xlink_uint32 t = 0;
    //struct timeval tv;
    ///*iperf_*/gettimeofday(&tv, NULL);
    //t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return OS_GetSysTick();
}



int xlink_tcp_send_data(char *data, int datalength) {
	xlink_debug_sdk("TCP xlink_tcp_send_data.  datalength=%d.\r\n", datalength);
	if(XLINK_DEBUG_SDK_ON){
		print_array_debug(data,datalength);
	}
    return send(gsocket, data, datalength, 0);
}


void number_of_crowd_XlinkDataPointUpdata(xlink_uint8 AppData)
{
	int i=0,Len_Tmp=0;
	unsigned char app_xink_data_point[20];

	memset(app_xink_data_point,0,sizeof(app_xink_data_point));
	app_xink_data_point[i++] = 0;
	app_xink_data_point[i++] = DP_BYTE;
	app_xink_data_point[i++] = 0x01;
	//app_xink_data_point[i++] = BREAK_UINT32(AppData, 3);
	//app_xink_data_point[i++] = BREAK_UINT32(AppData, 2);
	//app_xink_data_point[i++] = BREAK_UINT32(AppData, 1);
	app_xink_data_point[i++] = AppData;

	Len_Tmp = i;
	if(Len_Tmp<=0)	{
		xlink_debug_sdk("No data input.\r\n");
		return;
	}
	xlink_debug_sdk("\r\nXlink send app data : ");
	if(XLINK_DEBUG_SDK_ON){
		print_array_debug(app_xink_data_point, Len_Tmp);
	}
	if(isConnectedServer){
		unsigned char* data_p = NULL;
		data_p = app_xink_data_point;
		xlink_update_datapoint((struct xlink_sdk_instance_t **)&g_sdk_instance_p, (xlink_uint16*)&tcp_dpmessage_id, (const xlink_uint8**)&data_p, Len_Tmp,1);
		tcp_dpmessage_id++;
	}	
}



int xlink_udp_send_data(char *data, int datalength, const xlink_addr_t **addr_t) {
    xlink_addr_t *__addr_p = xlink_null;
    struct sockaddr_in __cliaddr;
    int __clilen = 0;
    __addr_p = (xlink_addr_t *) *addr_t;
    memset(&__cliaddr, 0, sizeof(__cliaddr));
    __cliaddr.sin_family = AF_INET;
    __cliaddr.sin_port = (__addr_p->port);
    __cliaddr.sin_addr.s_addr = (__addr_p->ip);
    xlink_debug_sdk("UDP xlink_udp_send_data.  datalength=%d.\r\n", datalength);
    __clilen = sizeof(__cliaddr);
	if(XLINK_DEBUG_SDK_ON){
		print_array_debug(data,datalength);
	}
    return sendto(gudpsocket, data, datalength, 0, (struct sockaddr *) &__cliaddr, __clilen);
}


void xlink_sdk_tcp_thread(void *var) {
    xlink_uint8 ret = -1;

	if(gsocket >= 0) close(gsocket);
	gsocket = -1;
    while ((g_app_run) && ( server_reject == 0 )) {
        //没连接上服务器
        if ((gsocket < 0) && g_sdk_instance_t.cloud_enable) {
            //delay 5s
            OS_MsDelay(2000);
            close(gsocket);
            //连接tcp服务器
            gsocket = ConnectTCPSever("cm.iotapk.com", 1883);
            if (gsocket >= 0) {
                xlink_sdk_init((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
                xlink_sdk_connect_cloud((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
            }
            xlink_debug_sdk("ConnectTCPSever:socket=%d.\r\n", gsocket);
        }
        else{
            if(isConnectedServer) {
                OS_MsDelay(4);
#if 0
                g_location_t.latitude = (xlink_double)23.1021496364;
                g_location_t.longitude = (xlink_double)113.3832657592;
                //g_location_t.timestamp = 1502421709000;
                g_location_t.timestamp = time(xlink_null) * (xlink_uint64)1000;
                g_location_t.timestamp_flag = 1;
                g_location_t.address = "guangzhou,haizhu";
                g_location_t.address_length = strlen(g_location_t.address);
                //g_location_t.address_length = 0;
                g_location_p = &g_location_t;
                xlink_debug_sdk("upload location when the timestamp is %lld\n",g_location_t.timestamp);
                ret = xlink_upload_location_data((struct xlink_sdk_instance_t **) &g_sdk_instance_p,(struct xlink_location_t **) &g_location_p, &location_msg_wait.location_msgid);
                location_msg_wait.location_wait = 1;
                xlink_debug_sdk("xlink_upload_location return %d\n",ret);
#endif
#if 0
                xlink_production_test_start((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
                OS_MsDelay(3);
                xlink_uint8 pdct_test_data[2];
                pdct_test_data[0] = 0x00;
                pdct_test_data[1] = 0x00;
                xlink_uint8 *pdct_test_data_p = pdct_test_data;
                //xlink_production_test_end((struct xlink_sdk_instance_t **) &g_sdk_instance_p,&pdct_test_data_p,sizeof(pdct_test_data));
                xlink_production_test_end((struct xlink_sdk_instance_t **) &g_sdk_instance_p,xlink_null,0);
#endif
            }
        }//test location
        OS_MsDelay(10);
    }
    OS_TaskDelete(NULL);
}

void update_xlink_status(void)
{
	OS_EnterCritical();
	updata_number_of_crowd_flag = 1;
	OS_ExitCritical();
}

void xlinkProcessEnd(void)
{
	if(g_sdk_instance_p)
	{
		xlink_sdk_reset((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
		xlink_sdk_uninit((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
		g_sdk_instance_p = NULL;
		OS_MsDelay(5);
	}

	if(xlink_sdk_task) OS_TaskDelete(xlink_sdk_task);
	if(xlink_tcp_task) OS_TaskDelete(xlink_tcp_task);

	xlink_sdk_task = NULL;
	xlink_tcp_task = NULL;
}

void xlink_sdk_thread(void *var) {
    int ret = 0, i = 0;
    //unsigned char recbuf[1024];
    unsigned char *recbuf_p = xlink_null;
    fd_set fds, fds_udp;
    struct timeval tv;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    xlink_addr_t addr_t;
    xlink_addr_t *addr_p;

	if(gudpsocket >= 0) close(gudpsocket);
	gudpsocket = -1;
    xlink_debug_sdk("xlink_sdk_thread start.\r\n");

    //flash init
    //memset(&flsh_t, 0, sizeof(flsh_t));
    //Xlink_Flash_Init(&flsh_t, "/cygdrive/d/sdk.cfg", strlen("/cygdrive/d/sdk.cfg"));
    Xlink_Flash_Init(/*&flsh_t, FILE_NAME, strlen(FILE_NAME)*/);
    //init sd instance
    xlink_memset((void *) &g_sdk_instance_t, 0, sizeof(g_sdk_instance_t));
    g_sdk_instance_t.dev_pid = (unsigned char *) XLINK_PID;//It's effectual
    g_sdk_instance_t.dev_pkey = (unsigned char *) XLINK_PKEY;//It's effectual
    xlink_memcpy((void *) g_sdk_instance_t.dev_name, "xlink_dev", xlink_sizeof("xlink_dev"));
    g_sdk_instance_t.dev_mac_length = 6;
    //xlink_memcpy((void *) g_sdk_instance_t.dev_mac, xlink_mac, g_sdk_instance_t.dev_mac_length);
	xlink_get_mac_addr(g_sdk_instance_t.dev_mac, g_sdk_instance_t.dev_mac_length);
    g_sdk_instance_t.dev_firmware_version = 1;
    g_sdk_instance_t.cloud_enable = 1;
    g_sdk_instance_t.local_enable = 1;
    g_sdk_instance_t.certificate_id = (xlink_uint8*)XLINK_CERTIFY_ID;
    g_sdk_instance_t.certificate_id_length = xlink_strlen(XLINK_CERTIFY_ID);
    g_sdk_instance_t.certificate_key = (xlink_uint8*)XLINK_CERTIFY_KEY;
    g_sdk_instance_t.certificate_key_length = xlink_strlen(XLINK_CERTIFY_KEY);

    g_sdk_instance_t.cloud_rec_buffer = g_cloud_rec_buffer;
    g_sdk_instance_t.cloud_rec_buffer_length = 2048;
    g_sdk_instance_t.log_level = 0;
    g_sdk_instance_t.log_enable = 0;
    g_sdk_instance_t.dev_sn_length = 0;
    g_sdk_instance_p = &g_sdk_instance_t;
   
    //creat udp server
    gudpsocket = CreatUDPSever();
    if (gudpsocket < 0) {
        xlink_debug_sdk("Create udp server failed!\r\n");
    } else {
        xlink_debug_sdk("Create udp server successful!\r\n");
    }

    while (( g_app_run ) && ( server_reject == 0 )) {
		if(updata_number_of_crowd_flag == 1){
			OS_EnterCritical();
			updata_number_of_crowd_flag = 0;
			OS_ExitCritical();
			app_data_number_of_crowd = get_Switch_status();
			number_of_crowd_XlinkDataPointUpdata(app_data_number_of_crowd);
		}
        //连接上服务器tcp
        if (gsocket >= 0) {
            FD_ZERO(&fds);
            FD_SET(gsocket, &fds);
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
            ret = -1;
            if (select(gsocket + 1, &fds, NULL, NULL, &tv) > 0) {
                ret = (int) recv(gsocket, g_recbuf, 1024, 0);
            }
            if (ret > 0) {
                //接收到数据
                xlink_debug_sdk("recv tcp datalen=%d.\r\n", ret);
                recbuf_p = g_recbuf;
				if(XLINK_DEBUG_SDK_ON){
					print_array_debug(recbuf_p,ret);
				}
                //添加数据到SDK处理
                xlink_receive_data((struct xlink_sdk_instance_t **) &g_sdk_instance_p,
                                   (const xlink_uint8 **) &recbuf_p, ret, NULL, 1);
            } else if (ret == 0) {
                //断开tcp服务器
                xlink_debug_sdk("tcp disconnect to cloud.\r\n");
                close(gsocket);
                gsocket = -1;
                xlink_sdk_disconnect_cloud((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
            }
        }
        //udp server
        if (gudpsocket >= 0) {
            FD_ZERO(&fds_udp);
            FD_SET(gudpsocket, &fds_udp);
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
            ret = -1;
            if (select(gudpsocket + 1, &fds_udp, NULL, NULL, &tv) > 0) {
                //接收udp数据
                clilen = sizeof(struct sockaddr_in);
                ret = (int) recvfrom(gudpsocket, g_recbuf, 1024, 0, (struct sockaddr *) &cliaddr, &clilen);
                xlink_debug_sdk("recv udp datalen=%d.\r\n", ret);

                addr_t.ip = (cliaddr.sin_addr.s_addr);
                addr_t.port = (cliaddr.sin_port);
                addr_t.socket = gudpsocket;
                addr_p = &addr_t;
                xlink_debug_sdk("recv udp IP=%d.%d.%d.%d  port %d\r\n", 
		   		BREAK_UINT32(addr_t.ip, 0),BREAK_UINT32(addr_t.ip, 1),BREAK_UINT32(addr_t.ip, 2),
		   		BREAK_UINT32(addr_t.ip, 3),ntohs(addr_t.port));
                recbuf_p = g_recbuf;
				if(XLINK_DEBUG_SDK_ON){
					print_array_debug(recbuf_p,ret);
				}
                //处理数据
                xlink_receive_data((struct xlink_sdk_instance_t **) &g_sdk_instance_p,
                                   (const xlink_uint8 **) &recbuf_p, ret, (const xlink_addr_t **) &addr_p, 0);
            }
        }
        //xlink sdk process
        xlink_demo_loop();        
        OS_MsDelay(10);
    }
    OS_TaskDelete(NULL);
}

/**
  * @brief:main start
  * @param:none
  * @retval:int,always 0
  */

/*typedef union NODE  
{  
    int i;  
    char c;  
}Node;*/
  
int xlinkProcessStart(void) {
    int ret;
    //pthread_t workid, workid1;

	/*Node node;    
    node.i = 0x12345678;  
    if(node.c == 0x78){
    	printf("little ending device %x\n", node.c);
	}else{
    	printf("big ending device %x\n", node.c);
	}*/

    ret = OS_TaskCreate(xlink_sdk_thread, "xlink_sdk", 2048, NULL, 1, &xlink_sdk_task);
	if (ret == 0) {
        printf("Create xlink_sdk_thread error!");
        return -1;
    }
	ret = OS_TaskCreate(xlink_sdk_tcp_thread, "xlink_sdk_tcp", 2048, NULL, 1, &xlink_tcp_task);
    if (ret == 0) {
        printf("Create xlink_sdk_tcp_thread error!");
        return -1;
    }
    //while (g_app_run) {
    //    OS_MsDelay(1000);
    //}

    return 0;
}

int ConnectTCPSever(char *domain, int port) {
    struct hostent *host;
    int sockfd;
    struct sockaddr_in server_addr;
    struct timeval timeout = {0, 100000};

    if ((host = gethostbyname(domain)) == NULL) {
        xlink_debug_sdk("Get host name error.\r\n");
        return -1;
    }
	xlink_debug_sdk("Host name : %s\n", host->h_name); 
    xlink_debug_sdk("IP Address : %s\n",inet_ntoa(*((struct in_addr *)host->h_addr))); 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        xlink_debug_sdk("Socket creat failed.\r\n");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *) host->h_addr);

    xlink_debug_sdk("Socket start connect.\r\n");
    if (connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr)) == -1) {
        xlink_debug_sdk("Socket connect server failed.\r\n");
        return -1;
    } else {
        xlink_debug_sdk("Socket connect server successful.\r\n");
    }
    return sockfd;
}

int CreatUDPSever() {
    struct hostent *host;
    int sockfd;
    struct sockaddr_in server_addr;
    struct timeval timeout = {0, 100000};

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        xlink_debug_sdk("Socket creat failed.\r\n");
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(10000);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    xlink_debug_sdk("Start to creat UDP.\r\n");
    if (bind(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr)) == -1) {
        xlink_debug_sdk("Creat UDP failed.\r\n");
        return -1;
    } else {
        xlink_debug_sdk("Creat UDP successful.\r\n");
    }
    return sockfd;
}


int g_dayOfMonth[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

void DeviceSysTimeCount(unsigned int c_time)
{    
	int 			temp = 0;						//xlink_ticks_diff(c_time, m_old_getTime);
	static unsigned int m_old_getTime = 0;
	if (c_time < m_old_getTime) {
		m_old_getTime		= c_time;
	}
	temp				= c_time - m_old_getTime;
	if (temp > 0) {
		m_old_getTime		= c_time;
		g_pTime.second		+= temp;
		if (g_pTime.second >= 60) {
			g_pTime.second		-= 60;
			g_pTime.min++;
			if (g_pTime.min >= 60) {
				g_pTime.min 		= 0;
				g_pTime.hour++;
				if (g_pTime.hour >= 24) {
					g_pTime.hour		= 0;
					g_pTime.week++;
					if (g_pTime.week == 7) {
						g_pTime.week		= 0;
					}
					g_pTime.day++;
					if (g_pTime.day > g_dayOfMonth[g_pTime.month - 1]) {
						g_pTime.day 		= 1;
						g_pTime.month++;
						if (g_pTime.month > 12) {
							g_pTime.month		= 1;
							g_pTime.year++;
							if (g_pTime.year % 400 == 0 || (g_pTime.year % 100 != 0 && g_pTime.year % 4 == 0)) {
								g_dayOfMonth[1] 	= 29;
							}
							else {
								g_dayOfMonth[1] 	= 28;
							}
						}
					}
				}
			}
		}
	}
}

void xlink_demo_loop(void)
{
	xlink_uint32	time_count;
    static xlink_uint32 PreTime = 0;
	static xlink_uint32 CurTime = 0;
	struct xlink_sdk_event_t __event_t;
	struct xlink_sdk_event_t * __event_p;
	xlink_uint16	messid = 0;
	CurTime 			= xlink_get_ticktime_ms_cb((struct xlink_sdk_instance_t **)&g_sdk_instance_p)/ 1000;
	DeviceSysTimeCount(CurTime / 1000);
	xlink_sdk_process((struct xlink_sdk_instance_t **) &g_sdk_instance_p);
	time_count			= TICKS_DIFF(CurTime, PreTime);
	if ((time_count > 60) || (IsAlreadyConnectToCloud)) {
		PreTime 			= CurTime;
		IsAlreadyConnectToCloud = 0;
		if (isConnectedServer) {
			__event_t.enum_event_type_t = EVENT_TYPE_REQ_DATETIME;
			__event_p			= &__event_t;
			xlink_request_event((struct xlink_sdk_instance_t **)&g_sdk_instance_p, &messid, &__event_p);
		}
	}
}





