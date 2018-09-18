//#include "esp_common.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "colink_define.h"
#include "colink_error.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "colink_socket.h"
#include "colink_sysadapter.h"
#include "colink_network.h"

#ifdef SSLUSERENABLE
	#define COLINK_SSL
	#define COLINK_VERIFY
#endif

#if defined(COLINK_VERIFY)

const char colink_test_cli_key_rsa[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIICxDCCAi2gAwIBAgIJAKZOAGaffv/UMA0GCSqGSIb3DQEBBQUAMHoxCzAJBgNV\r\n"
"BAYTAmNiMQswCQYDVQQIDAJnZDELMAkGA1UEBwwCc3oxEDAOBgNVBAoMB2Nvb2xr\r\n"
"aXQxDDAKBgNVBAsMA2RldjETMBEGA1UEAwwKY29vbGtpdC5jbjEcMBoGCSqGSIb3\r\n"
"DQEJARYNMjIwMzM1QHFxLmNvbTAgFw0xNzA3MTEwOTUzNTFaGA8yMTE3MDYxNzA5\r\n"
"NTM1MVowejELMAkGA1UEBhMCY2IxCzAJBgNVBAgMAmdkMQswCQYDVQQHDAJzejEQ\r\n"
"MA4GA1UECgwHY29vbGtpdDEMMAoGA1UECwwDZGV2MRMwEQYDVQQDDApjb29sa2l0\r\n"
"LmNuMRwwGgYJKoZIhvcNAQkBFg0yMjAzMzVAcXEuY29tMIGfMA0GCSqGSIb3DQEB\r\n"
"AQUAA4GNADCBiQKBgQDNib2yd5iOrhUahGb9YPxVXJU16uBIFMgbTlfJu0JzxdOk\r\n"
"Ejt0i3+6Ijz6ISmNY+0/ojOLlXO7qPmBDl/DQtn0faigzVOtJFJZdNaiAnUkGVvp\r\n"
"/4RCIhdmHVXj3fL2Ojcuh9ua6k2MaFUIroHiyD6c0Bict8jke1hIEpP8On2anQID\r\n"
"AQABo1AwTjAdBgNVHQ4EFgQUwFM57JgjWg7fzO/tEPjZYdYDz74wHwYDVR0jBBgw\r\n"
"FoAUwFM57JgjWg7fzO/tEPjZYdYDz74wDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B\r\n"
"AQUFAAOBgQC62kzGjskLHLuPY0Em+xl26SQmnx0mJOLgzLx13lpc5xf0vWWSsiI+\r\n"
"IGCA+ybWXavTUZAbJ2waLA5eQJCGgEosnO5Nce3OR3kxfHCdW7k+fVvQqlmU0mQR\r\n"
"K8U0/gOdogOGK7McH+UK4QYjeECcFZp1WD/uinsXg4u2hiuGBw7Dwg==\r\n"
"-----END CERTIFICATE-----\r\n";

const int32_t colink_test_cas_pem_len = sizeof(colink_test_cli_key_rsa);

#endif

ColinkTcpErrorCode colinkGethostbyname(char* hostname, char ip_list[][20], uint8_t num)
{
    struct in_addr sin_addr;

    if (!inet_aton(hostname, &sin_addr)) {
        struct hostent* hp;
        colinkPrintf("%s\n", hostname);
        hp = gethostbyname(hostname);

        if (!hp) {
            return -1;
        }

        int i = 0;

        for (i = 0; (hp->h_addr_list[i] != 0) && (i < num); i++) {
            colinkPrintf("tmp ip = %s\n", inet_ntoa(*((struct in_addr*)hp->h_addr_list[i])));
            memcpy(ip_list[i], inet_ntoa(*((struct in_addr*)hp->h_addr_list[i])),
                   strlen(inet_ntoa(*((struct in_addr*)hp->h_addr_list[i]))));
        }
    }

    return COLINK_TCP_NO_ERROR;
}

#if defined(COLINK_SSL)

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    colinkPrintf("%s:%04d: %s", file, line, str);
}

mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_net_context server_fd;

#if defined(COLINK_VERIFY)
mbedtls_x509_crt cacert;
#endif


int32_t colinkTcpSslConnect(const char* dst, uint16_t port)
{
    int ret, len;
    int i;
    char port_str[8] = "";

    if (NULL == dst)
    {
        return COLINK_TCP_ARG_INVALID;
    }

    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);

#if defined(COLINK_VERIFY)
    mbedtls_x509_crt_init( &cacert );
#endif

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     NULL, 0)) != 0)
    {
        return COLINK_TCP_CREATE_CONNECT_ERR;
    }

#if defined(COLINK_VERIFY)
    ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *)colink_test_cli_key_rsa,
                                   colink_test_cas_pem_len );
 
    if (ret < 0)
    {
        printf("\r\n mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
    }
#endif

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        printf("mbedtls_ssl_config_defaults returned %d", ret);
        return COLINK_TCP_CREATE_CONNECT_ERR;
    }

#if defined(COLINK_VERIFY)
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED/*MBEDTLS_SSL_VERIFY_OPTIONAL*/);
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
#else

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);

#endif

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_read_timeout(&conf, 10);
    mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        printf("mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        return COLINK_TCP_CREATE_CONNECT_ERR;
    }

    struct sockaddr_in servaddr;
    int flags;
    int reuse = 1;

    server_fd.fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd.fd < 0)
    {
        colinkPrintf("ssl creat socket fd failed\n");
        return COLINK_TCP_CREATE_CONNECT_ERR;
    }

    flags = fcntl(server_fd.fd, F_GETFL, 0);

    if (flags < 0 || fcntl(server_fd.fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        colinkPrintf("ssl fcntl: %s\n", strerror(errno));
        close(server_fd.fd);
        return COLINK_TCP_CREATE_CONNECT_ERR;
    }

    if (setsockopt(server_fd.fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char*) &reuse, sizeof(reuse)) != 0)
    {
        close(server_fd.fd);
        colinkPrintf("ssl set SO_REUSEADDR failed\n");
        return COLINK_TCP_CREATE_CONNECT_ERR;
    } 

    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(dst);
    servaddr.sin_port = htons(port);

    if (connect(server_fd.fd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) == 0)
    {
        colinkPrintf("ssl dst %s errno %d\n", dst, errno);
    }
    else
    {
        colinkPrintf("ssl dst %s errno %d\n", dst, errno);

        if (errno == EINPROGRESS)
        {
            colinkPrintf("ssl tcp conncet noblock\n");
        }
        else
        {
            close(server_fd.fd);
            return COLINK_TCP_CREATE_CONNECT_ERR;
        }
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    return (int32_t)&ssl;
}

void colinkTcpSslDisconnect(int32_t fd)
{
    mbedtls_ssl_context *pssl = (mbedtls_ssl_context *)fd;

    if (NULL == pssl)
    {
        colinkPrintf("colinkTcpSslDisconnect ????????\r\n");
    }

    mbedtls_ssl_close_notify(pssl);

    mbedtls_net_free((mbedtls_net_context*)(pssl->p_bio));

#if defined(COLINK_VERIFY)

        mbedtls_x509_crt_free( &cacert );

#endif

    mbedtls_ssl_free( pssl);
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    printf("colinkTcpSslDisconnect\r\n");
}

