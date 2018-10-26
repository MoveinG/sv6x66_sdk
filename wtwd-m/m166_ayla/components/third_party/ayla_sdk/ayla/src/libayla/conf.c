/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ayla/log.h>
#include <ayla/conf.h>
#include <ayla/mod_log.h>
#include <al/al_log.h>
#include <al/al_os_mem.h>
#include <al/al_os_reboot.h>
#include <al/al_persist.h>
#include "conf_str.h"

#define CONF_PNAME_TABLE_MAX	10
#define CONF_COMMIT_TABLE_MAX	3
#define CONF_TREE_TABLE_MAX	60
#define CONF_STR_SHOW_MAX	60
#define CONF_BIN_SHOW_MAX	20

struct conf_tree {
	struct conf_tree *siblings;
	struct conf_tree *children;
	enum conf_token token;
	const struct conf_desc *desc;
};

struct conf_desc {
	const char *persist_name;
	int (*persist_enable)(void);
	enum ayla_tlv_type type;
	void *data;
	size_t len;
	int ntoken;
	u16 flags;
	int (*commit)(int from_ui);
	enum conf_token token[1];
};

struct conf_save_state {
	enum conf_error err;
	u8 buf[CONF_VAL_MAX];
	size_t pos;
	const char *name;
};

static const char *conf_pname_table[CONF_PNAME_TABLE_MAX];
static u8 conf_pname_flag[CONF_PNAME_TABLE_MAX];
static int conf_pname_cnt;
static struct conf_tree conf_tree_table[CONF_TREE_TABLE_MAX];
static struct conf_tree *conf_tree_root;
static struct conf_tree *conf_tree_free = conf_tree_table;
static u8 disable_conf_reg;
static int (*conf_commit_table[CONF_COMMIT_TABLE_MAX])(int from_ui);
static int conf_commit_cnt;

static struct conf_tree *conf_search(const enum conf_token *token, int ntoken)
{
	struct conf_tree *node;
	int i;

	node = conf_tree_root;
	for (i = 0; i < ntoken; i++) {
		while (node && node->token != token[i]) {
			node = node->siblings;
		}
		if (!node) {
			break;
		}
		if (i < ntoken - 1) {
			node = node->children;
		}
	}
	return node;
}

static const char *conf_token_name(const enum conf_token *token, int ntoken)
{
	if (ntoken == 3 && token[0] == CT_log && token[1] == CT_mod) {
		return al_log_get_mod_name(token[2]);
	} else {
		return conf_token_names[token[ntoken - 1]];
	}
}

static void conf_walk(void *arg,
	int (*handler)(struct conf_tree **node, int depth, void *arg))
{
	struct conf_tree *node[CONF_PATH_MAX];
	int depth;

	depth = 0;
	node[depth] = conf_tree_root;

	while (depth >= 0) {
		if (node[depth]->desc) {
			if (handler(node, depth + 1, arg)) {
				return;
			}
		}
		if (node[depth]->children) {
			depth++;
			node[depth] = node[depth - 1]->children;
			continue;
		}
		if (node[depth]->siblings) {
			node[depth] = node[depth]->siblings;
			continue;
		}
		do {
			depth--;
		} while (depth >= 0 && !node[depth]->siblings);
		if (depth >= 0) {
			node[depth] = node[depth]->siblings;
		}
	}
}

static u32 conf_node_get_uint(struct conf_tree *node)
{
	if (!node || !node->desc) {
		return 0;
	}
	if (node->desc->type != ATLV_UINT && node->desc->type != ATLV_BOOL) {
		return 0;
	}

	switch (node->desc->len) {
	case sizeof(u8):
		return *(u8 *)node->desc->data;
	case sizeof(u16):
		return *(u16 *)node->desc->data;
	case sizeof(u32):
		return *(u32 *)node->desc->data;
	default:
		return 0;
	}
}

s32 conf_node_get_int(struct conf_tree *node)
{
	if (!node || !node->desc) {
		return 0;
	}
	if (node->desc->type != ATLV_INT) {
		return 0;
	}

	switch (node->desc->len) {
	case sizeof(s8):
		return *(s8 *)node->desc->data;
	case sizeof(s16):
		return *(s16 *)node->desc->data;
	case sizeof(s32):
		return *(s32 *)node->desc->data;
	default:
		return 0;
	}
}

