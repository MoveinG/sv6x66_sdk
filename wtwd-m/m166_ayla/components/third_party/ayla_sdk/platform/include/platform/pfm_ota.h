/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_PFM_COMMON_OTA_H__
#define __AYLA_PFM_COMMON_OTA_H__

/**
 * Structure of platform dependent OTA operation context.
 */
struct pfm_ota_ctx;

/**
 * Open a download image for upgrading.
 *
 * \param size is the image size.
 * \param ver is a string conains the image version.
 *
 * \return a pointer to OTA operation context, or NULL on failure.
 */
struct pfm_ota_ctx *pfm_ota_image_open(size_t size, const char *ver);

/**
 * Write a data block to image file.
 *
 * \param pctx is a pointer to OTA operation context.
 * \param off is the offset that the data block being writen to.
 * \param buf is the address of the data.
 * \param len is the length of the data block.
 *
 * \returns error code, zero on success.
 */
int  pfm_ota_image_write(struct pfm_ota_ctx *pctx, size_t off,
	const void *buf, size_t len);

/**
 * Close the downdoaded image file.
 *
 * \param pctx is a pointer to OTA operation context.
 *
 * \returns error code, zero on success.
 */
int  pfm_ota_image_close(struct pfm_ota_ctx *pctx);

/**
 * Reboot from the downloaded image.
 */
void pfm_ota_image_commit(void);

#endif /* __AYLA_PFM_COMMON_OTA_H__ */
