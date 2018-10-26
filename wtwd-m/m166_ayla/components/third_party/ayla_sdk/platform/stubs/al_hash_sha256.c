/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_hash_sha256.h>

struct al_hash_sha256_ctxt *al_hash_sha256_ctxt_alloc(void)
{
	return 0;
}

void al_hash_sha256_ctxt_free(struct al_hash_sha256_ctxt *ctxt)
{
}

void al_hash_sha256_ctxt_init(struct al_hash_sha256_ctxt *ctxt)
{
}

void al_hash_sha256_add(struct al_hash_sha256_ctxt *ctxt,
			const void *buf, size_t len)
{
}

void al_hash_sha256_final(struct al_hash_sha256_ctxt *ctxt, void *buf)
{
}