void conf_log(const char *fmt, ...)
{
	ADA_VA_LIST args;

	ADA_VA_START(args, fmt);
	log_put_va(MOD_LOG_CONF, fmt, args);
	ADA_VA_END(args);
}

int conf_init(void)
{
	return 0;
}

static int conf_desc_free(struct conf_tree **node, int depth, void *arg)
{
	if (node[depth - 1]->desc) {
		al_os_mem_free((void *)node[depth - 1]->desc);
		node[depth - 1]->desc = NULL;
	}
	return 0;
}

#ifdef ADA_BUILD_FINAL
void conf_final(void)
{
	conf_walk(NULL, conf_desc_free);
}
#endif

const char *conf_string(enum conf_token token)
{
	if (token >= ARRAY_LEN(conf_token_names)) {
		return NULL;
	}
	return conf_token_names[token];
}

enum conf_token conf_token_parse(const char *name, size_t len)
{
	enum conf_token token;
	int token_len;

	if (len == 0) {
		len = strlen(name);
	}
	for (token = 0; token < ARRAY_LEN(conf_token_names); token++) {
		if (conf_token_names[token]) {
			token_len = strlen(conf_token_names[token]);
			if (token_len == len &&
			    memcmp(conf_token_names[token], name, len) == 0) {
				return token;
			}
		}
	}
	return CT_INVALID_TOKEN;
}

int conf_str_to_tokens(const char *path, enum conf_token *token, int ntoken)
{
	int index = 0;
	char *cp;
	char *errptr;
	unsigned long ulval;
	size_t len;

	while (*path != '\0') {
		if (index >= ntoken) {
			return -1;
		}
		cp = strchr(path, '/');
		if (cp) {
			len = cp - path;
		} else {
			len = 0;
		}
		token[index] = conf_token_parse(path, len);
		if (token[index] == CT_INVALID_TOKEN) {
			ulval = strtoul(path, &errptr, 10);
			if (*errptr != '\0' || ulval > MAX_U8) {
				conf_log(LOG_ERR
				    "bad token for parsing %s", path);
				return -1;
			}
			token[index] = (enum conf_token)ulval;
		}
		index++;
		if (!cp) {
			return index;
		}
		path = cp + 1;
	}

	return index;
}

void conf_register(const char *persist_name, int (*persist_enable)(void),
		u16 flags, enum ayla_tlv_type type, void *data, size_t len,
		int (*commit)(int from_ui), int ntoken, ...)
{
	int i;
	int found;
	struct conf_tree **node;
	enum conf_token token[CONF_PATH_MAX];
	struct conf_desc *desc;
	enum conf_token token_tmp;
	const char *name;
	va_list arg;

	if (disable_conf_reg) {
		conf_log(LOG_ERR "All conf entries must be registered before"
		   " the first sync with flash");
		ASSERT_NOTREACHED();
	}

	if (type != ATLV_UINT && type != ATLV_INT && type != ATLV_UTF8 &&
	    type != ATLV_BOOL && type != ATLV_BIN && type != ATLV_FILE) {
		conf_log(LOG_ERR "Unsupported TLV type!");
		ASSERT_NOTREACHED();
	}
	if (ntoken <= 0) {
		conf_log(LOG_ERR "Invalid conf path depth!");
		ASSERT_NOTREACHED();
	}

	if (persist_name) {
		found = 0;
		for (i = 0; i < conf_pname_cnt; i++) {
			if (conf_pname_table[i] == persist_name) {
				found = 1;
				break;
			}
			if (!strcmp(conf_pname_table[i], persist_name)) {
				found = 1;
				break;
			}
		}
		ASSERT(i < ARRAY_LEN(conf_pname_table));
		if (!found) {
			conf_pname_table[i] = persist_name;
			conf_pname_cnt++;
		}
	}
	va_start(arg, ntoken);
	node = &conf_tree_root;
	for (i = 0; i < ntoken; i++) {
		// token_tmp = va_arg(arg, enum conf_token);  //modify by xuan.ruan
		token_tmp = (enum conf_token)va_arg(arg, int);
		token[i] = token_tmp;
		name = conf_token_name(token, i + 1);
		found = 0;
		while (1) {
			if (*node == NULL) {
				break;
			}
			if ((*node)->token == token_tmp) {
				found = 1;
				break;
			}
			token[i] = (*node)->token;
			if (strcmp(name, conf_token_name(token, i + 1)) < 0) {
				break;
			}
			node = &((*node)->siblings);
		}
		token[i] = token_tmp;
		if (!found) {
			ASSERT(conf_tree_free <
			    &conf_tree_table[ARRAY_LEN(conf_tree_table)]);
			memset(conf_tree_free, 0, sizeof(*conf_tree_free));
			conf_tree_free->siblings = *node;
			*node = conf_tree_free;
			conf_tree_free++;
			(*node)->token = token_tmp;
		}
		if (i < ntoken - 1) {
			node = &((*node)->children);
		}
	}
	va_end(arg);
	desc = al_os_mem_calloc(sizeof(struct conf_desc) +
	    (ntoken - 1) * sizeof(enum conf_token));
	ASSERT(desc);
	desc->persist_name = persist_name;
	desc->persist_enable = persist_enable;
	desc->type = type;
	desc->data = data;
	desc->len = len;
	desc->ntoken = ntoken;
	desc->flags = flags;
	desc->commit = commit;
	memcpy(desc->token, token, ntoken * sizeof(enum conf_token));
	(*node)->desc = desc;
}

