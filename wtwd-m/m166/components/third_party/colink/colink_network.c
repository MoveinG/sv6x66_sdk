//#include "esp_common.h"
#include <string.h>
#include "colink_profile.h"
#include "colink_define.h"
#include "colink_network.h"
#include "colink_setting.h"
#include "colink_sysadapter.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "cJSON.h"
#include "colink_global.h"
#include "colink_upgrade.h"
#include "colink_link.h"
#include "iotapi/wifi_api.h"
#include "tools/atcmd/sysconf_api.h"
#include "mytime.h"

//static char test_read_tcp[1024];
//static int fd;
//static bool esptouch_flag = false;
static bool task_process_flag = false;

extern int32_t softap_set_custom_conf(CoLinkFlashParam*);

static void colinkSelfAPTask(void* pData)
{
    os_printf("Enter Self Ap Mode!\n");

    softap_exit();
    if(get_DUT_wifi_mode() != DUT_STA ) DUT_wifi_start(DUT_STA);

    colink_flash_param.sap_config.start_ip = 0x0a0a0702; //10, 10, 7, 2
    colink_flash_param.sap_config.end_ip   = 0x0a0a0705; //10, 10, 7, 5
    colink_flash_param.sap_config.gw       = 0x0a0a0701; //10, 10, 7, 1
    colink_flash_param.sap_config.subnet   = 0xffffff00; //255, 255, 255, 0

    strcpy((char*)colink_flash_param.sap_config.ssid, "ITEAD-");
    strcat((char*)colink_flash_param.sap_config.ssid, DEVICEID);
    colink_flash_param.sap_config.ssid_length = 16;

    strcpy((char*)colink_flash_param.sap_config.key, "12345678");
    colink_flash_param.sap_config.keylen = 8;

    colink_flash_param.sap_config.channel = 7;
    colink_flash_param.sap_config.encryt_mode = 4;
    //colink_flash_param.ssid_hidden = 0;
    colink_flash_param.sap_config.max_sta_num = 4;
    colink_flash_param.sap_config.beacon_interval = 100;

    coLinkSetDeviceMode(DEVICE_MODE_SETTING_SELFAP);

    if(softap_set_custom_conf((void*)&colink_flash_param.sap_config) == -4) printf("Don't configure SoftAP when SoftAP is running\n");

    DUT_wifi_start(DUT_AP);

    colinkSettingStart();

    vTaskDelete(NULL);
}

void enterSettingSelfAPMode(void)
{
    xTaskCreate(colinkSelfAPTask, "colinkSelfAPTask", 1024, NULL, 3, NULL);
}

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
    coLinkSetDeviceMode(DEVICE_MODE_SETTING);
    smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS);
    smartconfig_start(cbSmartConfig);
    vTaskDelete(NULL);
}
#endif

static uint16_t mytime_str_duration(const char *str)
{
	while(*str != 0)
	{
		if(*str == 0x20)
		{
			return (uint16_t)atoi(str+1);
		}
		str++;
	}
	return 0;
}

