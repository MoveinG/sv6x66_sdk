//#include "esp_common.h"
//#include "freertos/task.h"
#include "colink_profile.h"
#include "colink_define.h"
#include "colink_network.h"
#include "colink_setting.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "cJSON.h"
#include "colink_profile.h"




//static char test_read_tcp[1024];
//static int fd;
static bool esptouch_flag = false;
static bool task_process_flag = false;

#if 0
static void testSocketTask(void* pData)
{

    vTaskDelete(NULL);
}

static void colink_wifi_cb(System_Event_t* evt)
{
    os_printf("event %x\r\n", evt->event_id);
    switch (evt->event_id)
    {
    case EVENT_STAMODE_CONNECTED:
    {
        os_printf("connect to ssid %s, channel %d\r\n", evt->event_info.connected.ssid, 
                evt->event_info.connected.channel);
    }break;
    case EVENT_STAMODE_DISCONNECTED:
    {
        os_printf("disconnect from ssid %s, reason %d\r\n", evt->event_info.disconnected.ssid, 
                evt->event_info.disconnected.reason);
    }break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
    {
        os_printf("mode: %d -> %d\r\n", evt->event_info.auth_change.old_mode, 
                evt->event_info.auth_change.new_mode);
    }break;
    case EVENT_STAMODE_GOT_IP:
    {
        os_printf("ip:"IPSTR",mask:"IPSTR",gw:"IPSTR"\r\n",
                IP2STR(&evt->event_info.got_ip.ip),
                IP2STR(&evt->event_info.got_ip.mask),
                IP2STR(&evt->event_info.got_ip.gw));
        //xTaskCreate(testSocketTask, "testSocketTask", 512, NULL, 4, NULL);

        if(!esptouch_flag)
        {
			if(!task_process_flag)
			{
				task_process_flag = true;
				colinkProcessStart();
			}
        }
    }break;
    case EVENT_SOFTAPMODE_STACONNECTED:
    {
        os_printf("station: "MACSTR"join, AID = %d\r\n",  
                MAC2STR(evt->event_info.sta_connected.mac),    
                evt->event_info.sta_connected.aid);
    }break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
    {
        os_printf("station: "MACSTR"leave, AID = %d\r\n",  
                    MAC2STR(evt->event_info.sta_disconnected.mac), 
                    evt->event_info.sta_disconnected.aid);
    }break;
     default:
        break;
    }
}

static void cbSmartConfig(sc_status status, void *pdata)
{
    switch(status)
    {
    case SC_STATUS_WAIT:
        os_printf("SC_STATUS_WAIT\n");
        break;

    case SC_STATUS_FIND_CHANNEL:
        os_printf("SC_STATUS_FIND_CHANNEL\n");
        break;

    case SC_STATUS_GETTING_SSID_PSWD:
        os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
        sc_type *type = pdata;

        if (*type == SC_TYPE_ESPTOUCH)
        {
            os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
        }
        else
        {
            os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
        }

        break;

    case SC_STATUS_LINK:
        os_printf("SC_STATUS_LINK\n");
        struct station_config *sta_conf = pdata;

        if (!sta_conf)
        {
            os_printf("sta_conf is NULL!\r\n");
            break;
        }

        int8 len = strlen(sta_conf->password);
        int8 i = 0;

        for (i = 0; i < len; i++)
        {
            if ((i % 2) == 0)
            {
                sta_conf->password[i] -= 7;
            }
            else
            {
                sta_conf->password[i] -= 2;
            }
        }

        os_printf("ssid:[%s], password:[%s]\r\n", sta_conf->ssid, sta_conf->password);
        /**
            The esptouch process alawys sets BSSID to 1,
            then causing the device can only connect to the AP with specified mac.
            So U must force BSSID to 0
        */
        sta_conf->bssid_set = 0;

        wifi_station_set_config(sta_conf);
        wifi_station_disconnect();
        wifi_station_connect();
        esptouch_flag = true;
        break;

    case SC_STATUS_LINK_OVER:
        os_printf("SC_STATUS_LINK_OVER\n");
        if (pdata != NULL)
        {
            uint8 phone_ip[4] = {0};
            memcpy(phone_ip, (uint8 *)pdata, 4);
            os_printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2],
                        phone_ip[3]);
        }

        smartconfig_stop();
        colinkSettingStart();
        break;
    }

}

