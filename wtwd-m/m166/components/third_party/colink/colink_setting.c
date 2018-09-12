//#include "esp_common.h"
//#include "freertos/task.h"
#include "fsal.h"
#include "lwip/sockets.h"
#include "cJSON.h"
#include "colink_define.h"
#include "colink_setting.h"
#include "colink_network.h"
#include "colink_type.h"
#include "colink_global.h"
#include "colink_link.h"

#define COLINKFILE_NAME "colink.conf"
extern spiffs* fs_handle;

/////////////////////////////////////////////////
void colinkSettingTask(void* pData)
{
    char *outBuff = NULL;
    uint16_t outLen = 0;
    ColinkLinkInfo colinkInfo;
    int sockfd, newconn, size, ret;
    struct sockaddr_in address, remotehost;
    char *recv_buffer = NULL;
    char *t1 = NULL;
    char *t2 = NULL;
    int Length = 0;
    char *content = NULL;
    cJSON *cjson_root = NULL;
    cJSON *cjson_server_name = NULL;
    //char domain[32] = "";

    os_printf("colinkSettingTask\r\n");

    colinkLinkInit(DEVICEID, APIKEY);
    colinkLinkReset();

    recv_buffer = (char *)os_malloc(512);
    outBuff = (char *)os_malloc(512);

    if(NULL == recv_buffer)
    {
        os_printf("os_malloc err\r\n");
        vTaskDelete(NULL);
        return;
    }
    /* create a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        os_printf("can not create socket\r\n");
        os_free(recv_buffer);
        vTaskDelete(NULL);
        return;
    }

    /* bind to port 80 at any interface */
    address.sin_family = AF_INET;
    address.sin_port = htons(80);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&address, sizeof (address)) < 0)
    {
        os_printf("can not bind socket\r\n");
        close(sockfd);
        os_free(recv_buffer);
        vTaskDelete(NULL);
        return;
    }

    /* listen for connections (TCP listen backlog = 1) */
    listen(sockfd, 1); 
    size = sizeof(remotehost);

    while (1)
    {
        newconn = accept(sockfd, (struct sockaddr *)&remotehost, (socklen_t *)&size);

        if (newconn >= 0)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            ret = read(newconn, recv_buffer, 512); 

            ret = colinkLinkParse((uint8_t*)recv_buffer, ret, (uint8_t*)outBuff, 1024, &outLen);
            //os_printf("outBuff %d = %s\r\n", outLen, outBuff);
            os_printf("ret = %d\r\n", ret);

            if(COLINK_LINK_RES_DEV_INFO_OK == ret)
            {
                ret = write(newconn, outBuff, outLen);
                os_printf("write result:%d\r\n", ret);
            }
            else if(COLINK_LINK_GET_INFO_OK == ret)
            {
                ret = write(newconn, outBuff, outLen);
                os_printf("write result:%d\r\n", ret);

                ret = colinkLinkGetInfo(&colinkInfo);
                if(!ret)
                {
                    os_printf("net info :\r\n");
                    os_printf("ssid:%s\r\n", colinkInfo.ssid);
                    os_printf("password:%s\r\n", colinkInfo.password);
                    os_printf("distor_domain:%s\r\n", colinkInfo.distor_domain);
                    os_printf("distor_port:%d\r\n", colinkInfo.distor_port);
                    
                    //strcpy(colinkInfoSetting.ssid, colinkInfo.ssid);
                    //strcpy(colinkInfoSetting.password, colinkInfo.password);
                    //strcpy(colinkInfoSetting.distor_domain, colinkInfo.distor_domain);
                    //colinkInfoSetting.distor_port =  colinkInfo.distor_port;
                    
                    system_param_save_with_protect(/*DEVICE_CONFIG_START_SEC,*/(char*)&colinkInfo, sizeof(colinkInfo));

                    if(DEVICE_MODE_SETTING_SELFAP == coLinkGetDeviceMode())
                    {
                        strcpy(colink_flash_param.sta_config.ssid, colinkInfo.ssid);
                        strcpy(colink_flash_param.sta_config.password, colinkInfo.password);
                    }
                    break;
                }
                else
                {
                    colinkPrintf("error %d\r\n", ret);
                }
            }
            close(newconn);
        }
    }

    os_free(outBuff);
    os_free(recv_buffer);
    os_printf("exit colinkSettingTask\r\n");

    if(DEVICE_MODE_SETTING_SELFAP == coLinkGetDeviceMode())
    {
        colinkSoftOverStart();
    }

    colinkProcessStart();
    vTaskDelete(NULL);
}

void colinkSettingStart(void)
{
    xTaskCreate(colinkSettingTask, "colinkSettingTask", 512, NULL, 2, NULL);
}

//////////////////////////////////////////////
int system_param_save_with_protect(char *domain, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINKFILE_NAME, SPIFFS_CREAT | SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(domain && size>0) FS_write(fs_handle, fd, domain, size);
		FS_close(fs_handle, fd);
		return 0;
	}
	return -1;
}

int system_param_load(char *domain, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINKFILE_NAME, SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(domain && size>0) FS_read(fs_handle, fd, domain, size);
		FS_close(fs_handle, fd);
		return 0;
	}
	return -1;
}

void system_param_delete(void)
{
	FS_remove(fs_handle, COLINKFILE_NAME);
}

