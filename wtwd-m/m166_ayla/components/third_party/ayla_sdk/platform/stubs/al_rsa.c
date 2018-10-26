/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_rsa.h>

struct al_rsa_ctxt *al_rsa_ctxt_alloc(void)
{
	return 0;
}

void al_rsa_ctxt_free(struct al_rsa_ctxt *ctxt)
{
}

size_t al_rsa_pub_key_set(struct al_rsa_ctxt *ctxt,
			const void *key, size_t keylen)
{
	return 0;
}

void al_rsa_key_clear(struct al_rsa_ctxt *ctxt)
{
}

ssize_t al_rsa_encrypt_pub(struct al_rsa_ctxt *ctxt,
			const void *in, size_t in_len,
			void *out, size_t out_len)
{
	return -1;
}

ssize_t al_rsa_verify(struct al_rsa_ctxt *ctxt,
			const void *in, size_t in_len,
			void *out, size_t out_len)
{
	return -1;
}
