#include "fsal.h"
#include "colink_global.h"
#include "colink_define.h"
#include "osal.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
#include "lwip/sockets.h"
#include "ipv4/lwip/ip4_addr.h"
#include "mbedtls/sha256.h"
#include "colink_profile.h"
//#include "upgrade.h"
#include "wdt/drv_wdt.h"
#include "ota_api.h"

#define uint32				uint32_t
#define uint16				uint16_t
#define uint8				uint8_t
#define SPI_FLASH_SEC_SIZE	(4096)

#define FILE_BUFFER_SIZE            (4096)
#define HTTP_BUFFER_SIZE            (2048)

#define IOTGO_UPGRADE_BIN1_START_SECTOR (0x00)
#define IOTGO_UPGRADE_BIN2_START_SECTOR (0x00)

#define HTTP_206_RETURN		"HTTP/1.1 206 Partial Content"
#define HTTP_206_RANGES		"Content-Range: bytes "
#define OTA_BIN_FILENAME	"ota.bin"
#define OTA_MD5_FILENAME	"ota_info.bin"

#define system_upgrade_userbin_check(void) (0)
extern spiffs* fs_handle;

typedef struct
{
    char version[12];
    char introduction[32];
    int32_t progress;
    int32_t bootTime;
} UpdateState;

typedef struct {
    char path[128];//
    char digest[68];//
    char version[24];
    char ip[24];//
    uint16_t port;//
    uint32_t file_length;
}OTAFileInfo;

static OTAFileInfo ota_file_info;
static UpdateState dev_upgrade_state;

static uint32_t sector_start = 0;

static OsTimer upgrade_timer;

static void cb2UpgradeTimer(void *arg)//
{
    static uint8_t conut = 0;

    if(conut++ < 2)
    {
        os_printf("upgrade not yet complete now!\n");
    }
    else
    {
        os_printf("upgrade successfully and reboot now!\n");
        OS_TimerDelete(upgrade_timer);

		/*mbedtls_md5_context ctx;
		uint8_t output[16], md5_hex[33];

		mbedtls_md5_init(&ctx);
		mbedtls_md5_starts(&ctx);
		mbedtls_md5_update(&ctx, 0x30000000, 0x90000);
		mbedtls_md5_finish(&ctx, output);
		mbedtls_md5_free(&ctx);
	    for(int i=0; i<32; i+=2)
	    {
	        sprintf((char*)md5_hex+i, "%02x", output[i/2]);
	    }
		md5_hex[32] = 0;
	    printf("md5=%s\n", md5_hex);*/

		SSV_FILE fd = FS_open(fs_handle, OTA_MD5_FILENAME, SPIFFS_CREAT | SPIFFS_RDWR, 0);
		if(fd < 0) return;

		FS_write(fs_handle, fd, (void*)ota_file_info.digest, sizeof(ota_file_info.digest));
		FS_close(fs_handle, fd);

		ota_set_parameter(OTA_BIN_FILENAME, "");
		ota_update();

        //system_upgrade_reboot();
        drv_wdt_init();
        drv_wdt_enable(SYS_WDT, 100);
    }
}

