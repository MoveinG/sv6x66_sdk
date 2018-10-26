/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_NOTIFY_INT_H__
#define __AYLA_NOTIFY_INT_H__

enum notify_event {
	NS_EV_CHECK,	/* check for properties, also note service up */
	NS_EV_DOWN,	/* notification service went down */
	NS_EV_DOWN_RETRY, /* notification service went down, will retry */
	NS_EV_DNS_PASS,	/* notification service dns lookup passed */
	NS_EV_CHANGE,	/* the notification server may have changed - re-ID */
};

#define NP_KEY_LEN	16	/* max AES crypto key length in 8-bit bytes */

void np_init(void (*cb_func)(void));
void np_final(void);
int  np_start(u8 *cipher_key, size_t len);
enum notify_event np_event_get(void);
void np_stop(void);
void np_status(void);
void np_set_server(const char *name);
int  np_server_host_cmp(const char *name);
void np_retry_server(void);
void np_set_poll_interval(u32 interval);

#endif /* __AYLA_NOTIFY_H_ */
