/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_hash_sha1.h>

struct al_hash_sha1_ctxt *al_hash_sha1_ctxt_alloc(void)
{
	return 0;
}

void al_hash_sha1_ctxt_free(struct al_hash_sha1_ctxt *ctxt)
{
}

void al_hash_sha1_ctxt_init(struct al_hash_sha1_ctxt *ctxt)
{
}

void al_hash_sha1_add(struct al_hash_sha1_ctxt *ctxt,
			const void *buf, size_t len)
{
}

void al_hash_sha1_final(struct al_hash_sha1_ctxt *ctxt, void *buf)
{
}
