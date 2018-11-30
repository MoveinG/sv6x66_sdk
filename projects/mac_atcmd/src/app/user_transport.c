/****************************include*********************/

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
#include "wifi_api.h"
#include "netstack.h"
#include "netstack_def.h"
#include "uart/drv_uart.h"
#include "rf/rf_api.h"
#include "user_transport.h"
#include "user_aes.h"



/****************************typedef*********************/
typedef struct {
	WIFI_OPMODE deviceMode;
	unsigned char deviceConApStatus;
	unsigned char deviceConServerStatus;
}device_status_t;




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

unsigned char get_device_connect_ap_status(void)
{
	return deviceStatus.deviceConApStatus;
}

void set_connect_server_status(unsigned char status)
{
	deviceStatus.deviceConServerStatus = status;
}

unsigned char get_connect_server_status(void)
{
	return deviceStatus.deviceConServerStatus;
}


int tcp_server_init(void)
{
	int ret = true;
	return ret;
}


int connect_to_wifi(void)
{
	int ret = true;
	return ret;
}

int connect_to_user_server(void)
{
	int ret = true;
	return ret;
}

int user_socket_init(void)
{
	int ret = true;
	return ret;
}



void user_transport_init(WIFI_OPMODE mode)
{
	printf("[%s]:%d!\r\n",__func__,__LINE__);

	unsigned char encryptData[40] = {0};
	unsigned char decryptData[40] = {0};
	set_device_mode(mode);
	
	switch (mode)
	{	
		 case DUT_AP:
		 	if (!tcp_server_init())
			{
				printf("[%s]tcp create failure!\r\n",__func__);
			}
		 break;
		 
		 case DUT_STA:
			if (get_device_connect_ap_status())
			{
				set_device_connect_ap_status(true);
				if (connect_to_user_server())
				{
					set_connect_server_status(true);
				}
				else 
				{
					printf("[%s]connect to server failure!\r\n",__func__);
				}
			}
			else
			{
				if (!connect_to_wifi())
				{
					printf("[%s]connect to wifi failure!\r\n",__func__);
				}
				if (!get_auto_connect_flag())
				{
					set_auto_connect_flag(1);
				}
			}
			if(!user_socket_init())
			{
				printf("[%s]user socket create error!\r\n",__func__);
			}
		 break;
		 
		 default:
		 	printf("[%s]device mode error!\r\n",__func__);
		 break;
	}
}





void user_transport_func(void)
{
	switch (get_device_mode())
	{
		case DUT_AP:
			//if (get_wifi_config_msg_flag())
			{
				DUT_wifi_start(DUT_STA);
				set_device_mode(DUT_STA);
				set_auto_connect_flag(1);
				if (!connect_to_wifi())
				{
					printf("[%s]connect wifi failure!\r\n",__func__);
				}
			}
		break;

		case DUT_STA:
			if (get_connect_server_status())
			{
				//receive_server_data();
				//解析数据
				//发送数据
			}
			else
			{
				set_connect_server_status(false);
				if(!user_socket_init())
				{
					printf("[%s]user socket create error!\r\n",__func__);
				}
				if (connect_to_user_server())
				{
					set_connect_server_status(true);
				}
			}
		break;

		default:
			printf("[%s]device mode error!\r\n",__func__);
		break;
	}

}



