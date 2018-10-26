/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <platform/pfm_ota.h>

struct pfm_ota_ctx *pfm_ota_image_open(size_t size, const char *ver)
{
	return NULL;
}

int pfm_ota_image_write(struct pfm_ota_ctx *pctx, size_t offset,
	const void *buf, size_t len)
{
	return -1;
}

int pfm_ota_image_close(struct pfm_ota_ctx *pctx)
{
	return 0;
}

void pfm_ota_image_commit(void)
{
}