static void colinkEsptouchTask(void* pData)
{
    os_printf("colinkEsptouchTask\r\n");
    smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS);
    smartconfig_start(cbSmartConfig);
    vTaskDelete(NULL);
}
#endif

static void colinkRecvUpdate(char* data)
{
    extern void Ctrl_Switch_cb(int open);

    os_printf("colinkRecvUpdate [%s]\r\n", data);

    cJSON *json_root = NULL;
    cJSON *switch_p = NULL;

    json_root = cJSON_Parse(data);

    if (!json_root)
    {
        os_printf("parse json failed");
        return;
    }

    switch_p = cJSON_GetObjectItem(json_root, "switch");

    if (switch_p)
    {
        if(0 == colinkStrcmp(switch_p->valuestring, "on"))
        {
            os_printf("sever on instruction");
            Ctrl_Switch_cb(1); //switchON();
        }
        else if(0 == colinkStrcmp(switch_p->valuestring, "off"))
        {
            os_printf("sever off instruction");
            Ctrl_Switch_cb(0); //switchOFF();
        }
        else
        {
            os_printf("err net switch info...\n");
        }
    }



ExitErr1:
    cJSON_Delete(json_root);
    return;
}

static void colinkNotifyDevStatus(ColinkDevStatus status)
{
    os_printf("colinkNotifyDevStatus %d\r\n", status);
}

static void colinkProcessTask(void* pData)
{
    int ret = 0;
    ColinkDev *dev_data = NULL;
    ColinkEvent ev;
    char domain[32];
    
    os_printf("colinkProcessTask\r\n");
    
    dev_data = (ColinkDev *)os_malloc(sizeof(ColinkDev));
    
    if(dev_data == NULL)
    {
        os_printf("os_malloc err\r\n");
        vTaskDelete(NULL);
        return ;
    }

    ev.colinkRecvUpdate = colinkRecvUpdate;
    ev.colinkNotifyDevStatus = colinkNotifyDevStatus;

    strcpy(dev_data->deviceid, DEVICEID);
    strcpy(dev_data->apikey, APIKEY);
    strcpy(dev_data->model, MODEL);
    strcpy(dev_data->version, VERDION);
    //system_param_load(DEVICE_CONFIG_START_SEC, 0, domain, 32);
    strcpy(dev_data->distor_domain, domain);
    os_printf("distor_domain=[%s]\r\n", dev_data->distor_domain);
    dev_data->ssl_enable = true;
    colinkInit(dev_data, &ev);

    os_free(dev_data);

    while(1)
    {
        if(ret = colinkProcess())
        {
            os_printf("colinkProcess ret=%d\r\n", ret);
        }
        vTaskDelay(50 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void colinkProcessStart(void)
{
    xTaskCreate(colinkProcessTask, "colinkProcessTask", 1024, NULL, 3, NULL);
}

#if 0
void network_init(void)
{
    os_printf("network_init\r\n");

    if (!wifi_set_opmode(STATION_MODE))
    {
        os_printf("Set to STATION_MODE err!\r\n");
    }

    struct station_config sta_config;
    wifi_set_event_handler_cb(colink_wifi_cb);            //set wifi handler event cb
    memset(&sta_config, 0, sizeof(sta_config));
    wifi_station_get_config(&sta_config);

    os_printf("ssid = %s,pwd = %s\r\n", sta_config.ssid, sta_config.password);

    if('\0' == sta_config.ssid[0])
    {
        xTaskCreate(colinkEsptouchTask, "colinkEsptouchTask", 1024, NULL, 3, NULL);
    }
}
#endif