ColinkTcpErrorCode colinkTcpSslState(int32_t fd)
{
    int errcode = 0;
    mbedtls_ssl_context *pssl = (mbedtls_ssl_context *)fd;
    int tcp_fd = ((mbedtls_net_context*)(pssl->p_bio))->fd;
    int ret;

    if (tcp_fd < 0 || NULL == pssl) 
    {
        return COLINK_TCP_ARG_INVALID;
    }

    fd_set rset, wset;
    int ready_n;

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_CLR(tcp_fd, &rset);
    FD_CLR(tcp_fd, &wset);
    FD_SET(tcp_fd, &rset);
    FD_SET(tcp_fd, &wset);

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    ready_n = select(tcp_fd + 1, &rset, &wset, NULL, &timeout);

    if (0 == ready_n)
    {
        errcode = COLINK_TCP_CONNECTING;
    }
    else if (ready_n < 0)
    {
        errcode = COLINK_TCP_CONNECT_ERR;
    }
    else
    {
        if (FD_ISSET(tcp_fd, &wset) != 0)
        {
            colinkPrintf("ssl COLINK_TCP_CONNECTING \n");
            errcode = COLINK_TCP_CONNECTING;

            if(pssl->state != MBEDTLS_SSL_HANDSHAKE_OVER)
            {
                ret = mbedtls_ssl_handshake_step( pssl );
                colinkPrintf("mbedtls_ssl_handshake_step return = 0X%X\r\n", -ret);

                if (0 != ret)
                {
                    errcode = COLINK_TCP_CONNECT_ERR;
                }
            }
            else
            {
                errcode = COLINK_TCP_NO_ERROR;
            }
        }
        else
        {
            int len = (int) sizeof(int);;

            if (0 != getsockopt(tcp_fd, SOL_SOCKET, SO_ERROR, &ret, &len))
            {
                errcode = COLINK_TCP_CONNECT_ERR;
            }

            if (0 != ret)
            {
                errcode = COLINK_TCP_CONNECT_ERR;
            }

            errcode = COLINK_TCP_CONNECT_ERR;
        }
    }

    colinkPrintf("colinkTcpState errcode=%d\n", errcode);

    return errcode;
}

int32_t colinkTcpSslSend(int32_t fd, const uint8_t* buf, uint16_t len)
{
    int ret = 0;
    mbedtls_ssl_context *pssl = (mbedtls_ssl_context *)fd;

    if (NULL == buf || NULL == pssl)
    {
        return COLINK_TCP_ARG_INVALID;
    }

    ret = mbedtls_ssl_write(pssl, buf, len);
    
    if(ret > 0)
    {
        return ret;
    }
    else if(MBEDTLS_ERR_SSL_TIMEOUT == ret)
    {
        colinkPrintf("colinkTcpSslSend timeout ret = 0X%X\r\n",-MBEDTLS_ERR_SSL_TIMEOUT);
        return 0;
    }
    else
    {
        colinkPrintf("colinkTcpSslsend error\r\n");
        return COLINK_TCP_SEND_ERR;
    }

    return ret;
}

int32_t colinkTcpSslRead(int32_t fd, uint8_t* buf, uint16_t len)
{
    int ret = 0;
    mbedtls_ssl_context *pssl = (mbedtls_ssl_context *)fd;

    if (NULL == buf || NULL == pssl)
    {
        return COLINK_TCP_ARG_INVALID;
    }

    ret = mbedtls_ssl_read(pssl, buf, len);

    if(ret > 0)
    {
        return ret;
    }
    else if(MBEDTLS_ERR_SSL_TIMEOUT == ret)
    {
        return 0;
    }
    else
    {
        colinkPrintf("colinkTcpSslRead ret = 0X%X\r\n",-ret);
        return COLINK_TCP_READ_ERR;
    }
}

#endif /**< #if defined(COLINK_SSL) */

int32_t colinkTcpRead(int32_t fd, uint8_t* buf, uint16_t len)
{
#if defined(COLINK_SSL)
    return colinkTcpSslRead(fd,buf,len);
#else

    int ret = -1;

    if (buf == NULL)
    {
        return COLINK_ARG_INVALID;
    }

    ret = (int)(recv(fd, buf, len, MSG_DONTWAIT));

    if (ret <= 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return COLINK_NO_ERROR;
        }
        else
        {
            printf("ret=%d errno=%d\n", ret, errno);
            return COLINK_TCP_READ_ERR;
        }
    }
    return ret;

