/****************************include*********************/
#include <string.h>
#include "soc_types.h"
#include "atcmd.h"
#include "error.h"
#include "Cabrio-conf.h"
#include "netstack_def.h"
#include "ssv_lib.h"
#include "wifi_api.h"
#include "user_transport.h"
#include "user_socket.h"



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
	char *pIp = 0;
    u16 port;    
    int socketId,newconn;
	static int ret;
    struct sockaddr_in s_sockaddr;
    int listen = 0;
	unsigned char buffer[BUFFER_SIZE_MAX] = {0};


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
		vTaskDelete(NULL);
        return ERROR_INVALID_PARAMETER;
    }
    ret = connect(socketId, (struct sockaddr *) &s_sockaddr, sizeof(s_sockaddr));
    if (ret < 0)
    {
        printf("Failed to connect ret=%d\n",ret);
        close(socketId);      
		vTaskDelete(NULL);
        return ERROR_INVALID_PARAMETER;
    }

    if (update_new_sd(socketId, -1) != 0)
    {
        close(socketId);
        printf("fail to add socket\n");
		vTaskDelete(NULL);
        return ERROR_TCP_CONNECTION;
    }

	set_connect_server_status(true);
	while(1)
	{
		//if (send_cmd_flag) cmd from uart
		send(ret, "Hello", strlen("Hello"), 0);
		read(newconn, buffer, BUFFER_SIZE_MAX);
		if(buffer[0] != 0)
		{
			memcpy(deviceStatus.socketClientRevBuffer,buffer,strlen(buffer));
			set_rev_server_data_flag(true);
			printf("receive msg =%s\r\n", buffer);
			memset(buffer,0,BUFFER_SIZE_MAX);
		}
        OS_MsDelay(1000);
	}
	vTaskDelete(NULL);
}


static void user_tcp_server_task(void *arg)
{
	int sockfd, newconn, size, ret;
    struct sockaddr_in address, remotehost;
   	char readlen;
    printf("tcpservertask\r\n");
	unsigned char buffer[100] = {0};

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

	
	newconn = accept(sockfd, (struct sockaddr *)&remotehost, (socklen_t *)&size);
    while (1)
    {
    	vTaskDelay(1000 / portTICK_RATE_MS);
        if (newconn >= 0)
        {
			set_wifi_config_msg(true);
			memset(deviceStatus.socketServerRevBuffer, 0 , BUFFER_SIZE_MAX);
			read(newconn, deviceStatus.socketServerRevBuffer, BUFFER_SIZE_MAX);
			printf("receive msg =%s\r\n", deviceStatus.socketServerRevBuffer);
        }
		if (get_socket_send_ack())
		{	
			//这里需要吧设备的mac地址和封包为{"ack":"11:12:13:aa:bb:cc"}的形式发送打用户APP；
			//send(ret, "{\"ack\":\"11:12:13:aa:bb:cc\"}", strlen("{\"ack\":\"11:12:13:aa:bb:cc\"}"), 0);
			vTaskDelay(1000 / portTICK_RATE_MS);
			goto exit;
		}
    }

 
exit:
    vTaskDelete(NULL);
}


int user_tcp_server_create(void)
{
	int ret = 0;
	
	ret = OS_TaskCreate(user_tcp_server_task, "user_tcp_server_task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);

	return ret;
}

int user_tcp_client_create(void)
{
	int ret = 0;

	if (deviceStatus.socketCreateFlag != true)
	{
		deviceStatus.socketCreateFlag = true;
		ret = OS_TaskCreate(user_tcp_client_task, "user_tcp_client_task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
	}
	return ret;
}