static int conf_save_node(struct conf_tree **node, int depth, void *arg)
{
	struct conf_save_state *state = (struct conf_save_state *)arg;
	const struct conf_desc *desc = node[depth - 1]->desc;
	size_t len;

	if (!desc->persist_name || strcmp(state->name, desc->persist_name)) {
		return 0;
	}
	if (desc->persist_enable && !desc->persist_enable()) {
		return 0;
	}

	len = sizeof(state->buf) - state->pos;
	state->err = tlv_import(state->buf + state->pos, &len, ATLV_CONF,
	    desc->token, desc->ntoken * sizeof(desc->token));
	if (state->err) {
		return -1;
	}
	state->pos += len;

	len = sizeof(state->buf) - state->pos;
	state->err = tlv_import(state->buf + state->pos, &len, desc->type,
	    desc->data, desc->len);
	if (state->err) {
		return -1;
	}
	state->pos += len;
	return 0;
}

enum conf_error conf_save(const char *name)
{
	struct conf_save_state state;

	memset(&state, 0, sizeof(state));
	state.name = name;
	conf_walk(&state, conf_save_node);
	if (state.err) {
		return state.err;
	}

	if (al_persist_data_write(AL_PERSIST_STARTUP, name, state.buf,
	    state.pos)) {
		return CONF_ERR_PERM;
	}
	return CONF_ERR_NONE;
}

enum conf_error conf_load(const char *name)
{
	struct conf_tree *node;
	enum conf_error err;
	u8 buf[CONF_VAL_MAX];
	enum conf_token token[CONF_PATH_MAX];
	int i;
	size_t data_len;
	ssize_t pos;
	ssize_t len;

	if (!name && !disable_conf_reg) {
		if (conf_pname_cnt < ARRAY_LEN(conf_pname_table)) {
			conf_log(LOG_WARN
			    "conf_pname_table is occupied %d of %d",
			    conf_pname_cnt, ARRAY_LEN(conf_pname_table));
		}
		i = conf_tree_free - conf_tree_table;
		if (i < ARRAY_LEN(conf_tree_table)) {
			conf_log(LOG_WARN
			    "conf_tree_table is occupied %d of %d",
			    i, ARRAY_LEN(conf_tree_table));
		}
		disable_conf_reg = 1;
	}

	for (i = 0; i < conf_pname_cnt; i++) {
		if (name && strcmp(name, conf_pname_table[i])) {
			continue;
		}
		len = al_persist_data_read(AL_PERSIST_STARTUP,
		    conf_pname_table[i], buf, sizeof(buf));
		if (len < 0) {
			continue;
		}
		pos = 0;
		while (pos < len) {
			data_len = sizeof(token);
			err = tlv_export(ATLV_CONF, token, &data_len,
			    buf + pos, len - pos);
			if (err) {
				return err;
			}
			pos += tlv_info(buf + pos, NULL, NULL, NULL);
			node = conf_search(token,
			    data_len / sizeof(enum conf_token));
			if (!node || !node->desc) {
				break;
			}

			data_len = node->desc->len;
			err = tlv_export(node->desc->type, node->desc->data,
			    &data_len, buf + pos, len - pos);
			if (err) {
				return err;
			}
			pos += tlv_info(buf + pos, NULL, NULL, NULL);
		}
	}
	return CONF_ERR_NONE;
}