static bool verifyFlashData(uint32 start_addr, uint32 size, const uint8 digest_hex[65])
{
    mbedtls_sha256_context _sha256_ctx;
    uint8 digest[32];
    uint8 digest_hex2[65];
    uint16 i;
    uint8 *read_flash_buffer = (uint8 *)OS_MemAlloc(SPI_FLASH_SEC_SIZE);

    if(!read_flash_buffer)
    {
        os_printf("os_malloc to read_flash_buffer\n");
        return false;
    }

    vTaskSuspendAll();
    mbedtls_sha256_starts(&_sha256_ctx, 0);
    xTaskResumeAll();

	int32_t offset, seek_pos;
	SSV_FILE fd = FS_open(fs_handle, OTA_BIN_FILENAME, SPIFFS_RDONLY, 0);
	if(fd < 0)
	{
		OS_MemFree(read_flash_buffer);
		return false;
	}
    for (i = 0; i < (size / SPI_FLASH_SEC_SIZE); i++)
    {
        //if(SPI_FLASH_RESULT_OK == spi_flash_read(start_addr + (i * SPI_FLASH_SEC_SIZE),
        //        (uint32 *)read_flash_buffer, SPI_FLASH_SEC_SIZE))
		offset = start_addr + (i * SPI_FLASH_SEC_SIZE);
		seek_pos = FS_lseek(fs_handle, fd, offset, SPIFFS_SEEK_SET);
		if(offset == seek_pos)
		{
			FS_read(fs_handle, fd, read_flash_buffer, SPI_FLASH_SEC_SIZE);

            vTaskSuspendAll();
            mbedtls_sha256_update(&_sha256_ctx, read_flash_buffer, SPI_FLASH_SEC_SIZE);
            xTaskResumeAll();
        }
        else
        {
            os_printf("read sector 0x%02X err\n", start_addr / SPI_FLASH_SEC_SIZE + i);
        }
    }

    //spi_flash_read(start_addr + (i * SPI_FLASH_SEC_SIZE),
    //               (uint32 *)read_flash_buffer, size % SPI_FLASH_SEC_SIZE);
	offset = start_addr + (i * SPI_FLASH_SEC_SIZE);
	seek_pos = FS_lseek(fs_handle, fd, offset, SPIFFS_SEEK_SET);
	if(offset == seek_pos) FS_read(fs_handle, fd, read_flash_buffer, size % SPI_FLASH_SEC_SIZE);
	FS_close(fs_handle, fd);

    vTaskSuspendAll();
    mbedtls_sha256_update(&_sha256_ctx, read_flash_buffer, size % SPI_FLASH_SEC_SIZE);
    mbedtls_sha256_finish(&_sha256_ctx, digest);
    xTaskResumeAll();
    OS_MemFree(read_flash_buffer);

    for (i = 0; i < 64; i += 2)
    {
        sprintf((char*)&digest_hex2[i], "%02x", digest[i / 2]);
    }
    digest_hex2[64] = 0;
    os_printf("digest_flash = [%s]\n", digest_hex2);
    os_printf("digest_origin = [%s]\n", digest_hex);

    if (0 == strcmp((char*)digest_hex2, (char*)digest_hex))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool flashOneSector(uint16 sector, uint8 *src, uint16 len)//
{
	SSV_FILE fd = FS_open(fs_handle, OTA_BIN_FILENAME, SPIFFS_CREAT | SPIFFS_RDWR, 0);
	if(fd < 0) return false;

	int32_t	offset = sector * SPI_FLASH_SEC_SIZE;

	int32_t seek_pos = FS_lseek(fs_handle, fd, offset, SPIFFS_SEEK_SET);
	if(offset != seek_pos)
	{
		printf("seek_pos=%d, offset=%d\n", (int)seek_pos, offset);
		FS_close(fs_handle, fd);
		return false;
	}

	int32_t write_len = FS_write(fs_handle, fd, (void*)src, len);
	if(write_len != len)
	{
		printf("write_len=%d, len=%d\n", (int)write_len, len);
		FS_close(fs_handle, fd);
		return false;
	}
	FS_close(fs_handle, fd);

    return true;
}

static int32_t getUserBinLength(uint8 *pdata, uint16_t len)//
{
    char *t1, *t2, *t3;
    char bin_length_str[12] = "";
    int32_t bin_length = 0;
    
    t1 = strstr((char*)pdata, HTTP_206_RANGES);

    if(t1)
    {
        t1 += strlen(HTTP_206_RANGES);

        t2 = strstr(t1, "/");

        if(t2)
        {
            t2 += 1;

            t3 = strstr(t2, "\r\n");

            if(t3)
            {
                memcpy(bin_length_str, t2, t3-t2);
                bin_length_str[t3-t2] = '\0';
                os_printf("bin_length_str=[%s]\n", bin_length_str);

                bin_length = atoi(bin_length_str);
            }
        }
    }
    
    return bin_length;
}

static int sendHttpRequest(int sockfd, uint8_t *buffer, uint32_t start, uint32_t end)//
{
    mbedtls_sha256_context *psha256_ctx;
    uint8_t range[50] = {0};
    uint8 digest_hex[65] = {0};
    uint8 digest[32];
    char ts[20] = {0};
    int ret;
    int i;
    
    psha256_ctx = (mbedtls_sha256_context *)OS_MemAlloc(sizeof(mbedtls_sha256_context));
    bzero(psha256_ctx, sizeof(mbedtls_sha256_context));
    sprintf(ts, "%u", OS_Random()+0x80000000);
    vTaskSuspendAll();
    mbedtls_sha256_starts(psha256_ctx, 0);
    mbedtls_sha256_update(psha256_ctx, DEVICEID, strlen(DEVICEID));
    mbedtls_sha256_update(psha256_ctx, ts, strlen(ts));
    mbedtls_sha256_update(psha256_ctx, APIKEY, strlen(APIKEY));
    mbedtls_sha256_finish(psha256_ctx, digest);
    xTaskResumeAll();
    OS_MemFree(psha256_ctx);
    
    for (i = 0; i < 64; i += 2)
    {
        sprintf((char*)digest_hex+i, "%02x", digest[i / 2]);
    }
    
    digest_hex[64] = 0;
    os_printf("digest_hex=[%s]\n", digest_hex);

    bzero(buffer, HTTP_BUFFER_SIZE);

    sprintf((char*)range, "Range: bytes=%d-%d\r\n", start, end);
    os_printf("range[%s]\n", range);

    sprintf((char*)buffer, "GET %s?deviceid=%s&ts=%s&sign=%s HTTP/1.1\r\n", ota_file_info.path, DEVICEID, ts, digest_hex);
    sprintf((char*)buffer+strlen((char*)buffer), "Host: %s:%d\r\n", ota_file_info.ip, ota_file_info.port);
    strcat((char*)buffer, "User-Agent: itead-device\r\n");
    strcat((char*)buffer, (char*)range);
    strcat((char*)buffer, "\r\n");

	/*uint8_t ch; 
	i = strlen((char*)buffer);

	ch = buffer[100];
	buffer[100] = 0;
    os_printf("buffer=[%d][%s", i, buffer);
	buffer[100] = ch;

	if(i > 200)
	{
		ch = buffer[200];
		buffer[200] = 0;
	    os_printf("%s", buffer+100);
		buffer[200] = ch;
	}
    os_printf("%s]\n", buffer+200);*/

    ret = write(sockfd, buffer, strlen((char*)buffer));
    os_printf("ret=%d\n", ret);

    return ret;
}

static int launchUserbinDownload(int sockfd)//
{
    uint32_t sector_index = 0;
    bool start_download_flag = false;
    char *http_buffer = NULL;
    char *file_buffer = NULL;
    uint32_t range_start = 0;
    uint32_t range_end = 4095;
    uint32_t bin_file_size = 0;
    uint32_t current_bin_length = 0;
    int ret = 0;
    uint32_t recv_bin_length = 0;
    char *t1 = NULL;
    bool read_http_error_flag = false;

    os_printf("heap size = %u Bytes\n", OS_MemRemainSize());

    http_buffer = (char *)OS_MemAlloc(HTTP_BUFFER_SIZE);
    file_buffer = (char *)OS_MemAlloc(FILE_BUFFER_SIZE);

    if(http_buffer == NULL)
    {
        os_printf("malloc err");
        os_printf("\n##############_9\n");
        return -1;
    }

    if(file_buffer == NULL)
    {
        os_printf("malloc err");
        OS_MemFree(http_buffer);
        os_printf("\n##############_8\n");
        return -1;
    }
    
    while(1)
    {
        os_printf("range_start=%d range_end=%d\n", range_start, range_end);
        os_printf("heap size = %u Bytes\n", OS_MemRemainSize());
        
        ret = sendHttpRequest(sockfd, file_buffer, range_start, range_end);

        os_printf("heap size = %u Bytes\n", OS_MemRemainSize());

        if(ret <= 0)
        {
            os_printf("ret=%d", ret);
            OS_MemFree(file_buffer);
            OS_MemFree(http_buffer);
            os_printf("\n##############_7\n");
            return -1;
        }

        vTaskDelay(1 / portTICK_RATE_MS);
        ret = read(sockfd, http_buffer, HTTP_BUFFER_SIZE);
        os_printf("heap size = %u Bytes\n", OS_MemRemainSize());
        if(ret <= 0)
        {
            os_printf("ret=%d", ret);
            OS_MemFree(file_buffer);
            OS_MemFree(http_buffer);
            os_printf("\n##############_6\n");
            return -1;
        }

        os_printf("read data=[%d][%s]\n", ret, http_buffer);
#if 0
        char *http_debug = (char *)OS_MemAlloc(ret + 1);
        if(http_debug)
        {
            memcpy(http_debug, http_buffer, ret);
            http_debug[ret] = '\0';
            hilink_info("recv http=[%d][%s]", ret, http_debug);
            OS_MemFree(http_debug);
        }
#endif
        
        if(0 != strncmp(http_buffer, HTTP_206_RETURN, strlen(HTTP_206_RETURN)))
        {
            os_printf("download file error");
            OS_MemFree(file_buffer);
            OS_MemFree(http_buffer);
            os_printf("\n##############_5\n");
            return -1;
        }

        if(!start_download_flag)
        {
            start_download_flag = true;
            bin_file_size = getUserBinLength(http_buffer, ret);
        }

        if(0 == bin_file_size)
        {
            os_printf("bin_file_size");
            OS_MemFree(file_buffer);
            OS_MemFree(http_buffer);
            os_printf("\n##############_4\n");
            return -1;
        }

        ota_file_info.file_length = bin_file_size;
        
        bzero(file_buffer, FILE_BUFFER_SIZE);
        t1 = strstr(http_buffer, "\r\n\r\n");
        if(NULL == t1)
        {
            os_printf("http data error");
            OS_MemFree(file_buffer);
            OS_MemFree(http_buffer);
            os_printf("\n##############_3\n");
            return -1;
        }

        recv_bin_length = ret - (t1 - http_buffer + 4);
        if(recv_bin_length > 0)
        {
            memcpy(file_buffer, t1+4, recv_bin_length);
        }

        do{
            ret = read(sockfd, http_buffer, HTTP_BUFFER_SIZE);

            if(ret <= 0)
            {
                os_printf("ret=%d\n", ret);
                read_http_error_flag = true;
                break;
            }
            else
            {
                read_http_error_flag = false;
            }

            memcpy(&file_buffer[recv_bin_length], http_buffer, ret);
            recv_bin_length += ret;
            current_bin_length += ret;
            os_printf("recv_bin_length=%d ret=%d current_bin_length=%d\n", recv_bin_length, ret, current_bin_length);
        }while(recv_bin_length < FILE_BUFFER_SIZE && current_bin_length < bin_file_size );
        
        if(!read_http_error_flag)
        {
            if(!flashOneSector(sector_start + sector_index, file_buffer, recv_bin_length))
            {
                OS_MemFree(file_buffer);
                OS_MemFree(http_buffer);
                os_printf("\n##############_2\n");
                return -2;
            }
            sector_index++;

            range_start = range_end + 1;
            if(range_end + 4096 >= bin_file_size - 1 )
            {
                range_end = bin_file_size - 1;
            }
            else
            {
                range_end += 4096;
            }
        }

        if(current_bin_length >= bin_file_size)
        {
            os_printf("download file success!\n");
            break;
        }

        os_printf("flashOneSector=%d\n", sector_start + sector_index);

        os_printf("current=%d, length=%d\n", current_bin_length, bin_file_size);
        dev_upgrade_state.progress = (current_bin_length*100)/bin_file_size;
        
        if(0 < dev_upgrade_state.progress && dev_upgrade_state.progress <= 99 && DEVICE_ONLINE == colinkGetDevStatus())
        {
            os_printf("progress=%d\n", dev_upgrade_state.progress);
        }
    }

    OS_MemFree(file_buffer);
    OS_MemFree(http_buffer);

    if(current_bin_length >= bin_file_size)
    {
        return 0;
    }
    else
    {
        os_printf("\n##############_1\n");
        return -1;
    }
}

static int startDownloadOTAFile(void)//
{
    int sockfd = -1;
    int ret = 0;
    struct ip4_addr addr;
    struct sockaddr_in addr_in;

    bzero(&addr, sizeof(addr));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(ota_file_info.port);
    addr_in.sin_addr.s_addr = inet_addr(ota_file_info.ip);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        os_printf("socket fail\n");
    }
    else
    {
        ret = connect(sockfd, (struct sockaddr*)&addr_in, sizeof(addr_in));
        os_printf("connect ret=%d\n", ret);

        ret = launchUserbinDownload(sockfd);
        close(sockfd);
    }
    return ret;
}

