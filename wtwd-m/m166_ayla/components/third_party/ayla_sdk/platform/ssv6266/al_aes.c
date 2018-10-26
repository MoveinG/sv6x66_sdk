/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_aes.h>
#include "aes.h"
#include "osal.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define AES_KET_BITS	(AES_BLOCK_SIZE * 8)

struct al_aes_ctxt {
	unsigned char iv[16];
	mbedtls_aes_context ctxt;
};

struct al_aes_ctxt *al_aes_ctxt_alloc(void)
{
	return OS_MemAlloc(sizeof(struct al_aes_ctxt));
}

void al_aes_ctxt_free(struct al_aes_ctxt *ctxt)
{
	OS_MemFree(ctxt);
}

int al_aes_cbc_key_set(struct al_aes_ctxt *ctxt,
		void *key, size_t key_len, void *iv, int decrypt)
{
	mbedtls_aes_init(&ctxt->ctxt);
	if (decrypt) {
		mbedtls_aes_setkey_dec(&ctxt->ctxt, key, key_len * 8);
	} else {
		mbedtls_aes_setkey_enc(&ctxt->ctxt, key, key_len * 8);
	}
	memcpy(ctxt->iv,iv,sizeof(ctxt->iv));
	return 0;
}

int al_aes_iv_get(struct al_aes_ctxt *ctxt, void *buf, size_t len)
{
	if (len > sizeof(ctxt->iv)) {
		len = sizeof(ctxt->iv);
	}
	memcpy(buf, ctxt->iv, len);
	return 0;
}

int al_aes_cbc_encrypt(struct al_aes_ctxt *ctxt,
			const void *in, void *out, size_t len)
{
	mbedtls_aes_crypt_cbc(&ctxt->ctxt, MBEDTLS_AES_ENCRYPT,
	    len, ctxt->iv, in, out);

	return 0;
}

int al_aes_cbc_decrypt(struct al_aes_ctxt *ctxt,
			const void *in, void *out, size_t len)
{
	mbedtls_aes_crypt_cbc(&ctxt->ctxt, MBEDTLS_AES_DECRYPT,
	    len, ctxt->iv, in, out);
	return 0;
}
