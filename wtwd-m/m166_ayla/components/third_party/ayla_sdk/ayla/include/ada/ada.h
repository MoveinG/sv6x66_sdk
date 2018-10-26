/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_ADA_ADA_H__
#define __AYLA_ADA_ADA_H__

#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/conf.h>
#include <ayla/log.h>
#include <ayla/tlv.h>
#include <al/al_clock.h>
#include <ada/err.h>
#include <ada/ada_conf.h>
#include <ada/client.h>
#include <ada/sched.h>
#include <ada/sprop.h>
#include <ada/task_label.h>

/**
 * @file
 * ADA Application Interfaces
 *
 * The file contains ADA APIs for applications
 */

/**
 * Initialize the ADA client environment.
 *
 * \param conf is parameters for the ADA by the app.
 *
 * \return zero on success, -1 on error.
 */
int ada_init(const struct ada_conf *conf);

#ifdef ADA_BUILD_FINAL
/**
 * Finalize the ADA client environment.
 */
void ada_final(void);
#endif

/**
 * Start up the ADA client agent.
 */
int ada_client_up(void);

/**
 * Shut down the ADA client agent.
 */
void ada_client_down(void);

/**
 * Register a entry to conf tree.
 *
 * \param persist_name is the group name to persist.
 * \param persist_enable is a callback to get enable persistent flag.
 * \param flags is the logical or value of macro with the prefix
 *    CONF_REG_FLAG_.
 * \param type is Ayla TLV type.
 * \param data points the data.
 * \param len is the length of data in bytes.
 * \param commit is callback to handle commit events.
 * \param ntoken is the amount of the following token.
 * \param token is config path tokens.
 */
#define ada_conf_register(persist_name, persist_enable, flags, type, data,\
	len, commit, ntoken, token...)				  \
	conf_register(persist_name, persist_enable, flags, type, data,	  \
	len, commit, ntoken, token)					  \

#endif /* __AYLA_ADA_ADA_H__ */
