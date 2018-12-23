/****************************include*********************/
#include <string.h>
#include "soc_types.h"
#include "atcmd.h"
#include "error.h"
#include "Cabrio-conf.h"
#include "netstack_def.h"
#include "ssv_lib.h"
#include "wifi_api.h"
#include "drv_wdt.h"
#include "user_transport.h"
#include "user_socket.h"
#include "../cfg/mac_cfg.h"


/****************************define*********************/
#define USER_SERVER_IP                 "120.77.49.36"
#define USER_SERVER_PORT               "8282"
#define USER_SOCKET_PORT               8000
#define BUFFER_SIZE                    256
#define SOCKET_RECV_TIMEOUT            300


/****************************typedef*********************/
enum {
	ERROR_MEMORY_FAILED                = -11,
	ERROR_GPIO_CONNECTION              = -10,
	ERROR_UDP_CONNECTION               = -9,
	ERROR_TCP_CONNECTION               = -8,
	ERROR_WIFI_CONNECTION_DEAUTH       = -7,
	ERROR_WIFI_CONNECTION_ASSOCIATION  = -6,
	ERROR_WIFI_CONNECTION_AUTH         = -5,
	ERROR_WIFI_CONNECTION              = -4,
	ERROR_UNKNOWN_COMMAND              = -3,
	ERROR_NOT_IMPLEMENT                = -2,
	ERROR_INVALID_PARAMETER            = -1,
	ERROR_SUCCESS                      =  0,
	ERROR_SUCCESS_NO_RSP               =  1,
};

typedef enum {
	SOCKET_SEND_LOGIN                  = 1,
	SOCKET_SEND_WIFICONFIG_ACK         = 2,
	SOCKET_SEND_UART_CMD               = 3,
	SOCKET_SENT_CLIENT                 = 4,
}e_sockeSendType_t;





/***********************local variable ********************/
int connetServerSocketId = 0;



/***********************local function ********************/
void get_device_mac(unsigned char *mac)
{
	char mac_addr[6] = {0};
    void *cfg_handle = wifi_cfg_init();
    
    wifi_cfg_get_addr1(cfg_handle, mac_addr);
    wifi_cfg_deinit(cfg_handle);
	memcpy(mac, mac_addr, 6);
	OS_MsDelay(10);
}


int user_socket_send(int socketId, char* buffer , char type)
{
	int ret = 0;
	unsigned char addrBuf[6]             = {0};
	unsigned char sBuf[BUFFER_SIZE_MAX]  = {0};
	unsigned char dBuf[BUFFER_SIZE_MAX]  = {0};

	memset(sBuf,0,BUFFER_SIZE_MAX);
	memset(dBuf,0,BUFFER_SIZE_MAX);

	OS_MsDelay(100);
	//printf("[%s]:[%d]\r\n",__func__,__LINE__);
	
	switch(type)
	{
		case SOCKET_SEND_LOGIN: 
			get_device_mac(addrBuf);
			sprintf(sBuf,"~17%.2x:%.2x:%.2x:%.2x:%.2x:%.2x?",\
				addrBuf[0],addrBuf[1],addrBuf[2],addrBuf[3],addrBuf[4],addrBuf[5]);
			user_aes_encrypt(sBuf,dBuf);
			memset(sBuf,0,BUFFER_SIZE_MAX);
			sprintf(sBuf,"{%s}",dBuf);
			printf("[%s]login buffer:%s\r\n",__func__,sBuf);
		break;
		
		case SOCKET_SEND_WIFICONFIG_ACK: 
			get_device_mac(addrBuf);
			sprintf(sBuf,"{\"ack\":\"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\"}",\
				addrBuf[0],(addrBuf[1]),(addrBuf[2]),(addrBuf[3]),(addrBuf[4]),(addrBuf[5]));
		break;
		
		case SOCKET_SEND_UART_CMD: 
			//printf("sendCmdBuffer:%s\r\n",buffer);
			user_aes_encrypt(buffer,dBuf);
			sprintf(sBuf,"{%s}",dBuf);
			memset(deviceStatus.uartCmdBuffer,0,BUFFER_SIZE_MAX);
			printf("[%s]uart buffer:%s,len=%d\r\n",__func__,sBuf,strlen(sBuf));
		break;

		case SOCKET_SENT_CLIENT:
			printf("1111 buffer:%s\r\n",buffer);
			memcpy(sBuf,buffer,strlen(buffer));
		break;
		default:
		break;
	}

	OS_MsDelay(100);
	ret = send(socketId, sBuf, strlen(sBuf), 0);
	
	return ret;
}


