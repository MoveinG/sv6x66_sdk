/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "osal.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"

#include "lwip/sockets.h"

#include <ayla/assert.h>
#include <ayla/log.h>
#include <al/al_net_stream.h>
#include <al/al_os_mem.h>
#include <platform/pfm_net_tls.h>

#define TLS_USE_MUTEX 

#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
#else
#define ERR_remove_thread_state(p)
#define CRYPTO_malloc_init()	OPENSSL_malloc_init()
#endif


/* Macros */
#undef BUFSIZZ
#define BUFSIZZ (1024*8)

#define TLS_PRINT   printf

/* #define SRP_NUMBER_ITERATIONS_FOR_PRIME  64 */

/* Local globals */
#ifdef TLS_USE_MUTEX
static OsMutex gv_tls_mutex = -1;
#endif


static char *psk_identity = "Client_identity";
/*--------------------------------------------------------------------*
 * Ayla SSL client
 *--------------------------------------------------------------------*/

#define TLS_ERR_OK	0
#define TLS_ERR_END	-1
#define TLS_ERR_SHUT	-2
#define TLS_ERR_RESTART -3

struct pfm_net_tls {
	int state;	/* state */
	int s;		/* socket */
	int socket_type;
	mbedtls_ssl_context ssl;  /* SSL context */
	int in_init;	/* in handshaking */
    int ca_certs_inited;

	mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    mbedtls_ssl_config conf;
    mbedtls_x509_crt *cacert;
    mbedtls_timing_delay_context timer;
};


int wrapper_send(struct pfm_net_tls *pctx, const void *buf, int size)
{
    
    int rs = -1;
    fd_set fdw;
    struct timeval tv;

    if (!pctx || pctx->s == -1) {
        return rs;
    }
	int fd = pctx->s;
    FD_ZERO(&fdw);
    FD_SET(fd, &fdw);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    rs = select(fd + 1, NULL, &fdw, NULL, &tv);
    if (FD_ISSET(fd, &fdw)) {
        rs = send(fd, buf, size, 0);
    } else if (rs == 0) {
        //need send aganin
    } else if (errno != EINTR) {
        rs = -1;
    }

    if (rs < 0 ) {
        printf("write socket error %d:%s", errno, strerror(errno));
    }

    return rs > 0 ? size : rs;
}

int wrapper_recv(struct pfm_net_tls *pctx, const void *buf, int size)
{
    int rs = -1;
    fd_set fdr;
    struct timeval tv;

    if (!pctx || pctx->s == -1) {
        return rs;
    }
	int fd = pctx->s;
    FD_ZERO(&fdr);
    FD_SET(fd, &fdr);

    tv.tv_sec = 3;
    tv.tv_usec = 0;

    rs = select(fd + 1, &fdr, NULL, NULL, &tv);
    if (FD_ISSET(fd, &fdr)) {
        rs = recv(fd, buf, size, 0);
        if (rs <= 0) {
            printf("Can't reach remote host!!! rs = %d", rs);
            rs = -1;
        }
    } else if (rs == 0) {
		printf("mbed tls hand recv timeout\n");
        rs = 0;
    } else if (errno != EINTR) {
		printf("mbed tls hand recv EINTR\n");
        rs = -1;
    }

    if (rs < 0 ) {
        printf("read socket error %d:%s", errno, strerror(errno));
    }

    return rs;
}

static int trans_encrypted_poll(void* ctx, unsigned char* output,
        size_t len, size_t* olen) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        output[i] = rand() & 0xFF;
    }

    *olen = len;
    return 0;
}

static int stream_ssl_verify(struct stream_pcb *sp,
	mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
	return 0;
}