static void conf_commit_add_items(struct conf_tree *node)
{
	int i;

	if (!node || !node->desc) {
		return;
	}

	if (node->desc->persist_name) {
		for (i = 0; i < conf_pname_cnt; i++) {
			if (conf_pname_table[i] == node->desc->persist_name ||
			    !strcmp(conf_pname_table[i],
			    node->desc->persist_name)) {
				conf_pname_flag[i] = 1;
				break;
			}
		}
	}

	if (node->desc->commit) {
		for (i = 0; i < conf_commit_cnt; i++) {
			if (conf_commit_table[i] == node->desc->commit) {
				break;
			}
		}
		if (i >= conf_commit_cnt) {
			ASSERT(conf_commit_cnt < ARRAY_LEN(conf_commit_table));
			conf_commit_table[conf_commit_cnt++] =
			    node->desc->commit;
		}
	}
}

int conf_commit_start(void)
{
	int i;

	for (i = 0; i < conf_pname_cnt; i++) {
		conf_pname_flag[i] = 0;
	}
	conf_commit_cnt = 0;

	return 0;
}

int conf_commit(int from_ui, int persist)
{
	int i;
	int ret;

	ret = 0;
	for (i = 0; i < conf_commit_cnt; i++) {
		ret |= conf_commit_table[i](from_ui);
	}

	if (!persist) {
		return ret;
	}
	for (i = 0; i < conf_pname_cnt; i++) {
		if (conf_pname_flag[i]) {
			if (conf_save(conf_pname_table[i])) {
				ret |= CONF_COMMIT_RET_ERROR;
			}
		}
	}

	return ret;
}

int conf_set_str(enum conf_token *token, int ntoken, const char *str)
{
	struct conf_tree *node;
	size_t len;

	node = conf_search(token, ntoken);
	if (!node || !node->desc) {
		return -CONF_ERR_PATH;
	}
	if (node->desc->type != ATLV_UTF8) {
		return -CONF_ERR_TYPE;
	}
	if (node->desc->flags & CONF_REG_FLAG_READONLY) {
		return -CONF_ERR_PERM;
	}

	len = strlen(str);
	if (len + 1 > node->desc->len) {
		return -CONF_ERR_LEN;
	}

	memcpy(node->desc->data, str, len);
	((char *)node->desc->data)[len] = '\0';

	conf_commit_add_items(node);
	return 0;
}

int conf_set_uint(enum conf_token *token, int ntoken, u32 val)
{
	struct conf_tree *node;

	node = conf_search(token, ntoken);
	if (!node || !node->desc) {
		return -CONF_ERR_PATH;
	}
	if (node->desc->type != ATLV_UINT && node->desc->type != ATLV_BOOL) {
		return -CONF_ERR_TYPE;
	}
	if (node->desc->flags & CONF_REG_FLAG_READONLY) {
		return -CONF_ERR_PERM;
	}

	switch (node->desc->len) {
	case sizeof(u8):
		if (val > MAX_U8) {
			return -CONF_ERR_RANGE;
		}
		*(u8 *)node->desc->data = val;
		break;
	case sizeof(u16):
		if (val > MAX_U16) {
			return -CONF_ERR_RANGE;
		}
		*(u16 *)node->desc->data = val;
		break;
	case sizeof(u32):
		*(u32 *)node->desc->data = val;
		break;
	default:
		return -CONF_ERR_LEN;
	}

	conf_commit_add_items(node);
	return 0;
}

int conf_set_int(enum conf_token *token, int ntoken, s32 val)
{
	struct conf_tree *node;

	node = conf_search(token, ntoken);
	if (!node || !node->desc) {
		return -CONF_ERR_PATH;
	}
	if (node->desc->type != ATLV_INT) {
		return -CONF_ERR_TYPE;
	}
	if (node->desc->flags & CONF_REG_FLAG_READONLY) {
		return -CONF_ERR_PERM;
	}

	switch (node->desc->len) {
	case sizeof(s8):
		if (val < MIN_S8 || val > MAX_S8) {
			return -CONF_ERR_RANGE;
		}
		*(s8 *)node->desc->data = val;
		break;
	case sizeof(s16):
		if (val < MIN_S16 || val > MAX_S16) {
			return -CONF_ERR_RANGE;
		}
		*(s16 *)node->desc->data = val;
		break;
	case sizeof(s32):
		*(s32 *)node->desc->data = val;
		break;
	default:
		return -CONF_ERR_LEN;
	}

	conf_commit_add_items(node);
	return 0;
}

