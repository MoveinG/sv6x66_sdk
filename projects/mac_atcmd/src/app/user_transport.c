/****************************include*********************/
#include <string.h>
#include "soc_types.h"
#include "sys/backtrace.h"
#include "sys/xip.h"
#include "fsal.h"
#include "osal.h"
#include "wifinetstack.h"
#include "idmanage/pbuf.h"
#include "security/drv_security.h"
#include "phy/drv_phy.h"
#include "soc_defs.h"
#include "ieee80211_mgmt.h"
#include "ieee80211_mac.h"
#include "sta_func.h"
#include "error.h"
#include "wifi_api.h"
#include "netstack.h"
#include "netstack_def.h"
#include "uart/drv_uart.h"
#include "rf/rf_api.h"
#include "user_transport.h"
#include "user_aes.h"
#include "user_uart.h"


/****************************define*********************/


/****************************typedef*********************/




/***********************local variable ********************/
device_status_t deviceStatus;





/***********************local function ********************/
void set_device_mode(WIFI_OPMODE mode)
{
	deviceStatus.deviceMode = mode;
}

WIFI_OPMODE get_device_mode(void)
{
	return deviceStatus.deviceMode;
}

void set_device_connect_ap_status(unsigned char status)
{
	deviceStatus.deviceConApStatus = status;
}

int get_device_connect_ap_status(void)
{
	return deviceStatus.deviceConApStatus;
}

void set_connect_server_status(unsigned char status)
{
	deviceStatus.deviceConServerStatus = status;
}

int get_connect_server_status(void)
{
	return deviceStatus.deviceConServerStatus;
}

void set_wifi_config_msg(int value)
{
	deviceStatus.socketServerRevFlag = value;
}

int get_wifi_config_msg(void)
{
	return (deviceStatus.socketServerRevFlag);
}

void set_socket_send_ack(char value)
{
	deviceStatus.socketSendAckFlag = value;
}

char get_socket_send_ack(void)
{
	return deviceStatus.socketSendAckFlag;
}

void set_rev_server_data_flag(int value)
{
	deviceStatus.socketClientRevFlag = value;
}

int get_rev_server_data_flag(void)
{
	return (deviceStatus.socketClientRevFlag);
}

void get_wifi_param(char* wifiSsid,char* wifiKey)
{
	char *pbuffer = NULL;
	char *ppost = NULL;
	char *pkey = NULL;
	int len = 0;
	printf("\r\n---------------------------------\r\n");
	//memcpy(deviceStatus.socketServerRevBuffer,"{\"ssid\":\"A15\",\"password\":\"cdd@20180725\"}",42);
	if ((deviceStatus.socketServerRevBuffer[0] == '{') && \
		(deviceStatus.socketServerRevBuffer[1] == '\"'))
	{
		pbuffer = &deviceStatus.socketServerRevBuffer[1];
		if ((strstr(pbuffer,"ssid")) && (strstr(pbuffer,"password")))
		{
			pbuffer = &deviceStatus.socketServerRevBuffer[9];
			ppost = strchr(pbuffer,',');
			len = ppost - pbuffer - 1;
			memcpy(wifiSsid, pbuffer, len);
			printf("ssid=%s\r\n",wifiSsid);
			pbuffer = &deviceStatus.socketServerRevBuffer[9+len];
			ppost = strchr(pbuffer,':');
			ppost = ppost+2;
			pkey = strchr(ppost,'\"');
			len = pkey - ppost;
			memcpy(wifiKey, ppost, len);
			printf("key=%s\r\n",wifiKey);
		}
	}
	printf("\r\n---------------------------------\r\n");
}


void wifi_cb_func(WIFI_RSP *msg)
{
    uint8_t dhcpen;
    u8 mac[6];
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;

	//printf("[%s]:%d!\r\n",__func__,__LINE__);

    if(msg->wifistatus == 1)
    {
    	set_device_connect_ap_status(true);
        if(msg->id == 0)
            get_if_config_2("et0", &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
        else
            get_if_config_2("et1", &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
        printf("STA%d:\n", msg->id);
        printf("mac addr        - %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        printf("ip addr         - %d.%d.%d.%d\n", ipaddr.u8[0], ipaddr.u8[1], ipaddr.u8[2], ipaddr.u8[3]);
        printf("netmask         - %d.%d.%d.%d\n", submask.u8[0], submask.u8[1], submask.u8[2], submask.u8[3]);
        printf("default gateway - %d.%d.%d.%d\n", gateway.u8[0], gateway.u8[1], gateway.u8[2], gateway.u8[3]);
        printf("DNS server      - %d.%d.%d.%d\n", dnsserver.u8[0], dnsserver.u8[1], dnsserver.u8[2], dnsserver.u8[3]);

        recordAP();
    }
    else
    {
    	set_device_connect_ap_status(false);
    }
}


int connect_to_wifi(void)
{
    return (wifi_connect(wifi_cb_func));
}



void user_transport_init(WIFI_OPMODE mode)
{
	//printf("[%s]:%d!\r\n",__func__,__LINE__);

	app_uart_int();
	key_led_task_create();
	memset(&deviceStatus,0,sizeof(device_status_t));
	set_device_mode(mode);
	app_uart_send("user app start!\r\n",strlen("user app start!\r\n"));

	switch (mode)
	{	
		 case DUT_AP:
		 	printf("[%d]deviece mode is AP!\r\n",__LINE__);
			if (!user_tcp_server_create())
			{
				printf("[%d]tcp server create failure!\r\n",__LINE__);
			}
		 break;
		 
		 case DUT_STA:
		 	printf("[%d]deviece mode is STA!\r\n",__LINE__);
			
			if (!get_wifi_status())
			{
				if (!connect_to_wifi())
					set_device_connect_ap_status(true);
				else
					set_device_connect_ap_status(false);
			}
			else
			{
				set_device_connect_ap_status(true);
			}
			if (!get_auto_connect_flag())
			{
				set_auto_connect_flag(1);
			}
			if (!user_tcp_client_create())
			{
				printf("[%d]tcp client create failure!\r\n",__LINE__);
			}
		 break;
		 
		 default:
		 	//printf("[%d]device mode error!\r\n",__LINE__);
		 break;
	}
}





void user_transport_func(void)
{
	switch (get_device_mode())
	{
		case DUT_AP:
			if ((deviceStatus.socketServerCreateFlag == false) && (!get_socket_send_ack()))
			{
				OS_MsDelay(1000);
				user_tcp_server_create();
			}
		break;

		case DUT_STA:
			#if 1
			if (get_socket_send_ack())
			{
				set_socket_send_ack(false);
			}
			if (get_connect_server_status())
			{
				if (get_rev_server_data_flag())
				{
					set_rev_server_data_flag(false);
					app_uart_send(deviceStatus.socketClientRevBuffer,strlen(deviceStatus.socketClientRevBuffer));
					memset(deviceStatus.socketClientRevBuffer,0,BUFFER_SIZE_MAX);
					OS_MsDelay(100);
				}
			}
			else
			{
				if (deviceStatus.socketClientCreateFlag == false)
				{
					OS_MsDelay(1000);
					user_tcp_client_create();
				}
			}
			#endif
		break;

		default:
			//printf("[%s]device mode error!\r\n",__func__);
		break;
	}

}



