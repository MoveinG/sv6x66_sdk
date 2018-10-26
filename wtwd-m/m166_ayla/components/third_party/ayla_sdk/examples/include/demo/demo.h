/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_DEMO_H__
#define __AYLA_DEMO_H__

/*
 * The function is defined by an app, it is the main entry of the app.
 * The platform calls this after it's initialized.
 */
void ada_demo_main(void);

/*
 * Flag to indicate the ADS connection state
 */
extern int conf_connected;

/*
 * Initialize a demo.
 */
void demo_init(void);
void demo_sched_init(void);
void demo_ota_init(void);

#ifdef ADA_BUILD_FINAL
/*
 * Finalize the demo.
 */
void demo_final(void);
#endif

#endif /* __AYLA_DEMO_H__ */