void user_socket_recv_func(char *buffer)
{
	char *pHead   = NULL;
	char *pCurPos = NULL;
	char pBuffer[BUFFER_SIZE_MAX] = {0};
	char sBuffer[BUFFER_SIZE_MAX] = {0};

	int length    = 0;
	
	if(*buffer != NULL)
	{
		if ((*buffer == '{') && (*(buffer+1) != '}'))
		{
			pHead = buffer;
			pCurPos = strchr(pHead,'}');
			length = pCurPos - pHead - 1;
			memcpy(pBuffer,(buffer+1),length);
			printf("[%s]buffer:%s,len=%d\r\n",__func__,pBuffer,length);
			user_aes_decrypt(pBuffer,sBuffer);
			//printf("send to uart buffer=%s,len=%d\r\n",sBuffer,strlen(sBuffer));
			OS_MsDelay(100);
			memcpy(deviceStatus.socketClientRevBuffer,sBuffer,strlen(sBuffer));
			set_rev_server_data_flag(true);
		}
		memset(buffer,0,BUFFER_SIZE_MAX);
	}
	
	return ;
}


int user_socket_client_init(void)
{
	int socketId = 0;
	int timeOut  = 0;
	int ret      = 0;
	struct sockaddr_in s_userSocket;
	
	memset(&s_userSocket, 0, sizeof(s_userSocket));
    s_userSocket.sin_family = AF_INET;
    s_userSocket.sin_port = htons(atoi (USER_SERVER_PORT));
    inet_aton(USER_SERVER_IP, &s_userSocket.sin_addr);
    s_userSocket.sin_len = sizeof(s_userSocket);

	socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId < 0)
    {
        printf("Failed to create socket\n");
		return -1;
    }

	ret = connect(socketId, (struct sockaddr *) &s_userSocket, sizeof(s_userSocket));
    if (ret < 0)
    {
        printf("[%d]Failed to connect ret=%d\n",__LINE__,ret);
        close(socketId);      
		return -1;
    }

    timeOut = SOCKET_RECV_TIMEOUT;
    setsockopt(socketId, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut));

	return socketId;
}


void user_tcp_client_task(void *arg)
{
	printf("[%d]:[%s]\r\n",__LINE__,__func__);
		
	char *pHead   = NULL;
	char *pCurPos = NULL;
	
    int socketId = 0;
    
	
	unsigned char buffer[BUFFER_SIZE_MAX]    = {0};
	unsigned char aesBuffer[BUFFER_SIZE_MAX] = {0};

	
    socketId = user_socket_client_init();
	if (socketId < 0)
	{
		goto exit;
	}
	
	if (user_socket_send(socketId,NULL,SOCKET_SEND_LOGIN) < 0)
	{
		close(socketId);
		goto exit;
	}

	set_connect_server_status(true);
	deviceStatus.socketClientCreateFlag = true;
	
	while(1)
	{
	#if 1
		if (deviceStatus.uartCmdFlag == true)
		{
			deviceStatus.uartCmdFlag = false;
			if (user_socket_send(socketId,deviceStatus.uartCmdBuffer,SOCKET_SEND_UART_CMD) < 0)
			{
				close(socketId);
				goto exit;
			}
		}
		else
		{
			if (recv(socketId, buffer, BUFFER_SIZE_MAX,0) > 0)
			{
				user_socket_recv_func(buffer);
				memset(buffer,0,BUFFER_SIZE_MAX);
			}
		}
		
		if (get_device_mode() == DUT_AP)
		{
			 close(socketId);
			 goto exit;
		}
	#endif
        vTaskDelay(50 / portTICK_RATE_MS);
	}

exit:
	deviceStatus.deviceLoagin = 0;
	app_uart_send("server disconnet\r\n",strlen("server disconnet\r\n"));
	OS_MsDelay(100);
	set_connect_server_status(false);
	set_rev_server_data_flag(false);
	deviceStatus.socketClientCreateFlag = false;
	vTaskDelete(NULL);

}