static void colinkRecvUpdate(char* data)
{
    extern void Ctrl_Switch_cb(int open);
    extern void Timer_update_time(void);

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

        cJSON_Delete(json_root);
        return;
    }

	cJSON *timers_p;
	cJSON *one_timer_p;
	cJSON *timer_do_p;
	cJSON *timer_startDo_p;
	cJSON *timer_endDo_p;
	cJSON *json_temp_p;
	uint8_t i, timer_num;
	colink_app_timer *app_timer, *buffer=NULL;

	timers_p = cJSON_GetObjectItem(json_root, "timers");
	if (timers_p)
	{
		timer_num = cJSON_GetArraySize(timers_p);

		if(timer_num > 0) buffer = mytime_get_buffer(timer_num * sizeof(colink_app_timer));
		else mytime_clean_delay();

		app_timer = buffer;
		for (i = 0; i < timer_num; i++)
		{
			if(app_timer) memset((char*)app_timer, 0, sizeof(colink_app_timer));

			one_timer_p = cJSON_GetArrayItem(timers_p, i);
			json_temp_p = cJSON_GetObjectItem(one_timer_p, "type");
			if (!colinkStrcmp(json_temp_p->valuestring, "once"))
			{
				app_timer->type = COLINK_TYPE_ONCE;
				json_temp_p = cJSON_GetObjectItem(one_timer_p, "enabled");
				if(json_temp_p->valueint != 0) app_timer->enable = 1;

				json_temp_p = cJSON_GetObjectItem(one_timer_p, "at");
				app_timer->at_time = mytime_str_to_time(json_temp_p->valuestring);

				timer_do_p = cJSON_GetObjectItem(one_timer_p, "do");
				json_temp_p = cJSON_GetObjectItem(timer_do_p, "switch");
				if(!colinkStrcmp(json_temp_p->valuestring, "on")) app_timer->start_do = 1;
			}
			else if (!colinkStrcmp(json_temp_p->valuestring, "repeat"))
			{
				app_timer->type = COLINK_TYPE_REPEAT;
				json_temp_p = cJSON_GetObjectItem(one_timer_p, "enabled");
				if(json_temp_p->valueint != 0) app_timer->enable = 1;

				json_temp_p = cJSON_GetObjectItem(one_timer_p, "at");
				app_timer->cron = mytime_str_repeat(json_temp_p->valuestring);

				timer_do_p = cJSON_GetObjectItem(one_timer_p, "do");
				json_temp_p = cJSON_GetObjectItem(timer_do_p, "switch");
				if(!colinkStrcmp(json_temp_p->valuestring, "on")) app_timer->start_do = 1;
			}
			else if (!colinkStrcmp(json_temp_p->valuestring, "duration"))
			{
				app_timer->type = COLINK_TYPE_DURATION;
				json_temp_p = cJSON_GetObjectItem(one_timer_p, "enabled");
				if(json_temp_p->valueint != 0) app_timer->enable = 1;

				json_temp_p = cJSON_GetObjectItem(one_timer_p, "at");
				app_timer->at_time = mytime_str_to_time(json_temp_p->valuestring);
				app_timer->min_b = atoi(json_temp_p->valuestring+25);
				app_timer->min_c = mytime_str_duration(json_temp_p->valuestring+25);

				timer_startDo_p = cJSON_GetObjectItem(one_timer_p, "startDo");
				json_temp_p = cJSON_GetObjectItem(timer_startDo_p, "switch");
				if(!colinkStrcmp(json_temp_p->valuestring, "on")) app_timer->start_do = 1;

				timer_endDo_p = cJSON_GetObjectItem(one_timer_p, "endDo");
				json_temp_p = cJSON_GetObjectItem(timer_endDo_p, "switch");
				if(!colinkStrcmp(json_temp_p->valuestring, "on")) app_timer->end_do = 1;
			}
			//os_printf("type=%d, do1=%d, do2=%d, time=%u, min_b=%d, min_c=%d\n",
			//			app_timer->type, app_timer->start_do, app_timer->end_do, app_timer->at_time, app_timer->min_b, app_timer->min_c);
			if(app_timer)
			{
				if(app_timer->type != 0) app_timer++;
			}
		}
		if(buffer)
		{
			//mytime_set_delay(buffer, timer_num);
			colink_save_timer((char*)buffer, timer_num * sizeof(colink_app_timer));
			Timer_update_time();
			//OS_MemFree(buffer);
		}
	}
	cJSON_Delete(json_root);
}

extern int get_Switch_status(void);
void colinkSwitchUpdate(void)
{
    cJSON *params = NULL;
    char *raw = NULL;
    char *switch_value = NULL;

	if(DEVICE_MODE_WORK_NORMAL != coLinkGetDeviceMode())
		return;

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

static void colinkNotifyDevStatus(ColinkDevStatus status)
{
    os_printf("colinkNotifyDevStatus %d\r\n", status);
    if(status == DEVICE_ONLINE) colinkSwitchUpdate();
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

    system_param_load(/*DEVICE_CONFIG_START_SEC, 0, */(char*)&colinkInfoCopy, sizeof(colinkInfoCopy));
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
    
    strcpy(dev_data->distor_domain, colinkInfoCopy.distor_domain);
    os_printf("distor_domain=[%s]\r\n", dev_data->distor_domain);

    colinkInit(dev_data, &ev);

    os_free(dev_data);

    while(1)
    {
        if((ret = colinkProcess()) != COLINK_PROCESS_NO_ERROR)
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
#else
static void colinkwificbfunc(WIFI_RSP *msg)
{
    extern void wifi_status_cb(int connect);

    uint8_t dhcpen;
    u8 mac[6];
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;

    if(msg->wifistatus == 1)
    {
        if(msg->id == 0)
            get_if_config_2("et0", &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
        else
            get_if_config_2("et1", &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
        printf("Wifi Connect\n");
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
        printf("Wifi Disconnect\n");
    }
    wifi_status_cb(msg->wifistatus);
}

static void colink_scan_cbfunc(void *data)
{
    printf("scan finish\n");

    int ssid_len = strlen((char*)colink_flash_param.sta_config.ssid);
    int password_len = strlen((char*)colink_flash_param.sta_config.password);

    int ret = set_wifi_config(colink_flash_param.sta_config.ssid, ssid_len, colink_flash_param.sta_config.password, password_len, NULL, 0);

    if(ret == 0)
        ret = wifi_connect(colinkwificbfunc);
}

void colinkSoftOverStart(void)
{
    softap_exit();
    if(get_DUT_wifi_mode() != DUT_STA ) DUT_wifi_start(DUT_STA);

    scan_AP(colink_scan_cbfunc);
}
#endif

void colinkProcessStart(void)
{
    if(task_process_flag) return;

    task_process_flag = true;
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