static void upgrade_task(void* pData)//
{
    int ret;

    os_printf("upgrade_task\n");

    coLinkSetDeviceMode(DEVICE_MODE_UPGRADE);

    //system_upgrade_flag_set(UPGRADE_FLAG_START);

	FS_remove(fs_handle, OTA_BIN_FILENAME);

    ret = startDownloadOTAFile();

    os_printf("upgrade exit (ret=%d)\n", ret);

    if(ret == 0)
    {
        if(verifyFlashData(sector_start * SPI_FLASH_SEC_SIZE, ota_file_info.file_length, ota_file_info.digest))
        {
            //system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
            dev_upgrade_state.progress = 100;

            colinkUpgradeRes(COLINK_OTA_NO_ERROR);

            //os_timer_disarm(&upgrade_timer);
            //os_timer_setfn(&upgrade_timer, (os_timer_func_t *)cb2UpgradeTimer, NULL);
            //os_timer_arm(&upgrade_timer, 1000, 1);
            OS_TimerCreate(&upgrade_timer, 1000, (u8)TRUE, NULL, (OsTimerHandler)cb2UpgradeTimer);
            OS_TimerStart(upgrade_timer);

        }
        else
        {
            //system_upgrade_flag_set(UPGRADE_FLAG_IDLE);

            colinkUpgradeRes(COLINK_OTA_DIGEST_ERROR);

            dev_upgrade_state.progress = 1002;
        }
    }
    else if(ret == -1)
    {
        dev_upgrade_state.progress = 1001;

        colinkUpgradeRes(COLINK_OTA_DOWNLOAD_ERROR);

        //system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
    }
    else if(ret == -2)
    {
        dev_upgrade_state.progress = 1002;

        colinkUpgradeRes(COLINK_OTA_DOWNLOAD_ERROR);

        //system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
    }

    coLinkSetDeviceMode(DEVICE_MODE_WORK_NORMAL);

    vTaskDelete(NULL);

}

