/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <platform/pfm_ota.h>
#include "fsal.h"
#include "osal.h"

#define OTA_BIN_FILENAME       "ota.bin"
#define OTA_INFO_FILENAME     "ota_info.bin"

struct pfm_ota_ctx {
	size_t size;
	size_t rx_len;
	unsigned int crc;
	unsigned char done;

	/* Linux platform dependent code */
	SSV_FILE fp;
	char file[128];
};
extern SSV_FS fs_handle;
struct pfm_ota_ctx ctx;

#define CRC32_INIT 0xffffffffUL

static const u32 crc32_table[16] = {
	0, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
	0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
	0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
};

/*
 * Compute CRC-8 with IEEE polynomial LSB-first. Use 4-bit table.
 */
static u32 pfm_ota_crc32(const void *buf, size_t len, u32 crc)
{
	const u8 *bp = buf;

	while (len-- > 0) {
		crc ^= *bp++;
		crc = (crc >> 4) ^ crc32_table[crc & 0xf];
		crc = (crc >> 4) ^ crc32_table[crc & 0xf];
	}
	return crc;
}

static int ota_create_info_file(void){
	int rlt;
	int info_fd = 0;
	int bin_fd = 0;
	char buf[32];

	memset(buf, 0, sizeof(buf));
  
    info_fd = FS_open(fs_handle, OTA_INFO_FILENAME, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (info_fd < 0)
    {
        printf("info file create error\n");
        rlt = FS_errno(fs_handle);
        printf("rlt = %d\n", rlt);
        return -1;
    }

	bin_fd = FS_open(fs_handle, OTA_BIN_FILENAME, SPIFFS_RDWR, 0);
    if (bin_fd < 0)
    {
        printf("FS_open error\n");
        rlt = FS_errno(fs_handle);
        printf("rlt = %d\n", rlt);
        return -1;
    }

    if (FS_read(fs_handle, bin_fd, buf, 16) < 0)
    {
        printf("read error\n");
        rlt = FS_errno(fs_handle);
        printf("rlt = %d\n", rlt);
        return -1;
    }

    if(FS_write(fs_handle, info_fd, buf, 16) < 0 )
    {
        printf("FS_write error\n");
        rlt = FS_errno(fs_handle);
        printf("rlt = %d\n", rlt);

        return -1;
    }
	
    FS_close(fs_handle, info_fd);
	FS_close(fs_handle, bin_fd);
	FS_list(fs_handle);
    return 0;

}

struct pfm_ota_ctx *pfm_ota_image_open(size_t size, const char *ver)
{

	FS_remove(fs_handle, OTA_BIN_FILENAME);
	ctx.fp = FS_open(fs_handle, OTA_BIN_FILENAME, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	if (ctx.fp < 0) 
	{
		printf("[%s][%d]\n",__func__,__LINE__);
		return NULL;
	}
	return &ctx;
}

int pfm_ota_image_write(struct pfm_ota_ctx *pctx, size_t offset,
	const void *buf, size_t len)
{
	int err;
	size_t rc;
	if (!pctx || offset != pctx->rx_len ||
	    (pctx->rx_len + len) > pctx->size) {
		err = 0x12; //PB_ERR_FATAL;
		goto error;
	}
	pctx->crc = pfm_ota_crc32(buf, len, pctx->crc);
	rc = FS_write(fs_handle,pctx->fp,buf,len);
	if (rc != len) {
		err = 0x21; //PB_ERR_WRITE;
		goto error;
	}
	pctx->rx_len += len;
	return 0;
error:
	FS_close(fs_handle, pctx->fp);
	pctx->fp = NULL;
	pctx->file[0] = '\0';
	return err;
}

int pfm_ota_image_close(struct pfm_ota_ctx *pctx)
{
	FS_close(fs_handle, pctx->fp);
	return 0;
}

void pfm_ota_image_commit(void)
{
	ota_create_info_file();
	al_os_reboot();
}