static int ssl_client_init_setup(struct pfm_net_tls *pctx)
{
	int ret, len;
    mbedtls_ssl_init( &(pctx->ssl) );
    mbedtls_ssl_config_init( &(pctx->conf) );
    mbedtls_ctr_drbg_init( &(pctx->ctr_drbg) );
	mbedtls_entropy_init( &(pctx->entropy) );

    pctx->cacert = load_ca_cert();
	
#ifndef MBEDTLS_DRBG_ALT
    // SUPPRESS_WARNING(trans_encrypted_poll);
    ret = mbedtls_entropy_add_source(&pctx->entropy, trans_encrypted_poll, NULL, 128, 1);

    if (ret != 0) {
        printf("mbedtls_entropy_add_source failed: %d", ret);
        goto error;
    }

    ret = mbedtls_ctr_drbg_seed(&pctx->ctr_drbg,
                               mbedtls_entropy_func,
                               &pctx->entropy,
                               (const unsigned char*)(psk_identity),
                               strlen(psk_identity));

    if (ret != 0) {
        TLS_PRINT("mbedtls_ctr_drbg_seed failed: %d", ret);
        goto error;
    }

#endif

	mbedtls_ssl_set_bio(&pctx->ssl, pctx, wrapper_send,wrapper_recv, NULL); /* RTLTODO: timeout */

	ret = mbedtls_ssl_config_defaults(&pctx->conf,
                                     MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_STREAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT);

    if (ret != 0) {
        TLS_PRINT("mbedtls_ssl_config_defaults failed: %d", ret);
        goto error;
    }

	mbedtls_ssl_conf_ca_chain(&pctx->conf, pctx->cacert, NULL);
	mbedtls_ssl_conf_authmode(&pctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_verify(&pctx->conf,stream_ssl_verify, NULL);
	mbedtls_ssl_set_hostname(&pctx->ssl, "*.sunseaiot.com");
	// pctx->read_timeout = 10;
	mbedtls_ssl_conf_read_timeout(&pctx->conf, 100000);
#ifndef MBEDTLS_DRBG_ALT
    mbedtls_ssl_conf_rng(&pctx->conf, mbedtls_ctr_drbg_random, &pctx->ctr_drbg);
#else
    mbedtls_ssl_conf_rng(&ptr->ssl_conf, mbedtls_ctr_drbg_random, NULL);
#endif
    ret = mbedtls_ssl_setup(&pctx->ssl, &pctx->conf);
	return TLS_ERR_OK;
error:
exit:
end:
	return TLS_ERR_END;
}



int pfm_net_tls_write(struct pfm_net_tls *pctx, const void *buf, int size)
{
	/* returns: zero or positive -- bytes written, negative -- error */
	int k;
#ifdef TLS_USE_MUTEX
	OS_MutexLock(gv_tls_mutex);
#endif
	k = mbedtls_ssl_write(&pctx->ssl, buf, size);
	if(k < 0){
		switch(k){
			case MBEDTLS_ERR_SSL_WANT_WRITE:
			TLS_PRINT("[W] Want write.\n");
			break;

			case MBEDTLS_ERR_SSL_WANT_READ:
				TLS_PRINT( "[W] Want read.\n");
				break;
			default:
				printf("[%d]write:%s\n",size,buf);
				TLS_PRINT("\nmbedtls write other error code:%d\n",k);
				goto shut;
		}
		
	}
	goto last;
shut:
	k = 0;
	goto last;
last:
#ifdef TLS_USE_MUTEX
	OS_MutexUnLock(gv_tls_mutex);
#endif
	return k;
}

int pfm_net_tls_read(struct pfm_net_tls *pctx, void *buf, int size)
{
	/* returns: zero or positive -- bytes read, negative -- error */
	int k;
#ifdef TLS_USE_MUTEX
	OS_MutexLock(&gv_tls_mutex);
#endif
	k = mbedtls_ssl_read(&pctx->ssl, buf, size);  /* BUFSIZZ */
	if(k < 0){
		switch(k){
			case MBEDTLS_ERR_SSL_WANT_WRITE:
				TLS_PRINT("[W] Want write.\n");
				break;

			case MBEDTLS_ERR_SSL_WANT_READ:
				TLS_PRINT("[W] Want read.\n");
				break;
			default:
			
				TLS_PRINT("mbedtls read other error code:%d\n",k);
				goto shut;
		}
	}
	goto last;
shut:
	k = TLS_ERR_SHUT;
	goto last;

last:
#ifdef TLS_USE_MUTEX
	OS_MutexUnLock(&gv_tls_mutex);
#endif
	return k;
}

static void load_root_certs(void)
{
/* Notice:
 * (1). In embedded platform, certifications should be loaded into RAM
 *	on TLS initialization.
 * (2). For PWB-Linux, the TLS is based on OpenSSL.The certificates are
 *	stored in the directory of /etc/ssl/certs. So, before running
 *	the program, you should execute these commands on the command
 *	lines:
 *	cd /etc/ssl
 *	sudo mv certs  certs_old
 *	sudo mkdir certs
 *	sudo cp <root>/ada/libada/certs\*.pem /etc/ssl/certs
 *	sudo c_rehash
 */
}

void pfm_net_tls_init(void)
{
#ifdef TLS_USE_MUTEX
	if(gv_tls_mutex < 0)
		OS_MutexInit(&gv_tls_mutex);
#endif
	// load_root_certs();
}

void pfm_net_tls_final(void)
{
#ifdef TLS_USE_MUTEX
	OS_MutexUnLock(gv_tls_mutex);
	OS_MutexDelete(gv_tls_mutex);
#endif
	
}

struct pfm_net_tls *pfm_net_tls_handshake(int fd,
	const char *hostname, int timeout)
{
	int rc;
	struct pfm_net_tls *pctx;

#ifdef TLS_USE_MUTEX
	pfm_net_tls_init();
	OS_MutexLock(&gv_tls_mutex);
#endif
	pctx = OS_MemAlloc(sizeof(struct pfm_net_tls));
	if (!pctx) {
		goto last;
	}
	pctx->s = fd;
	/* tls->tcp = tcp; */

	rc = ssl_client_init_setup(pctx);
	if (rc)
		goto end;

	int retries = 0;
	while ((rc = mbedtls_ssl_handshake(&pctx->ssl)) != 0) {
			if ((rc != MBEDTLS_ERR_SSL_WANT_READ &&
				rc != MBEDTLS_ERR_SSL_WANT_WRITE &&
				rc != MBEDTLS_ERR_NET_RECV_FAILED) ||
				retries >= 3) {
				printf(
				    "mbedtls_ssl_handshake fail, err=-0x%x",
				    -rc);
				goto end;
			}
			retries++;
		}
    
    
	if(rc == 0){
        printf("SSL Handshake success\n");
    }
	goto last;
end:
	pfm_net_tls_free(pctx);
	pctx = NULL;
last:
#ifdef TLS_USE_MUTEX
	OS_MutexUnLock(&gv_tls_mutex);
#endif
	return pctx;
}

void pfm_net_tls_free(struct pfm_net_tls *pctx)
{
	if (!pctx) {
		return;
	}
#ifdef TLS_USE_MUTEX
	OS_MutexLock(&gv_tls_mutex);
#endif
	// mbedtls_net_free( pctx-> );

    free_ca_cert();
    mbedtls_ssl_free( &pctx->ssl );
    mbedtls_ssl_config_free( &pctx->conf );
    mbedtls_ctr_drbg_free( &pctx->ctr_drbg );
    mbedtls_entropy_free( &pctx->entropy );
#ifdef TLS_USE_MUTEX
	OS_MutexUnLock(&gv_tls_mutex);
#endif
	free(pctx);
}

int pfm_net_tls_get_link_status(struct pfm_net_tls *pctx)
{
	int ret;
#ifdef TLS_USE_MUTEX
	OS_MutexLock(&gv_tls_mutex);
#endif
	ret = AL_ERR_OK;


#ifdef TLS_USE_MUTEX
	OS_MutexUnLock(&gv_tls_mutex);
#endif
	return ret;
}

/* End of the file */