void iotgoUpgradeModeStart(void)//
{
    xTaskCreate(upgrade_task, "upgrade_task", 1024, NULL, 3, NULL);
}

static bool getServerInfo(char *url, char *ip, uint16_t *port, char *path)//
{
    char *t1, *t2, *t3;
    char port_str[8] = "";

    t1 = strstr(url, "//");
    t2 = strstr(t1, ":");

    if(NULL == t1 || NULL == t2)
    {
        os_printf("getServerInfo\n");
        return false;
    }

    memcpy(ip, t1+2, t2-(t1+2));

    t3 = strstr(t2, "/");

    if(NULL == t3)
    {
        os_printf("getServerInfo\n");
        return false;
    }
    
    memcpy(port_str, t2+1, t3-(t2+1));

    *port = atoi(port_str);

    strcpy(path, t3);

    return true;
}

void colinkUpgradeRequest(char *new_ver, ColinkOtaInfo file_list[], uint8_t file_num)
{
    os_printf("colinkUpgradeRequest %s num=%d\n", new_ver, file_num);

    /*os_printf("%s\n", file_list[0].name);
    os_printf("%s\n", file_list[0].download_url);
    os_printf("%s\n", file_list[0].digest);

    os_printf("%s\n", file_list[1].name);
    os_printf("%s\n", file_list[1].download_url);
    os_printf("%s\n", file_list[1].digest);*/

    if(file_num != 2)
    {
        os_printf("sever send error quantity\n");
        colinkUpgradeRes(COLINK_OTA_MODEL_ERROR);
        return;
    }

    int user_bin = 0;

    user_bin = system_upgrade_userbin_check();

    if(user_bin == 0x00)
    {
        if(0 == strcmp(file_list[1].name, "user2.bin"))
        {
            sector_start = IOTGO_UPGRADE_BIN2_START_SECTOR;
            strcpy(ota_file_info.digest, file_list[1].digest);
            getServerInfo(file_list[1].download_url, ota_file_info.ip, &(ota_file_info.port), ota_file_info.path);
        }
        else
        {
            os_printf("err user2.bin\n");
            colinkUpgradeRes(COLINK_OTA_MODEL_ERROR);
            return;
        }
    }
    else
    {
        if(0 == strcmp(file_list[0].name, "user1.bin"))
        {
            sector_start = IOTGO_UPGRADE_BIN1_START_SECTOR;
            strcpy(ota_file_info.digest, file_list[0].digest);
            getServerInfo(file_list[0].download_url, ota_file_info.ip, &(ota_file_info.port), ota_file_info.path);
        }
        else
        {
            os_printf("err user1.bin\n");
            colinkUpgradeRes(COLINK_OTA_MODEL_ERROR);
            return;
        }
    }
    
    os_printf("start download sector is %d\n", sector_start);
    iotgoUpgradeModeStart();

}


