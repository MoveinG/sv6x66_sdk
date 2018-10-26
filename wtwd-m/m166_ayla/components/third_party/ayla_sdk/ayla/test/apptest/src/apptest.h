/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_APPTEST_H__
#define __AYLA_APPTEST_H__

#define API_NOT_TESTED -99

enum apitest_struct {
	ADA_SPROP_MGR_REGISTER,
	ADA_SPROP_SEND,
	ADA_SPROP_SEND_BY_NAME,
	ADA_SPROP_SET_BOOL,
	ADA_SPROP_GET_BOOL,
	ADA_SPROP_SET_INT,
	ADA_SPROP_GET_INT,
	ADA_SPROP_SET_UINT,
	ADA_SPROP_GET_UINT,
	ADA_SPROP_SET_STRING,
	ADA_SPROP_GET_STRING,
	ADA_SPROP_DEST_MASK,
	ADA_PROP_MGR_REGISTER,
	ADA_PROP_MGR_READY,
	ADA_PROP_MGR_REQUEST,
	ADA_PROP_MGR_SEND,
	ADA_PROP_SEND_DONE,
	ADA_PROP_CONNECT_STATUS,
	ADA_PROP_RECV,
	ADA_CLIENT_REG_WINDOW_START,
	ADA_CLIENT_INIT,
	ADA_CLIENT_UP,
	ADA_CLIENT_DOWN,
	ADA_SCHED_INIT,
	ADA_SCHED_ENABLE,
	ADA_SCHED_SET_NAME,
	ADA_SCHED_GET_INDEX,
	ADA_SCHED_SET_INDEX,
	ADA_OTA_REGISTER,
	ADA_OTA_START,
	ADA_OTA_REPORT,
	ADA_LOG_INFO,
	ADA_LOG_PUT,
};

/*
 * ADA API test table
 */
struct ada_test_api {
	const char *name;
	unsigned int tc;
	unsigned int cnt;
	int rval;
} PACKED;

/*
 * Test related declarations
 */
extern char demo_host_version[];
extern struct ada_sprop demo_sprops[];
extern int demo_sprops_num;

/*
 * Board button structure
 */
struct TestBut {
	const char *name;
	int val;
	int ctrl;
};

/*
 * API calls wrapper
 */
enum ada_err ada_api_call(enum apitest_struct id, ...);

ssize_t api_sprop_get_bool(struct ada_sprop *sprop, void *buf, size_t len);
ssize_t api_sprop_get_int(struct ada_sprop *sprop, void *buf, size_t len);
ssize_t api_sprop_get_uint(struct ada_sprop *sprop, void *buf, size_t len);
ssize_t api_sprop_get_string(struct ada_sprop *sprop, void *buf, size_t len);
ssize_t api_sprop_dest_mask();

/*
 * Logging for test component
 */
void test_log(const char *fmt, ...);

/*
 * Test CLI
 */
void test_show_cmd(int argc, char **argv);
void test_show_res_cmd(int argc, char **argv);
void test_show_sys_cmd(int argc, char **argv);
void test_show_prop_cmd(int argc, char **argv);
void test_prop_cmd(int argc, char **argv);
void test_prop_get_cmd(int argc, char **argv);
void test_prop_set_cmd(int argc, char **argv);
void test_prop_show_cmd(int argc, char **argv);

/*
 * Searching sprops
 */
struct ada_sprop *sprop_find_by_name(const char *name);

/*
 * Test schedules
 */
void sched_show(int idx);

#endif /* __AYLA_APPTEST_H__ */
