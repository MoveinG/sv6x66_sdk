/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_rsa.h>
#include "rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/pk.h"
#include "osal.h"

struct al_rsa_ctxt{
	mbedtls_rsa_context rsa;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context pk;
	char *key_buf[1024];
	int key_len;
};

struct al_rsa_ctxt *al_rsa_ctxt_alloc(void)
{
	struct al_rsa_ctxt *ctxt = OS_MemAlloc(sizeof(struct al_rsa_ctxt));
	memset(ctxt,0,sizeof(struct al_rsa_ctxt));
	mbedtls_rsa_init( &ctxt->rsa, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_ctr_drbg_init( &ctxt->ctr_drbg );
    mbedtls_entropy_init( &ctxt->entropy );
    // mbedtls_pk_init( &ctxt->pk );
	return ctxt;
}

void al_rsa_ctxt_free(struct al_rsa_ctxt *ctxt)
{
	mbedtls_ctr_drbg_free( &ctxt->ctr_drbg );
    mbedtls_entropy_free( &ctxt->entropy );
    mbedtls_rsa_free( &ctxt->rsa );
    // mbedtls_pk_free( &ctxt->pk );
	OS_MemFree(ctxt);
}

size_t al_rsa_pub_key_set(struct al_rsa_ctxt *ctxt,
			const void *key, size_t keylen)
{
	unsigned char *p;
	int ret;

	// ASSERT(key);
	// memset(key, 0, sizeof(struct adc_rsa_key));

	ret = mbedtls_pk_setup(&ctxt->pk,
	    mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
	if (ret) {
		goto version_1_5;
	}
	p = (unsigned char *)key;
	ret = pk_get_rsapubkey(&p, p+keylen, mbedtls_pk_rsa(ctxt->pk));
	if (ret == 0) {
		return mbedtls_pk_rsa(ctxt->pk)->len;
	}else{
    }
	mbedtls_pk_free(&ctxt->pk);

version_1_5:
	p = (unsigned char *)key;
	if (mbedtls_pk_parse_subpubkey(&p, p+keylen, &ctxt->pk) == 0 &&
	    mbedtls_pk_can_do(&ctxt->pk, MBEDTLS_PK_RSA)) {
		    return mbedtls_pk_rsa(ctxt->pk)->len;
	}
	return 0;
}

void al_rsa_key_clear(struct al_rsa_ctxt *ctxt)
{
	mbedtls_pk_free( &ctxt->pk );
}

#ifndef MBEDTLS_DRBG_ALT
static int trans_encrypted_poll(void* ctx, unsigned char* output,
        size_t len, size_t* olen) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        output[i] = rand() & 0xFF;
    }

    *olen = len;
    return 0;
}
#endif

ssize_t al_rsa_encrypt_pub(struct al_rsa_ctxt *ctxt,
			const void *in, size_t in_len,
			void *out, size_t out_len)
{
	int return_val, exit_val;
	const char *pers = "rsa";
	int rc = -1;


#ifndef MBEDTLS_DRBG_ALT
    // SUPPRESS_WARNING(trans_encrypted_poll);
    rc = mbedtls_entropy_add_source(&ctxt->entropy, trans_encrypted_poll, NULL, 128, 1);

    if (rc != 0) {
        printf("mbedtls_entropy_add_source failed: %d", rc);
        goto exit;
    }

    rc = mbedtls_ctr_drbg_seed(&ctxt->ctr_drbg,
                               mbedtls_entropy_func,
                               &ctxt->entropy,
                               (const unsigned char*)(pers),
                               strlen(pers));

    if (rc != 0) {
        printf("mbedtls_ctr_drbg_seed failed: %d", rc);
        goto exit;
    }

#endif


    mbedtls_rsa_context *rsa;
	int ret;

	rsa = mbedtls_pk_rsa(ctxt->pk);
	mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, 0);

	ret = mbedtls_rsa_pkcs1_encrypt(rsa, mbedtls_ctr_drbg_random, &ctxt->ctr_drbg,
	    MBEDTLS_RSA_PUBLIC, in_len, in, out);

	if (ret < 0)
		return ret;
	else
		return rsa->len;

exit:
	return -1;
}

ssize_t al_rsa_verify(struct al_rsa_ctxt *ctxt,
			const void *in, size_t in_len,
			void *out, size_t out_len)
{
	mbedtls_rsa_context *rsa;
	int ret;

	rsa = mbedtls_pk_rsa(ctxt->pk);
	mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, 0);

	ret = mbedtls_rsa_pkcs1_decrypt(rsa, NULL, NULL,
	    MBEDTLS_RSA_PUBLIC, &out_len, in, out, out_len);

	if (ret < 0)
		return ret;
	else
		return out_len;
}