#endif
}

int32_t colinkTcpSend(int32_t fd, const uint8_t* buf, uint16_t len)
{
#if defined(COLINK_SSL)
    return colinkTcpSslSend(fd,buf,len);
#else

    int ret = -1;

    if (buf == NULL)
    {
        return COLINK_ARG_INVALID;
    }

    ret = (int)(send(fd, buf, len, MSG_DONTWAIT));

    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return COLINK_NO_ERROR;
        }
        else
        {
            printf("ret=%d errno=%d\n", ret, errno);
            return COLINK_TCP_SEND_ERR;
        }
    }
    return ret;

#endif
}

void colinkTcpDisconnect(int32_t fd)
{
#if defined(COLINK_SSL)
    colinkTcpSslDisconnect(fd);
#else

    close(fd);
#endif
}

ColinkTcpErrorCode colinkTcpState(int32_t fd)
{
#if defined(COLINK_SSL)
    return colinkTcpSslState(fd);
#else
    int errcode = 0;
    int tcp_fd = fd;
    
    if (tcp_fd < 0)
    {
        return COLINK_TCP_CONNECT_ERR;
    }

    fd_set rset, wset;
    int ready_n;

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_CLR(tcp_fd, &rset);
    FD_CLR(tcp_fd, &wset);
    FD_SET(tcp_fd, &rset);
    FD_SET(tcp_fd, &wset);

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    ready_n = select(tcp_fd + 1, &rset, &wset, NULL, &timeout);

    if (0 == ready_n)
    {
        colinkPrintf("select time out\n");
        errcode = COLINK_TCP_CONNECTING;
    }
    else if (ready_n < 0)
    {
        colinkPrintf("select error\n");
        errcode = COLINK_TCP_CONNECT_ERR;
    }
    else
    {
        if (FD_ISSET(tcp_fd, &wset) != 0)
        {
            errcode = COLINK_TCP_NO_ERROR;
        }
        else
        {
            int ret;
            int len = (int) sizeof(int);;

            if (0 != getsockopt(tcp_fd, SOL_SOCKET, SO_ERROR, &ret, &len))
            {
                colinkPrintf("getsocketopt failed\r\n");
                errcode = COLINK_TCP_CONNECT_ERR;
            }

            if (0 != ret)
            {
                errcode = COLINK_TCP_CONNECT_ERR;
            }

            errcode = COLINK_TCP_CONNECT_ERR;
        }
    }

    return errcode;
#endif
}

int32_t colinkTcpConnect(const char* dst, uint16_t port)
{
#if defined(COLINK_SSL)
    return colinkTcpSslConnect(dst,port);
#else

    struct sockaddr_in servaddr;
    int fd;
    int flags;
    int reuse;

    if (NULL == dst)
    {
        return COLINK_TCP_ARG_INVALID;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);


    if (fd < 0) {
        colinkPrintf("creat socket fd failed\n");
        return COLINK_TCP_CONNECT_ERR;
    }

    flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        colinkPrintf("fcntl: %s\n", strerror(errno));
        close(fd);
        return COLINK_TCP_CONNECT_ERR;
    }

    reuse = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char*) &reuse, sizeof(reuse)) != 0)
    {
        close(fd);
        colinkPrintf("set SO_REUSEADDR failed\n");
        return COLINK_TCP_CONNECT_ERR;
    } 

    memset(&servaddr, 0, sizeof(struct sockaddr_in));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(dst);
    servaddr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) == 0)
    {
        colinkPrintf("dst %s errno %d\n", dst, errno);
        return fd;
    }
    else
    {
        colinkPrintf("dst %s errno %d\n", dst, errno);

        if (errno == EINPROGRESS)
        {
            colinkPrintf("tcp conncet noblock\n");
            return fd;
        }
        else
        {
            close(fd);
            return COLINK_TCP_CONNECT_ERR;
        }
    }
#endif
}
