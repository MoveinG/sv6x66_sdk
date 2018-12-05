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
#define BUFFER_SIZE                    256


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






/***********************local variable ********************/










/***********************local function ********************/
static void user_tcp_client_task(void *arg)
{
	printf("[%d]:[%s]\r\n",__LINE__,__func__);
		
	char *pIp = 0;
    u16 port;    
    int socketId,newconn;
	static int ret;
    struct sockaddr_in s_sockaddr;
    int listen = 0;
	char pBuf = NULL;
	unsigned char buffer[BUFFER_SIZE_MAX] = {0};
	unsigned char aesBuffer[BUFFER_SIZE_MAX] = {0};


	pIp = USER_SERVER_IP;
	port = atoi (USER_SERVER_PORT);
	
    memset(&s_sockaddr, 0, sizeof(s_sockaddr));
    s_sockaddr.sin_family = AF_INET;
    s_sockaddr.sin_port = htons(port);
    inet_aton(pIp, &s_sockaddr.sin_addr);
    s_sockaddr.sin_len = sizeof(s_sockaddr);
    
    if ((socketId = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Failed to create socket\n");
		goto exit;
    }
    ret = connect(socketId, (struct sockaddr *) &s_sockaddr, sizeof(s_sockaddr));
    if (ret < 0)
    {
        printf("Failed to connect ret=%d\n",ret);
        close(socketId);      
		goto exit;
    }

    if (update_new_sd(socketId, -1) != 0)
    {
        close(socketId);
        printf("fail to add socket\n");
		goto exit;
    }

	set_connect_server_status(true);
	deviceStatus.socketClientCreateFlag = true;
	deviceStatus.socketClientRevFlag = true;
	while(1)
	{
		read(newconn, buffer, BUFFER_SIZE_MAX);
		if(buffer[0] != 0)
		{
			printf("buffer = %s\r\n",buffer);
			
			pBuf = strchr(buffer,'{');
			if ((pBuf != NULL) && (strlen(pBuf) > 2))
			{
				memset(aesBuffer,0,BUFFER_SIZE_MAX);
				memcpy(aesBuffer,(pBuf+1),strlen(pBuf) - 2);
				user_aes_decrypt(aesBuffer,buffer);
				memcpy(deviceStatus.socketClientRevBuffer,buffer,strlen(buffer));
				set_rev_server_data_flag(true);
			}
			memset(buffer,0,BUFFER_SIZE_MAX);
		}
		if (deviceStatus.uartCmdFlag)
		{
			deviceStatus.uartCmdFlag = false;
			memset(buffer,0,BUFFER_SIZE_MAX);
			user_aes_encrypt(deviceStatus.uartCmdBuffer,aesBuffer);
			sprintf(buffer,"{%s}",aesBuffer);
			printf("uart send to server buffer=%s\r\n",buffer);
			send(ret, buffer, strlen(buffer), 0);
			memset(aesBuffer,0,BUFFER_SIZE_MAX);
			memset(deviceStatus.uartCmdBuffer,0,BUFFER_SIZE_MAX);
		}
		if (get_device_mode == DUT_AP)
		{
			 goto exit;
		}
		
        OS_MsDelay(100);
	}

exit:
	set_connect_server_status(false);
	deviceStatus.socketClientCreateFlag = false;
	vTaskDelete(NULL);
}


void HexToStr(unsigned char *pbDest,unsigned  char *pbSrc,unsigned int nLen)
{
		unsigned char	ddl,ddh;
		unsigned int i;

		for (i=0; i<nLen; i++)
		{
			ddh = 48 + pbSrc[i] / 16;
			ddl = 48 + pbSrc[i] % 16;
			if (ddh > 57) ddh = ddh + 7;
			if (ddl > 57) ddl = ddl + 7;
			pbDest[i*2] = ddh;
			pbDest[i*2+1] = ddl;
		}

		pbDest[nLen*2] = '\0';
}

void read_mac1_addr(unsigned char *mac)
{
	printf("\n");
    void *cfg_handle = wifi_cfg_init();
    char mac_addr[6] = {0};
    wifi_cfg_get_addr1(cfg_handle, mac_addr);
    wifi_cfg_deinit(cfg_handle);
	memcpy(mac, mac_addr, sizeof(mac_addr));
	//printf("%02X:%02X:%02X:%02X:%02X:%02X\n", (uint8_t)mac_addr[0], (uint8_t)mac_addr[1], (uint8_t)mac_addr[2], (uint8_t)mac_addr[3], (uint8_t)mac_addr[4], (uint8_t)mac_addr[5]);
}

void send_client_ack(char *buffer)
{
	int i, j=0;
	unsigned char buf[30] = {0};
	unsigned char str[30] = {0};
	char arry[5] = {':'};
	char s[5] = "\"}";
	char dest[30] = "{\"ack\":\"";
	char *p = NULL;
	unsigned char macaddr[6] = {0};	

	for(i = 2; i < 15; i += 3)
	{
		memcpy(&buf[i], arry, 1);

	}
	read_mac1_addr(macaddr);
	HexToStr(str, macaddr, 6);
	for(i = 0; i < 18; i++)
	{
		if(i != 2 && i!= 5 && i!=8 && i!=11 && i!=14 ) 
		{
			memcpy(&buf[i], &str[j], 1);
			j++;
		}

	}
	strcat(buf, s);
	strcat(dest, buf);
	memcpy(buffer, dest, strlen(dest));

}

static void user_tcp_server_task(void *arg)
{
	printf("[%d]:[%s]\r\n",__LINE__,__func__);
	
	int sockfd, newconn, size, ret;
    struct sockaddr_in address, remotehost;
   	char readlen;
    printf("tcpservertask\r\n");
	unsigned char buffer[100] = {0};
	char buf[30];
	
	/* create a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("can not create socket\r\n");
        goto exit;
    }
    ret = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));

    /* bind to port 8000 at any interface */
    address.sin_family = AF_INET;
    address.sin_port = htons(8000);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&address, sizeof (address)) < 0)
    {
        printf("can not bind socket\r\n");
        close(sockfd);
        goto exit;
    }

    /* listen for connections (TCP listen backlog = 1) */
    listen(sockfd, 1); 
    size = sizeof(remotehost);

	deviceStatus.socketServerCreateFlag = true;
	newconn = accept(sockfd, (struct sockaddr *)&remotehost, (socklen_t *)&size);
    while (1)
    {
    	vTaskDelay(1000 / portTICK_RATE_MS);
        if (newconn >= 0)
        {
			deviceStatus.socketServerRevFlag = true;
			memset(deviceStatus.socketServerRevBuffer, 0 , BUFFER_SIZE_MAX);
			read(newconn, deviceStatus.socketServerRevBuffer, BUFFER_SIZE_MAX);
			printf("receive msg =%s\r\n", deviceStatus.socketServerRevBuffer);

			OS_MsDelay(100);

			char wifiSsid[20] = {0};
			char wifiKey[20]  = {0};
			get_wifi_param(wifiSsid,wifiKey);
			set_wifi_config((u8*)wifiSsid, strlen(wifiSsid), (u8*)wifiKey, strlen(wifiKey), NULL, 0);
			set_socket_send_ack(true);
        }
		if (get_socket_send_ack())
		{	
			send_client_ack(buf);
			send(newconn, buf, strlen(buf), 0);
			vTaskDelay(1000 / portTICK_RATE_MS);
			drv_wdt_init();
        	drv_wdt_enable(SYS_WDT, 100);
		}
		
    }

 
exit:
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



