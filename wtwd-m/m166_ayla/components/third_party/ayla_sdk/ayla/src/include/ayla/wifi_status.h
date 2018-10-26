/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_WIFI_STATUS_H__
#define __AYLA_WIFI_STATUS_H__

/*
 * Get URI-encoded currently-connected SSID.
 */
int wifi_curr_ssid_uri(char *buf, size_t);

#define SSID_URI_LEN	(32 * 3 + 1)	/* max URI-encoded SSID length */

/*
 * Return non-zero if Wi-Fi is in AP mode.
 */
int wifi_in_ap_mode(void);

/*
 * Show recent Wi-Fi connection history.
 */
void wifi_show_hist(int to_log);

#endif /* __AYLA_WIFI_STATUS_H__ */
