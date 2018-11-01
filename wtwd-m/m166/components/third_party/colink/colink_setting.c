//#include "esp_common.h"
//#include "freertos/task.h"
#include "fsal.h"
#include "lwip/sockets.h"
#include "mbedtls/sha256.h"
#include "cJSON.h"
#include "colink_define.h"
#include "colink_setting.h"
#include "colink_network.h"
#include "colink_type.h"
#include "colink_global.h"
#include "colink_link.h"
#include "colink_sysadapter.h"

#define COLINKFILE_NAME "colink.conf"
#define COLINKTIME_NAME "colink.time"
#define COLINK_DEV_NAME "colink.dev"

extern spiffs* fs_handle;
extern ColinkDevice colink_dev;

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

    colinkLinkInit(colink_dev.deviceid, colink_dev.apikey);
    colinkLinkReset();

    recv_buffer = (char *)os_malloc(512);
    outBuff = (char *)os_malloc(512);

    if(NULL == recv_buffer || NULL == outBuff)
    {
        os_printf("os_malloc err\r\n");
        goto exit;
    }
    /* create a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        os_printf("can not create socket\r\n");
        goto exit;
    }

    /* bind to port 80 at any interface */
    address.sin_family = AF_INET;
    address.sin_port = htons(80);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&address, sizeof (address)) < 0)
    {
        os_printf("can not bind socket\r\n");
        close(sockfd);
        goto exit;
    }

    /* listen for connections (TCP listen backlog = 1) */
    listen(sockfd, 1); 
    size = sizeof(remotehost);

    newconn = 8000; //8-second
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&newconn, sizeof(int)) != 0)
    {
        os_printf("set accept timeout failed");
        close(sockfd);
        goto exit;
    }

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
                        strcpy((char*)colink_flash_param.sta_config.ssid, colinkInfo.ssid);
                        strcpy((char*)colink_flash_param.sta_config.password, colinkInfo.password);
                    }
                    close(newconn);
                    close(sockfd);
                    break;
                }
                else
                {
                    os_printf("error %d\r\n", ret);
                }
            }
            close(newconn);
        }
        else
        {
            CoLinkDeviceMode mode = coLinkGetDeviceMode();
            if(mode != DEVICE_MODE_SETTING && mode != DEVICE_MODE_SETTING_SELFAP)
            {
                close(sockfd);
                os_printf("exit DeviceMode=%d\n", mode);
                goto exit;
            }
        }
    }

    os_free(outBuff);
    outBuff = NULL;

    os_free(recv_buffer);
    recv_buffer = NULL;

    os_printf("exit colinkSettingTask\r\n");
    if(DEVICE_MODE_SETTING_SELFAP == coLinkGetDeviceMode())
    {
        colinkSoftOverStart();
    }

    extern void mytime_clean_delay(void);
    mytime_clean_delay();

    colinkProcessStart();

exit:
    if(outBuff) os_free(outBuff);
    if(recv_buffer) os_free(recv_buffer);
    vTaskDelete(NULL);
}

void colinkSettingStart(void)
{
    xTaskCreate(colinkSettingTask, "colinkSettingTask", 512, NULL, 2, NULL);
}

/////////////////////////////////////
void colink_Init_Device(void)
{
	int size = colink_load_deviceid((char*)&colink_dev, sizeof(ColinkDevice));
	if(sizeof(ColinkDevice) != size)
	{
		printf("%s size=%d\n", __func__, size);
		memset(&colink_dev, 0, sizeof(ColinkDevice));
		memcpy(colink_dev.deviceid, DEVICEID, sizeof(colink_dev.deviceid));
		memcpy(colink_dev.apikey, APIKEY, sizeof(colink_dev.apikey));
		memcpy(colink_dev.model, MODEL, sizeof(colink_dev.model));
	}
}

