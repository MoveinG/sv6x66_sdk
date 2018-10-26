/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_hash_sha256.h>
#include "sha256.h"

struct al_hash_sha256_ctxt {
	mbedtls_sha256_context ctx;
};

struct al_hash_sha256_ctxt *al_hash_sha256_ctxt_alloc(void)
{
	return OS_MemAlloc(sizeof(struct al_hash_sha256_ctxt));
}

void al_hash_sha256_ctxt_free(struct al_hash_sha256_ctxt *ctxt)
{
	OS_MemFree(ctxt);
}

void al_hash_sha256_ctxt_init(struct al_hash_sha256_ctxt *ctxt)
{
	mbedtls_sha256_init(&ctxt->ctx);
	mbedtls_sha256_starts(&ctxt->ctx, 0);
}

void al_hash_sha256_add(struct al_hash_sha256_ctxt *ctxt,
			const void *buf, size_t len)
{
	mbedtls_sha256_update(&ctxt->ctx,buf,len);
}

void al_hash_sha256_final(struct al_hash_sha256_ctxt *ctxt, void *buf)
{
	mbedtls_sha256_finish(&ctxt->ctx,buf);
	mbedtls_sha256_free(&ctxt->ctx);
}
