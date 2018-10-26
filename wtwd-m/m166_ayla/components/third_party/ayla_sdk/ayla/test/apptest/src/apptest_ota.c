/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ada/libada.h>
#include <ada/client_ota.h>
#include "apptest_demo.h"
#include "apptest.h"

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
static u32 ota_crc32(const void *buf, size_t len, u32 crc)
{
	const u8 *bp = buf;

	while (len-- > 0) {
		crc ^= *bp++;
		crc = (crc >> 4) ^ crc32_table[crc & 0xf];
		crc = (crc >> 4) ^ crc32_table[crc & 0xf];
	}
	return crc;
}

struct demo_ota {
	u32 exp_len;
	u32 rx_len;
	u32 crc;
	FILE *fp;
};

static struct demo_ota demo_ota;

#define OTA_SAVE_TO_FILE  "./pwb_ota_down.bin"

static enum patch_state apptest_demo_ota_notify(unsigned int len,
	const char *ver)
{
	log_put(LOG_DEBUG "OTA notify( len = %u, ver = \"%s\" )\n", len, ver);
	demo_ota.exp_len = len;
	demo_ota.rx_len = 0;
	demo_ota.crc = CRC32_INIT;
	demo_ota.fp = fopen(OTA_SAVE_TO_FILE, "w+b");
	log_put(LOG_DEBUG "fopen( %s ) %s\n", OTA_SAVE_TO_FILE,
	    (demo_ota.fp) ? "Success" : "Fail");
	ada_api_call(ADA_OTA_START, OTA_HOST);
	return PB_DONE;
}

/*
 * This is where the OTA image would normally be saved, section by section.
 * This demo save the OTA image to a file, and checks the offset and length
 * and computes the CRC.
 */
static enum patch_state apptest_demo_ota_save(unsigned int offset,
				const void *buf, size_t len)
{
	struct demo_ota *ota = &demo_ota;

	if (offset != ota->rx_len) {
		log_put(LOG_WARN "OTA save: offset(%u) != rx_len(%lu)",
			offset, ota->rx_len);
		fclose(ota->fp);
		ota->fp = NULL;
		return PB_ERR_FATAL;
	}
	ota->rx_len += len;
	if (ota->rx_len > ota->exp_len) {
		log_put(LOG_WARN "OTA save: rx_len(%lu) > exp_len(%lu)",
		    ota->rx_len, ota->exp_len);
		fclose(ota->fp);
		ota->fp = NULL;
		return PB_ERR_FATAL;
	}
	ota->crc = ota_crc32(buf, len, ota->crc);
	if (ota->fp) {
		fwrite(buf, 1, len, ota->fp);
	}
	return PB_DONE;
}

static void apptest_demo_ota_save_done(void)
{
	struct demo_ota *ota = &demo_ota;

	if (ota->fp) {
		fclose(ota->fp);
		ota->fp = NULL;
	}
	if (ota->rx_len != ota->exp_len) {
		log_put(LOG_WARN "OTA save_done: rx_len(%lu) != exp_len(%lu)"
		    , ota->rx_len, ota->exp_len);
	}
	log_put(LOG_INFO "OTA save_done: len(%lu), crc(%lx)\r\n",
	    ota->rx_len, ota->crc);
	ada_api_call(ADA_OTA_REPORT, OTA_HOST, PB_DONE);
}

static struct ada_ota_ops demo_ota_ops = {
	.notify = apptest_demo_ota_notify,
	.save = apptest_demo_ota_save,
	.save_done = apptest_demo_ota_save_done,
};

void apptest_demo_ota_init(void)
{
	ada_api_call(ADA_OTA_REGISTER, OTA_HOST, &demo_ota_ops);
}
