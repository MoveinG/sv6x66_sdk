/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_CONF_H__
#define __AYLA_CONF_H__

#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/tlv.h>
#include <ayla/conf_token.h>

enum conf_error {
	CONF_ERR_NONE,
	CONF_ERR_LEN,
	CONF_ERR_UTF8,
	CONF_ERR_PATH,		/* unsupported path */
	CONF_ERR_TYPE,		/* unsupported type */
	CONF_ERR_RANGE,		/* value out of range */
	CONF_ERR_PERM,		/* access permission */
};

#define CONF_MODEL_MAX		24	/* max string length for device ID */
#define CONF_PATH_MAX		16	/* maximum depth of config path */
#define CONF_VAL_MAX		400	/* maximum length of value TLV */

#define CONF_REG_FLAG_READONLY	0x0001
#define CONF_REG_FLAG_SHOWHIDE	0x0002

#define CONF_COMMIT_RET_ERROR	0x0001
#define CONF_COMMIT_RET_REBOOT	0x0002

/*
 * Output conf log information.
 */
void conf_log(const char *fmt, ...);

/*
 * Initialize conf environment
 */
int conf_init(void);

/*
 * Initialize log conf environment.
 */
int log_conf_init(void);

#ifdef ADA_BUILD_FINAL
/*
 * Finalize conf environment
 */
void conf_final(void);
#endif

/*
 * Get name string of a token
 */
const char *conf_string(enum conf_token token);

/*
 * Parse a name string to the token
 */
enum conf_token conf_token_parse(const char *name, size_t len);

/*
 * Parse a path string to the token array
 */
int conf_str_to_tokens(const char *path, enum conf_token *token, int ntoken);

/*
 * Register a conf entry
 */
void conf_register(const char *persist_name, int (*persist_enable)(void),
		u16 flags, enum ayla_tlv_type type, void *data, size_t len,
		int (*commit)(int from_ui), int ntoken, ...);

/*
 * Reset commit collections of changed items. The collection has the persistent
 * group names and the commit callback handlers. The conf_set_xxx() serial
 * function puts changed items into the collections. conf_commit() will perform
 * the commit handler's action, and save data to the persistent storage.
 */
int conf_commit_start(void);

/*
 * Perform the commit action and save data to persistent storage
 * from_ui is passed to callback handler.
 * persist decides whether persit data into persistent storeage
 *
 * The return value is a OR operation of each commit handler return value,
 * the commit handler returns zero or the OR operation value of series macro
 * CONF_COMMIT_RET_xxx.
 */
int conf_commit(int from_ui, int persist);

/*
 * Persist conf entries by group name.
 */
enum conf_error conf_save(const char *name);

/*
 * Load conf entries by group name from persistent storage.
 */
enum conf_error conf_load(const char *name);

/*
 * Call commit callback handlers.
 */
int conf_set(const char *name, const char *val);

/*
 * Set conf string.
 */
int conf_set_str(enum conf_token *token, int ntoken, const char *str);

/*
 * Set conf unsigned int.
 */
int conf_set_uint(enum conf_token *token, int ntoken, u32 val);

/*
 * Set conf signed int.
 */
int conf_set_int(enum conf_token *token, int ntoken, s32 val);

/*
 * Get conf string.
 */
const char *conf_get_str(enum conf_token *token, int ntoken);
const char *conf_get_str_v(int ntoken, ...);

/*
 * Get conf unsigned int.
 */
u32 conf_get_uint(enum conf_token *token, int ntoken);
u32 conf_get_uint_v(int ntoken, ...);

/*
 * Get conf signed int.
 */
s32 conf_get_int(enum conf_token *token, int ntoken);
s32 conf_get_int_v(int ntoken, ...);

/*
 * conf CLI command
 */
extern const char conf_cli_help[];
void conf_cli(int argc, char **argv);

#endif /* __AYLA_CONF_H__ */
