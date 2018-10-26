/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_DEMO_H__
#define __AYLA_DEMO_H__

/*
 * Initialize a demo.
 */
void apptest_demo_init(void);
void apptest_demo_ota_init(void);

#ifdef ADA_BUILD_FINAL
/*
 * Finalize the demo.
 */
void apptest_demo_final(void);
#endif

extern const char mod_sw_build[];
extern const char mod_sw_version[];

extern int demo_props_num;
extern struct ada_sprop demo_props[];

#endif /* __AYLA_DEMO_H__ */