int colink_deviceid_sha256(ColinkDevice *pdev, uint8_t digest_hex[65])
{
	mbedtls_sha256_context *psha256_ctx;
	uint8_t digest[32];
	uint8_t *pMac;
	int i;

	psha256_ctx = (mbedtls_sha256_context *)OS_MemAlloc(sizeof(mbedtls_sha256_context));
	if(psha256_ctx == NULL) return -1;

	pMac = pdev->sta_mac;
	sprintf((char*)digest, "%02x:%02x:%02x:%02x:%02x:%02x", pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);
	pMac = pdev->sap_mac;
	sprintf((char*)digest_hex, "%02x:%02x:%02x:%02x:%02x:%02x", pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);

	bzero(psha256_ctx, sizeof(mbedtls_sha256_context));
	vTaskSuspendAll();
	mbedtls_sha256_starts(psha256_ctx, 0);
	mbedtls_sha256_update(psha256_ctx, (const unsigned char*)pdev->deviceid, CK_DEVICE_LEN);
	mbedtls_sha256_update(psha256_ctx, (const unsigned char*)pdev->apikey, CK_APIKEY_LEN);
	mbedtls_sha256_update(psha256_ctx, (const unsigned char*)digest, 17);
	mbedtls_sha256_update(psha256_ctx, (const unsigned char*)digest_hex, 17);
	mbedtls_sha256_update(psha256_ctx, (const unsigned char*)pdev->model, CK_MODEL_LEN);
	mbedtls_sha256_finish(psha256_ctx, digest);
	xTaskResumeAll();
	OS_MemFree(psha256_ctx);

	for(i=0; i<64; i+=2)
		sprintf((char*)digest_hex+i, "%02x", digest[i/2]);
	digest_hex[64] = 0;

	return 0;
}

//////////////////////////////////////////////
int system_param_save_with_protect(char *domain, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINKFILE_NAME, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(domain && size>0) size = FS_write(fs_handle, fd, domain, size);
		FS_close(fs_handle, fd);
		return size;
	}
	return -1;
}

int system_param_load(char *domain, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINKFILE_NAME, SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(domain && size>0) size = FS_read(fs_handle, fd, domain, size);
		FS_close(fs_handle, fd);
		return size;
	}
	return -1;
}

/*void system_param_delete(void)
{
	FS_remove(fs_handle, COLINKFILE_NAME);
}*/

///////////////////////////////////////////////
int colink_save_deviceid(char *deviceid, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINK_DEV_NAME, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(deviceid && size>0) size = FS_write(fs_handle, fd, deviceid, size);
		FS_close(fs_handle, fd);
		return size;
	}
	return -1;
}

int colink_load_deviceid(char *deviceid, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINK_DEV_NAME, SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(deviceid && size>0) size = FS_read(fs_handle, fd, deviceid, size);
		FS_close(fs_handle, fd);
		return size;
	}
	return -1;
}

/////////////////////////////////////////////
int colink_save_timer(char *buffer, int size)
{
	SSV_FILE fd = FS_open(fs_handle, COLINKTIME_NAME, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		if(buffer && size>0) size = FS_write(fs_handle, fd, buffer, size);
		FS_close(fs_handle, fd);
		return size;
	}
	return -1;
}

int colink_load_timer(char **buffer)
{
	SSV_FILE fd = FS_open(fs_handle, COLINKTIME_NAME, SPIFFS_RDWR, 0);
	printf("%s fd=%d\n", __func__, fd);
	if(fd >= 0)
	{
		extern void *mytime_get_buffer(int size);
		int size;

		size = FS_lseek(fs_handle, fd, 0, SPIFFS_SEEK_END);
		*buffer = mytime_get_buffer(size);

		FS_lseek(fs_handle, fd, 0, SPIFFS_SEEK_SET);
		if(*buffer && size>0) size = FS_read(fs_handle, fd, *buffer, size);

		FS_close(fs_handle, fd);
		return size;
	}
	return -1;
}

void colink_delete_timer(void)
{
	FS_remove(fs_handle, COLINKTIME_NAME);
}