int conf_set(const char *name, const char *val)
{
	enum conf_token token[CONF_PATH_MAX];
	int ntoken;
	struct conf_tree *node;
	int err;
	char *errptr;
	u32 uval;
	s32 sval;

	ntoken = conf_str_to_tokens(name, token, ARRAY_LEN(token));
	if (ntoken <= 0) {
		return -CONF_ERR_PATH;
	}
	node = conf_search(token, ntoken);
	if (node == NULL) {
		return -CONF_ERR_PATH;
	}
	switch (node->desc->type) {
	case ATLV_BOOL:
	case ATLV_UINT:
		uval = strtoul(val, &errptr, 10);
		if (*errptr != '\0') {
			return -CONF_ERR_RANGE;
		}
		err = conf_set_uint(token, ntoken, uval);
		if (err) {
			return err;
		}
		break;
	case ATLV_INT:
		sval = strtol(val, &errptr, 10);
		if (*errptr != '\0') {
			return -CONF_ERR_RANGE;
		}
		err = conf_set_int(token, ntoken, sval);
		if (err) {
			return err;
		}
		break;
	case ATLV_UTF8:
		err = conf_set_str(token, ntoken, val);
		if (err) {
			return err;
		}
		break;
	default:
		return -CONF_ERR_TYPE;
	}
	return CONF_ERR_NONE;
}

const char *conf_get_str(enum conf_token *token, int ntoken)
{
	struct conf_tree *node;

	node = conf_search(token, ntoken);
	if (!node || !node->desc || node->desc->type != ATLV_UTF8) {
		return NULL;
	}

	return node->desc->data;
}

const char *conf_get_str_v(int ntoken, ...)
{
	enum conf_token token[CONF_PATH_MAX];
	int i;
	va_list arg;

	if (ntoken > ARRAY_LEN(token)) {
		return NULL;
	}
	va_start(arg, ntoken);
	for (i = 0; i < ntoken; i++) {
		token[i] = va_arg(arg, int);
	}
	va_end(arg);
	return conf_get_str(token, ntoken);
}

u32 conf_get_uint(enum conf_token *token, int ntoken)
{
	return conf_node_get_uint(conf_search(token, ntoken));
}

u32 conf_get_uint_v(int ntoken, ...)
{
	enum conf_token token[CONF_PATH_MAX];
	int i;
	va_list arg;

	if (ntoken > ARRAY_LEN(token)) {
		return 0;
	}
	va_start(arg, ntoken);
	for (i = 0; i < ntoken; i++) {
		token[i] = va_arg(arg, int);
	}
	va_end(arg);
	return conf_node_get_uint(conf_search(token, ntoken));
}

s32 conf_get_int(enum conf_token *token, int ntoken)
{
	return conf_node_get_int(conf_search(token, ntoken));
}

s32 conf_get_int_v(int ntoken, ...)
{
	enum conf_token token[CONF_PATH_MAX];
	int i;
	va_list arg;

	if (ntoken > ARRAY_LEN(token)) {
		return 0;
	}
	va_start(arg, ntoken);
	for (i = 0; i < ntoken; i++) {
		token[i] = va_arg(arg, int);
	}
	va_end(arg);
	return conf_node_get_int(conf_search(token, ntoken));
}

static int conf_show_cal_len(struct conf_tree **node, int depth, void *arg)
{
	enum conf_token token[CONF_PATH_MAX];
	int i;
	int len;

	len = 0;
	for (i = 0; i < depth; i++) {
		token[i] = node[i]->token;
		len += strlen(conf_token_name(token, i + 1));
	}
	len += (depth - 1);
	if (len > *(int *)arg) {
		*(int *)arg = len;
	}
	return 0;
}

