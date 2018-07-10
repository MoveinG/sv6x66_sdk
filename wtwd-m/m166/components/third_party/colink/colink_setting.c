//#include "esp_common.h"
//#include "freertos/task.h"
#include "lwip/sockets.h"
#include "cJSON.h"
#include "colink_define.h"
#include "colink_setting.h"
#include "colink_network.h"

static int sigSettingSendIDToAPP(int sock_fd)
{
    char temp[20];
    char *http_buffer = NULL;
    cJSON *root = NULL;
    char *out = NULL;
    int ret = 0;

    http_buffer = (char *)os_malloc(512);

    if(NULL == http_buffer)
    {
        os_printf("os_malloc err\r\n");
        return -1;
    }
    memset(http_buffer, '\0', 512);

    root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "deviceid", DEVICEID);
    cJSON_AddStringToObject(root, "apikey", APIKEY);
    cJSON_AddStringToObject(root, "accept", "post");

    out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    sprintf(temp, "%d", strlen(out));

    strcpy(http_buffer, "HTTP/1.1 200 OK\r\n");
    strcat(http_buffer, "Content-Type: application/json\r\n");
    strcat(http_buffer, "Connection: keep-alive\r\n");
    strcat(http_buffer, "Content-Length: ");
    strcat(http_buffer, temp);
    strcat(http_buffer, "\r\n\r\n");
    strcat(http_buffer, out);
    os_free(out);

    os_printf("send data=[%d][%s]\r\n", strlen(http_buffer), http_buffer);
    ret = write(sock_fd, http_buffer, strlen(http_buffer));

    if(ret < 0)
    {
        os_printf("tcp send err\r\n");
    }

    os_free(http_buffer);

    return ret;
}

static int sigSettingSendRespToAPP(int sock_fd)
{
    char temp[20] = "";
    int ret = 0;
    char *http_buffer = NULL;
    char *str_error0 = "{\"error\":0}";

    http_buffer = (char *)os_malloc(512);
    if(NULL == http_buffer)
    {
        os_printf("os_malloc error");
        return -1;
    }
    
    sprintf(temp, "%d", strlen(str_error0));
    strcpy(http_buffer, "HTTP/1.1 200 OK\r\n");
    strcat(http_buffer, "Content-Type: application/json\r\n");
    strcat(http_buffer, "Connection: keep-alive\r\n");
    strcat(http_buffer, "Content-Length: ");
    strcat(http_buffer, temp);
    strcat(http_buffer, "\r\n\r\n");
    strcat(http_buffer, str_error0);

    os_printf("send data=[%d][%s]\r\n", strlen(http_buffer), http_buffer);
    
    ret = write(sock_fd, http_buffer, strlen(http_buffer));

    os_free(http_buffer);
    if(ret < 0)
    {
        os_printf("tcp send err\r\n");
    }
    return ret;
}

void colinkSettingTask(void* pData)
{
    int sockfd, newconn, size, ret;
    struct sockaddr_in address, remotehost;
    char *recv_buffer = NULL;
    char *t1 = NULL;
    char *t2 = NULL;
    int Length = 0;
    char *content = NULL;
    cJSON *cjson_root = NULL;
    cJSON *cjson_server_name = NULL;
    char domain[32] = "";

    os_printf("colinkSettingTask\r\n");

    recv_buffer = (char *)os_malloc(512);

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
            ret = read(newconn, recv_buffer, 512); 
            os_printf("recv_buffer=[%d][%s]\r\n", ret, recv_buffer);

            if(NULL != strstr(recv_buffer, "GET /device HTTP/1.1"))
            {
                ret = sigSettingSendIDToAPP(newconn);
            }
            else if(NULL != strstr(recv_buffer, "POST /ap HTTP/1.1"))
            {
                t1 = strstr(recv_buffer, "Content-Length: ");
                t1 += strlen("Content-Length: ");
                Length = atoi(t1);
                os_printf("Length=%d\r\n", Length);

                t2 = strstr(recv_buffer, "\r\n\r\n");
                t2 += 4;
                content = (char *)os_malloc(512);
                if(content)
                {
                    memset(content, 0, 512);
                    memcpy(content, t2, Length);

                    os_printf("content=[%s]\r\n", content);

                    cjson_root = cJSON_Parse(content);
                    cjson_server_name = cJSON_GetObjectItem(cjson_root, "serverName");
                    strcpy(domain, cjson_server_name->valuestring);
                    cJSON_Delete(cjson_root);

                    os_printf("domain=[%s]\r\n", domain);
                    //system_param_save_with_protect(DEVICE_CONFIG_START_SEC, domain, sizeof(domain));

                    os_free(content);
                }
                ret = sigSettingSendRespToAPP(newconn);
                break;
            }
            close(newconn);
        }
    }

    os_free(recv_buffer);
    os_printf("exit colinkSettingTask\r\n");
    colinkProcessStart();
    vTaskDelete(NULL);
}

void colinkSettingStart(void)
{
    xTaskCreate(colinkSettingTask, "colinkSettingTask", 512, NULL, 3, NULL);
}