static void user_tcp_server_task(void *arg)
{
	printf("[%d]:[%s]\r\n",__LINE__,__func__);
	
	int sockfd, newconn, size, ret;
    struct sockaddr_in address, remotehost;
BAGAIN:

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("can not create socket\r\n");
        goto exit;
    }
	
    ret = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));

    address.sin_family = AF_INET;
    address.sin_port = htons(USER_SOCKET_PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&address, sizeof (address)) < 0)
    {
        printf("can not bind socket\r\n");
        close(sockfd);
        goto exit;
    }

    listen(sockfd, 1); 
	
    size = sizeof(remotehost);
	newconn = accept(sockfd, (struct sockaddr *)&remotehost, (socklen_t *)&size);
	connetServerSocketId = newconn;

	deviceStatus.socketServerCreateFlag = true;
    while (1)
    {
    	vTaskDelay(50 / portTICK_RATE_MS);
        if (newconn >= 0) {
			deviceStatus.socketServerRevFlag = true;
			memset(deviceStatus.socketServerRevBuffer, 0 , BUFFER_SIZE_MAX);
		
			if (read(newconn, deviceStatus.socketServerRevBuffer, BUFFER_SIZE_MAX) > 0) {
				printf("receive msg =%s\r\n", deviceStatus.socketServerRevBuffer);

				OS_MsDelay(100);
				if (deviceStatus.socketServerRevBuffer[strlen(deviceStatus.socketServerRevBuffer) - 1] == '}') {
					get_wifi_param(deviceStatus.wifiSsid,deviceStatus.wifiKey);
					set_socket_send_ack(true);
				}
				//app_uart_command(deviceStatus.socketServerRevBuffer,strlen(deviceStatus.socketServerRevBuffer));
				app_uart_send(deviceStatus.socketServerRevBuffer,strlen(deviceStatus.socketServerRevBuffer));
				memset(deviceStatus.socketServerRevBuffer,0,BUFFER_SIZE_MAX);
			}else{
				close(newconn);
				close(sockfd);
				connetServerSocketId = -1;
				goto BAGAIN;
			}
        }
		if (get_socket_send_ack()) {	
			set_socket_send_ack(false);
			user_socket_send(newconn,NULL,SOCKET_SEND_WIFICONFIG_ACK);
			OS_MsDelay(1000);
			close(newconn);
			close(sockfd);
			connetServerSocketId = -1;
			softap_exit();
			OS_MsDelay(1000);

			wifi_disconnect(atwificbfunc);
			OS_MsDelay(1000);
			DUT_wifi_start(DUT_STA);
			set_device_mode(DUT_STA);
			OS_MsDelay(1000);
			wifi_connect_active((u8*)deviceStatus.wifiSsid, strlen(deviceStatus.wifiSsid), (u8*)deviceStatus.wifiKey, strlen(deviceStatus.wifiKey),atwificbfunc);
			goto exit;
		}
    }
 
exit:
	connetServerSocketId = -1;
	set_wifi_config_msg(false);
	deviceStatus.socketServerRevFlag = false;
	deviceStatus.socketServerCreateFlag = false;
    vTaskDelete(NULL);
}


int user_tcp_server_create(void)
{
	int ret = 0;
	
	ret = OS_TaskCreate(user_tcp_server_task, "user_tcp_server_task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
	OS_MsDelay(1000);
	return ret;
}

int user_tcp_client_create(void)
{
	int ret = 0;

	ret = OS_TaskCreate(user_tcp_client_task, "user_tcp_client_task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
	OS_MsDelay(1000);
	return ret;
}



