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
#include "colink_global.h"
#include "colink_upgrade.h"
#include "colink_link.h"


//static char test_read_tcp[1024];
//static int fd;
static bool esptouch_flag = false;
static bool task_process_flag = false;

#if 0
static void colinkSelfAPTask(void* pData)
{
    os_printf("Enter Self Ap Mode!\n");

    strcpy(colink_flash_param.sap_config.ssid, "ITEAD-");
    strcat(colink_flash_param.sap_config.ssid, DEVICEID);
    strcpy(colink_flash_param.sap_config.password, "12345678");
    colink_flash_param.sap_config.channel = 7;
    colink_flash_param.sap_config.authmode = 4;
    colink_flash_param.sap_config.ssid_len = 16;
    colink_flash_param.sap_config.ssid_hidden = 0;
    colink_flash_param.sap_config.max_connection = 4;
    colink_flash_param.sap_config.beacon_interval = 100;
    
    smartconfig_stop();
    coLinkSetDeviceMode(DEVICE_MODE_SETTING_SELFAP);
    
    if (wifi_station_disconnect())
    {
        os_printf("leave ap ok\n");
    }
    else
    {
        os_printf("leave ap err!\n");
    }

    if (STATIONAP_MODE != wifi_get_opmode())
    {
        if (wifi_set_opmode(STATIONAP_MODE))
        {
            os_printf("Set to STATIONAP_MODE ok\n");
        }
        else
        {
            os_printf("Set to STATIONAP_MODE err!\n");
        }
    }
    else
    {
        os_printf("Already in STATIONAP_MODE!\r\n");
    }

    if (wifi_softap_dhcps_stop())
    {
        os_printf("wifi_softap_dhcps_stop ok");
    }
    else
    {
        os_printf("wifi_softap_dhcps_stop err");
    }

    IP4_ADDR(&colink_flash_param.sap_ip_info.ip,       10, 10, 7, 1);
    IP4_ADDR(&colink_flash_param.sap_ip_info.gw,       10, 10, 7, 1);
    IP4_ADDR(&colink_flash_param.sap_ip_info.netmask,  255, 255, 255, 0);

    if (wifi_set_ip_info(SOFTAP_IF, &colink_flash_param.sap_ip_info))
    {
        os_printf("set sap ip ok\r\n");
    }
    else
    {
        os_printf("set sap ip err!\r\n");
    }

    if (wifi_softap_dhcps_start())
    {
        os_printf("wifi_softap_dhcps_start ok");
    }
    else
    {
        os_printf("wifi_softap_dhcps_start err");
    }

    if (wifi_softap_set_config(&(colink_flash_param.sap_config)))
    {
        os_printf("sap config ok\r\n");
    }
    else
    {
        os_printf("sap config err!\r\n");
    }

    colinkSettingStart();

    vTaskDelete(NULL);
}

void enterSettingSelfAPMode(void)
{
    xTaskCreate(colinkSelfAPTask, "colinkSelfAPTask", 1024, NULL, 3, NULL);
}

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
    coLinkSetDeviceMode(DEVICE_MODE_SETTING);
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
/*
static void colinkUpgradeRequest(char *new_ver, ColinkOtaInfo file_list[], uint8_t file_num)
{
    os_printf("colinkUpgradeRequest,");
    upgradeRequestFromSever(new_ver, file_list, file_num);

}
*/
static void colinkProcessTask(void* pData)
{
    int ret = 0;
    ColinkDev *dev_data = NULL;
    ColinkEvent ev;
    //char domain[32];
    ColinkLinkInfo colinkInfoCopy;
    
    os_printf("colinkProcessTask\r\n");

    coLinkSetDeviceMode(DEVICE_MODE_WORK_NORMAL);

    dev_data = (ColinkDev *)os_malloc(sizeof(ColinkDev));
    
    if(dev_data == NULL)
    {
        os_printf("os_malloc err\r\n");
        vTaskDelete(NULL);
        return ;
    }

    ev.colinkRecvUpdateCb = colinkRecvUpdate;
    ev.colinkNotifyDevStatusCb = colinkNotifyDevStatus;
    ev.colinkUpgradeRequestCb = colinkUpgradeRequest;/**< 升级通知的回调 */

    system_param_load(/*DEVICE_CONFIG_START_SEC, 0, */&colinkInfoCopy, sizeof(colinkInfoCopy));
    strcpy(dev_data->deviceid, DEVICEID);
    strcpy(dev_data->apikey, APIKEY);
    strcpy(dev_data->model, MODEL);
    strcpy(dev_data->version, VERDION);
    dev_data->dev_type = COLINK_SINGLE;
#ifdef SSLUSERENABLE
    dev_data->ssl_enable = true;
    dev_data->distor_port = colinkInfoCopy.distor_port;
    
#else
    dev_data->ssl_enable = false;
#endif
    
    strcpy(dev_data->distor_domain, /*"testapi.coolkit.cc"*/colinkInfoCopy.distor_domain);
    os_printf("distor_domain=[%s]\r\n", dev_data->distor_domain);

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

#if 0
static void colinkSoftapOverLinkApTask(void* pData)
{
    if (STATION_MODE != wifi_get_opmode())
    {
        if (wifi_set_opmode(STATION_MODE))
        {
            os_printf("Set to STATION_MODE ok\n");
        }
        else
        {
            os_printf("Set to STATION_MODE err!\n");
        }
    }

    printf("\n#######ssid=%s, password=%s, bssid_set=%d, bssid=%s\n",
            colink_flash_param.sta_config.ssid,
            colink_flash_param.sta_config.password,
            colink_flash_param.sta_config.bssid_set,
            colink_flash_param.sta_config.bssid
            );

    colink_flash_param.sta_config.bssid_set = 0;


    wifi_station_set_config(&colink_flash_param.sta_config);
    bool ret;
    wifi_station_disconnect();
    ret = wifi_station_connect();
    printf("the return of station is %d\n",ret);

    vTaskDelete(NULL);
}

void colinkSoftOverStart(void)
{
    xTaskCreate(colinkSoftapOverLinkApTask, "colinkSoftapOverLinkApTask", 512, NULL, 3, NULL);
}
#endif

void colinkProcessStart(void)
{
    xTaskCreate(colinkProcessTask, "colinkProcessTask", 2048, NULL, 4, NULL);
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

extern int get_Switch_status(void);
void colinkSwitchUpdate(void)
{
    cJSON *params = NULL;
    char *raw = NULL;
    char *switch_value = NULL;

    params = cJSON_CreateObject();

    if(get_Switch_status() == 0)
    {
        switch_value = "off";
    }
    else
    {
        switch_value = "on";
    }

    cJSON_AddStringToObject(params, "switch", switch_value);

    raw = cJSON_PrintUnformatted(params);

    if (!raw)
    {
        os_printf("cJSON_PrintUnformatted failed");
        cJSON_Delete(params);
        return;
    }

    colinkSendUpdate(raw); 
    cJSON_Delete(params);
    cJSON_free(raw);
}

