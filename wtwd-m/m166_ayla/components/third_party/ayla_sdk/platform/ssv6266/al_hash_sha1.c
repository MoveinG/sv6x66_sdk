/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_hash_sha1.h>
#include "osal.h"
#include "sha1.h"

struct al_hash_sha1_ctxt {
	mbedtls_sha1_context ctx;
};

struct al_hash_sha1_ctxt *al_hash_sha1_ctxt_alloc(void)
{
	return OS_MemAlloc(sizeof(struct al_hash_sha1_ctxt));
}

void al_hash_sha1_ctxt_free(struct al_hash_sha1_ctxt *ctxt)
{
	OS_MemFree(ctxt);
}

void al_hash_sha1_ctxt_init(struct al_hash_sha1_ctxt *ctxt)
{
	mbedtls_sha1_init(&ctxt->ctx);
	mbedtls_sha1_starts(&ctxt->ctx);
}

void al_hash_sha1_add(struct al_hash_sha1_ctxt *ctxt,
			const void *buf, size_t len)
{
	
	mbedtls_sha1_update(&ctxt->ctx,buf,len);
}

void al_hash_sha1_final(struct al_hash_sha1_ctxt *ctxt, void *buf)
{
	mbedtls_sha1_finish(&ctxt->ctx,buf);
	mbedtls_sha1_free(&ctxt->ctx);
}