static int conf_show_line(struct conf_tree **node, int depth, void *arg)
{
	struct conf_tree *data = node[depth - 1];
	enum conf_token token[CONF_PATH_MAX];
	char line[100];
	int pos = 0;
	int i;
	u32 val;
	s32 sval;
	size_t len;

	if (data->desc->flags & CONF_REG_FLAG_SHOWHIDE) {
		return 0;
	}
	pos = 0;
	for (i = 0; i < depth; i++) {
		if (i != 0) {
			pos += snprintf(line + pos, sizeof(line) - pos, "/");
		}
		token[i] = node[i]->token;
		pos += snprintf(line + pos, sizeof(line) - pos, "%s",
		    conf_token_name(token, i + 1));
	}
	for (; pos < *(int *)arg; pos++) {
		line[pos] = ' ';
	}
	pos += snprintf(line + pos, sizeof(line) - pos, " |");
	if (data->desc) {
		switch (data->desc->type) {
		case ATLV_BOOL:
		case ATLV_UINT:
			val = conf_node_get_uint(data);
			pos += snprintf(line + pos, sizeof(line) - pos,
			    " %lu (0x%lx)", val, val);
			break;
		case ATLV_INT:
			sval = conf_node_get_int(data);
			pos += snprintf(line + pos, sizeof(line) - pos,
			    " %ld (0x%lx)", sval, sval);
			break;
		case ATLV_UTF8:
			len = strlen((const char *)data->desc->data);
			if (len <= CONF_STR_SHOW_MAX) {
				pos += snprintf(line + pos, sizeof(line) - pos,
				    " \"%s\"", (char *)data->desc->data);
			} else {
				line[pos++] = ' ';
				line[pos++] = '"';
				memcpy(line + pos, data->desc->data,
				    CONF_STR_SHOW_MAX / 2 - 1);
				pos += CONF_STR_SHOW_MAX / 2 - 1;
				strcpy(line + pos, "...");
				pos += 3;
				strcpy(line + pos, ((char *)data->desc->data) +
				    len - CONF_STR_SHOW_MAX / 2 + 2);
				pos += CONF_STR_SHOW_MAX / 2 - 2;
				line[pos++] = '"';
				line[pos] = '\0';
			}
			break;
		case ATLV_BIN:
		case ATLV_FILE:
			pos += snprintf(line + pos, sizeof(line) - pos,
			    " [%d] = {", data->desc->len);
			if (data->desc->len <= CONF_BIN_SHOW_MAX) {
				for (i = 0; i < data->desc->len; i++) {
					pos += snprintf(line + pos,
					    sizeof(line) - pos, " %02X",
					    ((u8 *)data->desc->data)[i]);
				}
			} else {
				for (i = 0; i < CONF_BIN_SHOW_MAX / 2 - 1;
				    i++) {
					pos += snprintf(line + pos,
					    sizeof(line) - pos, " %02X",
					    ((u8 *)data->desc->data)[i]);
				}
				pos += snprintf(line + pos,
				    sizeof(line) - pos, " ...");
				for (i = data->desc->len -
				    CONF_BIN_SHOW_MAX / 2 + 1;
				    i < data->desc->len; i++) {
					pos += snprintf(line + pos,
					    sizeof(line) - pos, " %02X",
					    ((u8 *)data->desc->data)[i]);
				}
			}
			pos += snprintf(line + pos, sizeof(line) - pos, " }");
			break;
		default:
			break;
		}
	}

	printcli("%s", line);
	return 0;
}

const char conf_cli_help[] = "show | set <path> <value>";
void conf_cli(int argc, char **argv)
{
	int err;
	int maxlen;

	if (argc == 2 && !strcasecmp(argv[1], "show")) {
		maxlen = 0;
		conf_walk(&maxlen, conf_show_cal_len);
		conf_walk(&maxlen, conf_show_line);
		return;
	} else if (argc == 4 && !strcasecmp(argv[1], "set")) {
		conf_commit_start();
		err = conf_set(argv[2], argv[3]);
		if (err) {
			switch (err) {
			case -CONF_ERR_PATH:
				printcli("Invalid conf path!");
				break;
			case -CONF_ERR_RANGE:
				printcli("Invalid value!");
				break;
			case -CONF_ERR_TYPE:
				printcli("Invalid conf type!");
			default:
				break;
			}
			return;
		}
		if (conf_commit(0, 1) & CONF_COMMIT_RET_REBOOT) {
			al_os_reboot();
		}
		return;
	}

	printcli("Usage:\n"
	    "    conf show\n"
	    "    conf set <path> <value>");
}
