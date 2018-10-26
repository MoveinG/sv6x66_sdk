/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <al/al_os_reboot.h>
#include <al/al_os_thread.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ada/libada.h>
#include <ada/client_ota.h>
#include <platform/pfm_ota.h>
#include <demo/demo.h>

struct demo_ota {
	size_t exp_len;
	size_t rx_len;
	void *pctx;
};

static struct demo_ota demo_ota;

static enum patch_state demo_ota_notify(unsigned int len, const char *ver)
{
	struct demo_ota *ota = &demo_ota;
	log_put(LOG_DEBUG "OTA notify( len = %u, ver = \"%s\" )\n", len, ver);
	ota->exp_len = len;
	ota->rx_len = 0;
	ota->pctx = pfm_ota_image_open(len, ver);
	if (!ota->pctx) {
		return PB_ERR_ERASE; /* PB_ERR_FATAL */
	}
	ada_ota_start(OTA_HOST);
	return PB_DONE;
}

/*
 * This is where the OTA image would normally be saved, section by section.
 * This demo save the OTA image to a file, and checks the offset and length
 * and computes the CRC.
 */
static enum patch_state demo_ota_save(unsigned int offset,
				const void *buf, size_t len)
{
	struct demo_ota *ota = &demo_ota;

	if (offset != ota->rx_len) {
		log_put(LOG_WARN "OTA save: offset(%u) != rx_len(%u)",
			offset, ota->rx_len);
		pfm_ota_image_close(ota->pctx);
		ota->pctx = NULL;
		return PB_ERR_FATAL;
	}
	ota->rx_len += len;
	if (ota->rx_len > ota->exp_len) {
		log_put(LOG_WARN "OTA save: rx_len(%u) > exp_len(%u)",
		    ota->rx_len, ota->exp_len);
		pfm_ota_image_close(ota->pctx);
		ota->pctx = NULL;
		return PB_ERR_FATAL;
	}
	if (pfm_ota_image_write(ota->pctx, offset, buf, len)) {
		pfm_ota_image_close(ota->pctx);
		log_put(LOG_WARN "OTA save: error");
		return PB_ERR_WRITE;
	}
	return PB_DONE;
}

static void demo_ota_save_done(void)
{
	struct demo_ota *ota = &demo_ota;
	enum patch_state status;
	log_put(LOG_WARN "OTA done");
	status = pfm_ota_image_close(ota->pctx);
	switch (status) {
	case PB_DONE:
		log_put(LOG_INFO "OTA save_done: Success. Rebooting...");
		break;
	case PB_ERR_FILE_CRC:
		log_put(LOG_INFO "OTA save_done: File CRC Error");
		break;
	case PB_ERR_FATAL:
	default:
		log_put(LOG_INFO "OTA save_done: Error");
		break;
	}
	ada_ota_report(OTA_HOST, status);
	if (status == PB_DONE) {
		al_os_thread_sleep(200);
		pfm_ota_image_commit(); /* reboot */
	}
}

static struct ada_ota_ops demo_ota_ops = {
	.notify = demo_ota_notify,
	.save = demo_ota_save,
	.save_done = demo_ota_save_done,
};

void demo_ota_init(void)
{
	ada_ota_register(OTA_HOST, &demo_ota_ops);
}
