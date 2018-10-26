/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */

/*
 * Ayla schedule configuration.
 */
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <ada/libada.h>
#include <ada/sched.h>
#include <ayla/nameval.h>
#include <ayla/assert.h>
#include <al/al_persist.h>

#include "apptest.h"
#include "apptest_conf.h"

static const char *sched_conf_scheds[] = {
	"sched1",
};
static u32 sched_saved_run_time;	/* should be in nvram with magic no. */

static int sched_conf_name(char *name, size_t len, unsigned index)
{
	return snprintf(name, len, "sched/n/%u/value", index);
}

void apptest_sched_conf_load(void)
{
	char conf_name[40];
	u8 tlvs[SCHED_TLV_LEN];
	enum ada_err err;
	unsigned int i;
	int rv;

	/*
	 * Create schedules.
	 */
	err = ada_api_call(ADA_SCHED_INIT, ARRAY_LEN(sched_conf_scheds));
	AYLA_ASSERT(!err);

	for (i = 0; i < ARRAY_LEN(sched_conf_scheds); i++) {
		err = ada_api_call(ADA_SCHED_SET_NAME, i, sched_conf_scheds[i]);
		AYLA_ASSERT(!err);

		sched_conf_name(conf_name, sizeof(conf_name), i);
		rv = al_persist_data_read(AL_PERSIST_STARTUP,
			conf_name, tlvs, sizeof(tlvs));
		if (rv < 0) {
			continue;
		}
		err = ada_api_call(ADA_SCHED_SET_INDEX, i, tlvs, (size_t)rv);
		AYLA_ASSERT(!err);
	}
}

/*
 * Save schedules as needed.
 * Read old values first to avoid unnecessary writes.
 */
void adap_sched_conf_persist(void)
{
	char conf_name[40];
	u8 old_tlvs[SCHED_TLV_LEN];
	u8 new_tlvs[SCHED_TLV_LEN];
	size_t new_len;
	char *name;
	enum ada_err err;
	int i;
	int rv;

	for (i = 0; i < ARRAY_LEN(sched_conf_scheds); i++) {
		sched_conf_name(conf_name, sizeof(conf_name), i);

		new_len = sizeof(new_tlvs);
		err = ada_api_call(ADA_SCHED_GET_INDEX, i, &name,
			new_tlvs, &new_len);
		if (err) {
			new_len = 0;
		}

		/*
		 * See if the value has already been saved.
		 */
		rv = al_persist_data_read(AL_PERSIST_STARTUP,
			conf_name, old_tlvs, sizeof(old_tlvs));
		if (rv >= 0 && new_len == (size_t)rv &&
		    !memcmp(old_tlvs, new_tlvs, new_len)) {
			continue;
		}
		rv = al_persist_data_write(AL_PERSIST_STARTUP,
			conf_name, new_tlvs, new_len);
		AYLA_ASSERT(!rv);
	}
}

void adap_sched_run_time_write(u32 run_time)
{
	sched_saved_run_time = run_time;
}

u32 adap_sched_run_time_read(void)
{
	return sched_saved_run_time;
}
