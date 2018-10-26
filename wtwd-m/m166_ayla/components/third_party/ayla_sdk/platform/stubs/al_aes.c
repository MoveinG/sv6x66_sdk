/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_aes.h>

struct al_aes_ctxt *al_aes_ctxt_alloc(void)
{
	return 0;
}

void al_aes_ctxt_free(struct al_aes_ctxt *ctxt)
{
}

int al_aes_cbc_key_set(struct al_aes_ctxt *ctxt,
		void *key, size_t key_len, void *iv, int decrypt)
{
	return -1;
}

int al_aes_iv_get(struct al_aes_ctxt *ctxt, void *buf, size_t len)
{
	return -1;
}

int al_aes_cbc_encrypt(struct al_aes_ctxt *ctxt,
			const void *in, void *out, size_t len)
{
	return -1;
}

int al_aes_cbc_decrypt(struct al_aes_ctxt *ctxt,
			const void *in, void *out, size_t len)
{
	return -1;
}
