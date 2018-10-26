/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include <ayla/log.h>
#include <ayla/ipaddr_fmt.h>
#include <al/al_err.h>
#include <al/al_os_thread.h>
#include <al/al_net_addr.h>
#include <al/al_hash_sha1.h>
#include <al/al_net_stream.h>
#include <al/al_random.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_stream_arg {
	struct al_net_stream *stream;
	enum al_net_stream_type type;
	const char *hostname;
	struct al_net_addr *host_addr;
	u16 port;
	const void *data;
	size_t len;
	void *arg;
	int overwrite_stream;

	enum al_err (*connected)(void *arg, struct al_net_stream *stream,
			enum al_err err);
	enum al_err (*recv_cb)(void *arg, struct al_net_stream *stream,
		void *data, size_t size);
	void(*send_cb)(void *arg, struct al_net_stream *stream,
		size_t len_sent);
	void (*err_cb)(void *arg, enum al_err err);

	struct al_hash_sha1_ctxt *send_ctxt;
	struct al_hash_sha1_ctxt *recv_ctxt;
	int count;
	enum al_err err;
	size_t send_bytes;
	size_t recv_bytes;
};

#define TEST_NET_STREAM_TCP_NUM	(2)
#define TEST_NET_STREAM_TLS_NUM	(1)

#define TEST_STREAM_PACKAGE_NUM		(1000)
#define TEST_STREAM_WAIT_CIRCLE		(300)

#define TEST_STREAM_HOSTNAME	"altest.aylanetworks.com"

#define TEST_STREAM_TCP_PORT	(8888)
#define TEST_STREAM_TLS_PORT	(8889)

static char stream_buffer[4 * 1024];
static u32 test_ip_any;
static u32 test_ip_none = (u32)-1;
static u32 ipv4_buf = (u32)-1;
static struct al_net_addr addr_buf;

static enum al_err testcases_stream_null_connected(void *arg,
		struct al_net_stream *stream, enum al_err err)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)arg;

	p_param->err = err;
	return AL_ERR_OK;
}

static void testcases_stream_null_sent_cb(void *arg,
		struct al_net_stream *stream, size_t len_sent)
{
}

static enum al_err testcases_stream_async_connected(void *arg,
		struct al_net_stream *stream, enum al_err err)
{
	char *result;
	int rc;

	rc = al_net_stream_is_established(stream);
	if (AL_ERR_OK == err && 0 != rc) {
		result = NULL;
	} else {
		result = "connection failed";
	}
	al_net_stream_close(stream);
	pfm_test_case_async_finished(result);
	return AL_ERR_OK;
}

static enum al_err testcases_stream_recv_cb(void *arg,
		struct al_net_stream *stream, void *data, size_t size)
{
	pfm_test_case_async_finished(NULL);
	return AL_ERR_OK;
}

static void testcases_stream_sent_cb(void *arg,
		struct al_net_stream *stream, size_t len_sent)
{
	pfm_test_case_async_finished(NULL);
}

static void testcases_stream_err_cb(void *arg, enum al_err err)
{
	pfm_test_case_async_finished(NULL);
}

static struct al_stream_arg al_stream_tcp_dft = {
	.type = AL_NET_STREAM_TCP,
	.hostname = TEST_STREAM_HOSTNAME,
	.host_addr = &addr_buf,
	.port = TEST_STREAM_TCP_PORT,
	.data = stream_buffer,
	.len = sizeof(stream_buffer),
	.arg = NULL,

	.connected = testcases_stream_async_connected,
	.recv_cb = testcases_stream_recv_cb,
	.send_cb = testcases_stream_sent_cb,
	.err_cb = testcases_stream_err_cb,
};

static struct al_stream_arg al_stream_tls_dft = {
	.type = AL_NET_STREAM_TLS,
	.hostname = TEST_STREAM_HOSTNAME,
	.host_addr = &addr_buf,
	.port = TEST_STREAM_TLS_PORT,
	.data = stream_buffer,
	.len = sizeof(stream_buffer),
	.arg = NULL,

	.connected = testcases_stream_async_connected,
	.recv_cb = testcases_stream_recv_cb,
	.send_cb = testcases_stream_sent_cb,
	.err_cb = testcases_stream_err_cb,
};

static char stream_server_ip[32] = {"127.0.0.1"};
static struct pfm_test_var stream_var_ip = {
		.name = "stream_server_ip",
		.buf = stream_server_ip,
		.buf_size = sizeof(stream_server_ip),
};
static const struct pfm_test_var * const stream_vars[] = {
		&stream_var_ip,
		NULL,
};
PFM_COMBI_PARAM_VALS(al_stream_invalid_ptr, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(al_stream_valid_type, enum al_net_stream_type, 2,
		0, AL_NET_STREAM_MAX - 1);
PFM_COMBI_PARAM_VALS(al_stream_invalid_type, enum al_net_stream_type, 2,
		-1, AL_NET_STREAM_MAX);

PFM_COMBI_PARAM_VALS(al_stream_arg, void *, 4,
		NULL, (void *)1, stream_buffer, (void *)-1);

PFM_COMBI_PARAM_VALS(al_stream_invalid_hostname, const char *, 1, NULL);
PFM_COMBI_PARAM_VALS(al_stream_invalid_addr, struct al_net_addr *, 3, NULL,
		(struct al_net_addr *)&test_ip_any,
		(struct al_net_addr *)&test_ip_none);
PFM_COMBI_PARAM_VALS(al_stream_valid_tcp_port, u16, 1, TEST_STREAM_TCP_PORT);
PFM_COMBI_PARAM_VALS(al_stream_valid_tls_port, u16, 1, TEST_STREAM_TLS_PORT);
PFM_COMBI_PARAM_VALS(al_stream_invalid_port, u16, 1, 0);
PFM_COMBI_PARAM_VALS(al_stream_invalid_stream, int, 1, 1);

static void testcases_stream_get_conf(const char *ip)
{
	ipv4_buf = str_to_ipv4(ip);
	al_net_addr_set_ipv4(&addr_buf, ipv4_buf);
	ASSERT((u32)-1 != ipv4_buf);
}

static void testcases_stream_connect_mthread_init(
		struct pfm_tf_mthread_info *info, int argc, char **argv)
{
	testcases_stream_get_conf(stream_server_ip);
	ASSERT((u32)-1 != ipv4_buf);
}

static void testcases_stream_init_combi_param(struct pfm_tf_combi_param_info
		*info, int argc, char **argv)
{
	struct al_stream_arg *p_param;

	testcases_stream_get_conf(stream_server_ip);
	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	if (0 == p_param->overwrite_stream) {
		p_param->stream = al_net_stream_new(p_param->type);
		ASSERT(NULL != p_param->stream);
	}
}
static void testcases_stream_final_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	err = al_net_stream_close(p_param->stream);
	ASSERT(AL_ERR_OK == err);
	p_param->stream = NULL;
}
/*
C610230 test al_net_stream_new and al_net_stream_close
expect:
1.no assert
2.al_net_stream_new return a non-null pointer
*/
static const void *al_stream_open_close_combi_param[] = {
	&al_stream_valid_type,
};

static void testcases_stream_open_and_close(struct pfm_tf_combi_param_info
		*info)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_stream_new(%d) ...",
	    p_param->type);
	p_param->stream = al_net_stream_new(p_param->type);
	ASSERT(NULL != p_param->stream);

	err = al_net_stream_close(p_param->stream);
	ASSERT(AL_ERR_OK == err);
	p_param->stream = NULL;
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_open_and_close_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 1),
		pfm_tcase_make_name("net-stream", "C610230"), 1, EXPECT_SUCCESS,
		stream_vars, &al_stream_tcp_dft,
		sizeof(struct al_stream_arg),
		al_stream_open_close_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_open_and_close,
		testcases_stream_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, type));

/*
C610231 test al_net_stream_new with invalid parameters
expect:
1.assert
*/
static const void *al_stream_open_invalid_type_combi_param[] = {
	&al_stream_invalid_type,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_open_invalid_type_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 2),
		pfm_tcase_make_name("net-stream", "C610231"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_stream_open_invalid_type_combi_param, NULL,
		testcases_stream_open_and_close, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, type));

/*
C610232 test al_net_stream_close with invalid parameters
expect:
1.no assert
*/
static const void *al_stream_close_invalid_combi_param[] = {
	&al_stream_invalid_ptr,
};

static void testcases_stream_close_invalid(struct pfm_tf_combi_param_info
		*info)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_stream_close(%p) ...",
	    p_param->stream);

	err = al_net_stream_close(p_param->stream);
	ASSERT(AL_ERR_OK == err);
	p_param->stream = NULL;
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_close_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 3),
		pfm_tcase_make_name("net-stream", "C610232"), 1, EXPECT_SUCCESS,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_stream_close_invalid_combi_param, NULL,
		testcases_stream_close_invalid, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, stream));

/*
C610233 test al_net_stream_set_arg with normal parameters
expect:
1.no assert
*/
static const void *al_stream_set_arg_combi_param[] = {
	&al_stream_arg,
};
static void testcases_stream_set_arg(struct pfm_tf_combi_param_info
		*info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_stream_set_arg(%p, %p) ...",
	    p_param->stream, p_param->arg);

	al_net_stream_set_arg(p_param->stream, p_param->arg);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_set_arg_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 4),
		pfm_tcase_make_name("net-stream", "C610233"), 1, EXPECT_SUCCESS,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_stream_set_arg_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_set_arg,
		testcases_stream_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, arg));

/*
C610234 test al_net_stream_set_arg with invalid parameters
expect:
1.assert
*/
static const void *al_stream_set_arg_invalid_combi_param[] = {
	&al_stream_invalid_ptr,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_set_arg_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 5),
		pfm_tcase_make_name("net-stream", "C610234"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_stream_set_arg_invalid_combi_param, NULL,
		testcases_stream_set_arg, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, stream));

/*
C610235 test al_net_stream_connect tcp with normal parameters
expect:
1.no assert
2.connected callback is called, and pass err=AL_ERR_OK
*/
static const void *al_stream_connect_valid_tcp_combi_param[] = {
	&al_stream_valid_tcp_port,
};

static void testcases_stream_connect_valid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	ASSERT((u32)-1 != ipv4_buf);

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_stream_connect(%p, %s, %p, %d, %p) ...",
	    p_param->stream, p_param->hostname, p_param->host_addr,
	    p_param->port, p_param->connected);

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, p_param->port, p_param->connected);
	ASSERT(AL_ERR_OK == err);

	pfm_test_case_async_start();
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_connect_valid_tcp_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 6),
		pfm_tcase_make_name("net-stream", "C610235"), 1,
		EXPECT_SUCCESS, NULL,
		&al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_stream_connect_valid_tcp_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_connect_valid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, port));

/*
C610236 test al_net_stream_connect tls with normal parameters
expect:
1.no assert
2.connected callback is called, and pass err=AL_ERR_OK
*/
static const void *al_stream_connect_valid_tls_combi_param[] = {
	&al_stream_valid_tls_port,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_connect_valid_tls_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 7),
		pfm_tcase_make_name("net-stream", "C610236"), 1,
		EXPECT_SUCCESS, NULL,
		&al_stream_tls_dft, sizeof(struct al_stream_arg),
		al_stream_connect_valid_tls_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_connect_valid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, port));

/*
C610237 test al_net_stream_connect with invalid parameters
expect:
1.assert
*/
static const void *al_stream_connect_invalid_combi_param[] = {
	&al_stream_invalid_stream,
	&al_stream_invalid_hostname,
	&al_stream_invalid_addr,
	&al_stream_invalid_port,
	&al_stream_invalid_ptr,
};

static void testcases_stream_connect_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_stream_connect(%p, %s, %p, %d, %p) ...",
	    p_param->stream, p_param->hostname, p_param->host_addr,
	    p_param->port, p_param->connected);

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, p_param->port, p_param->connected);
	ASSERT(AL_ERR_OK == err);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_connect_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 8),
		pfm_tcase_make_name("net-stream", "C610237"), 5, EXPECT_FAILURE,
		NULL, &al_stream_tls_dft, sizeof(struct al_stream_arg),
		al_stream_connect_invalid_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_connect_invalid_combi_param,
		testcases_stream_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, overwrite_stream),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, hostname),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, host_addr),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, port),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, connected));

/*
C610238 test al_net_stream_connect tls to tcp server
expect:
1.no assert
2.connection failed
*/
static const void *al_stream_connect_invalid_tls_combi_param[] = {
	&al_stream_valid_tcp_port,
};

static enum al_err testcases_stream_connected_invalid_tls(void *arg,
		struct al_net_stream *stream, enum al_err err)
{
	struct al_stream_arg *p_param;
	char *result;
	int rc;

	p_param = (struct al_stream_arg *)arg;

	rc = al_net_stream_is_established(stream);
	if (AL_ERR_OK == err && 0 != rc) {
		result = "connection invalid tls server success";
	} else {
		result = NULL;
	}
	al_net_stream_close(stream);
	p_param->stream = NULL;
	pfm_test_case_async_finished(result);
	return AL_ERR_OK;
}

static void testcases_stream_connect_invalid_tls_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);
	al_net_stream_set_arg(p_param->stream, p_param);

	test_printf("testing al_net_stream_connect(%p, %s, %p, %d, %p) ...",
	    p_param->stream, p_param->hostname, p_param->host_addr,
	    p_param->port, p_param->connected);

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, p_param->port,
	    testcases_stream_connected_invalid_tls);
	ASSERT(AL_ERR_OK == err);

	pfm_test_case_async_start();
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_connect_invalid_tls_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 9),
		pfm_tcase_make_name("net-stream", "C610238"), 1,
		EXPECT_SUCCESS, NULL,
		&al_stream_tls_dft, sizeof(struct al_stream_arg),
		al_stream_connect_invalid_tls_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_connect_invalid_tls_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, port));

/*
C610239 test al_net_stream_connect tls server without ayla certificate files
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net-stream-C610239".Please remove the ayla
certificate files before running this test case.
expect:
1.no assert
2.connection failed
*/
static const void *al_stream_connect_valid_tls_without_cert_combi_param[] = {
	&al_stream_valid_tls_port,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_connect_tls_without_cert_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-stream", "C610239"), 1,
		EXPECT_FAILURE, NULL,
		&al_stream_tls_dft, sizeof(struct al_stream_arg),
		al_stream_connect_valid_tls_without_cert_combi_param,
		testcases_stream_init_combi_param,
		testcases_stream_connect_valid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, port));

/*
C610243 test al_net_stream_connect tls in mutil-thread
expect:
1.no assert
2.connected callback is called, and pass err=AL_ERR_OK
*/

static void testcases_stream_connect_mthread(
		struct pfm_tf_mthread_info *info)
{
	u8 count = 100;
	int i, rc;
	struct al_stream_arg *p_param;
	char *result;
	enum al_err err;

	p_param = (struct al_stream_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}
	p_param->stream = NULL;

	while (count-- && !al_os_thread_get_exit_flag(info->tid)) {
		p_param->stream = al_net_stream_new(p_param->type);
		if (NULL == p_param->stream) {
			result = "al_net_stream_new failed";
			goto on_exit;
		}
		p_param->err = AL_ERR_OK;
		al_net_stream_set_arg(p_param->stream, p_param);

		err = al_net_stream_connect(p_param->stream, p_param->hostname,
		    p_param->host_addr, p_param->port,
		    testcases_stream_null_connected);
		if (AL_ERR_OK != err) {
			result = "al_net_stream_connect failed";
			goto on_exit;
		}
		for (i = 0; i < TEST_STREAM_WAIT_CIRCLE; i++) {
			al_os_thread_sleep(100);
			rc = al_net_stream_is_established(p_param->stream);
			if (0 != rc || AL_ERR_OK != p_param->err) {
				break;
			}
		}
		if (0 == rc || AL_ERR_OK != p_param->err) {
			result = "al_net_stream_connect failed";
			goto on_exit;
		}

		err = al_net_stream_close(p_param->stream);
		p_param->stream = NULL;
	}

	result = NULL;
on_exit:
	if (NULL != p_param) {
		al_net_stream_close(p_param->stream);
		p_param->stream = NULL;
	}
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_stream_connect_tls_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 10),
		pfm_tcase_make_name("net-stream", "C610243"),
		TEST_NET_STREAM_TLS_NUM, EXPECT_SUCCESS, NULL,
		&al_stream_tls_dft, sizeof(struct al_stream_arg),
		testcases_stream_connect_mthread_init,
		testcases_stream_connect_mthread, NULL);

/*
C610244 test al_net_stream_connect tcp in mutil-thread
expect:
1.no assert
2.connected callback is called, and pass err=AL_ERR_OK
*/

PFM_MTHREAD_TEST_DESC_DEF(al_stream_connect_tcp_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 11),
		pfm_tcase_make_name("net-stream", "C610244"),
		TEST_NET_STREAM_TCP_NUM, EXPECT_SUCCESS, NULL,
		&al_stream_tcp_dft, sizeof(struct al_stream_arg),
		testcases_stream_connect_mthread_init,
		testcases_stream_connect_mthread, NULL);

/*
C610245 test al_net_stream_connect double call
expect:
1.no assert
2.al_os_lock_create returns non-null pointer
3.the first connect success, and the second connect return failed
*/
static int test_connect_count;
static enum al_err testcases_stream_connected_double_call(void *arg,
		struct al_net_stream *stream, enum al_err err)
{
	struct al_stream_arg *p_param;
	char *result;

	p_param = (struct al_stream_arg *)arg;

	test_connect_count++;
	if (test_connect_count >= 2) {
		result = "connect twice";
		goto on_exit;
	}
	if (AL_ERR_OK == err) {
		test_printf("stream=%p:%p", stream, p_param->stream);
		err = al_net_stream_connect(p_param->stream,
		    p_param->hostname, p_param->host_addr,
		    TEST_STREAM_TLS_PORT,
		    testcases_stream_connected_double_call);
		if (AL_ERR_OK == err) {
			return AL_ERR_OK;
		} else {
			result = NULL;
		}
	} else {
		result = "connect failed";
		goto on_exit;
	}

on_exit:
	if (NULL != p_param) {
		al_net_stream_close(p_param->stream);
		p_param->stream = NULL;
	}
	pfm_test_case_async_finished(result);
	return AL_ERR_OK;
}

static char *testcases_stream_connect_double_call(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_stream_arg *p_param;
	enum al_err err;

	testcases_stream_get_conf(stream_server_ip);
	p_param = (struct al_stream_arg *)pcase->arg;

	p_param->stream = al_net_stream_new(p_param->type);
	ASSERT(p_param->stream);

	al_net_stream_set_arg(p_param->stream, p_param);
	test_connect_count = 0;

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, TEST_STREAM_TCP_PORT,
	    testcases_stream_connected_double_call);
	ASSERT(AL_ERR_OK == err);

	pfm_test_case_async_start();
	return NULL;
}

PFM_TEST_DESC_DEF(al_stream_connect_tcp_double_call,
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 12),
	pfm_tcase_make_name("net-stream", "C610245"), EXPECT_SUCCESS,
	NULL, testcases_stream_connect_double_call, &al_stream_tcp_dft);

/*
C610246 test al_net_stream_is_established with invalid parameters
expect:
1.assert
*/
static const void *al_stream_is_established_combi_param[] = {
	&al_stream_invalid_ptr,
};
static void testcases_stream_is_established_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	al_net_stream_is_established(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_stream_stream_is_established_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 13),
		pfm_tcase_make_name("net-stream", "C610246"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_stream_is_established_combi_param, NULL,
		testcases_stream_is_established_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, stream));

/*
C610249 test al_net_stream_continue_recv with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_continue_recv_combi_param[] = {
	&al_stream_invalid_ptr,
};
static void testcases_stream_continue_recv_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	al_net_stream_continue_recv(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_continue_recv_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 14),
		pfm_tcase_make_name("net-stream", "C610249"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_continue_recv_combi_param, NULL,
		testcases_stream_continue_recv_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, stream));

/*
C610250 test al_net_stream_set_recv_cb with invalid parameters
expect:
1.assert
*/

static const void *al_net_stream_set_recv_cb_combi_param[] = {
	&al_stream_invalid_stream,
	&al_stream_invalid_ptr,
};
static void testcases_stream_set_recv_cb_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	if (0 == p_param->overwrite_stream) {
		p_param->stream = al_net_stream_new(p_param->type);
	}
	test_printf("test al_net_stream_set_recv_cb(%p, %p)", p_param->stream,
	    p_param->recv_cb);
	al_net_stream_set_recv_cb(p_param->stream, p_param->recv_cb);

	al_net_stream_close(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_set_recv_cb_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 15),
		pfm_tcase_make_name("net-stream", "C610250"), 2, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_set_recv_cb_combi_param, NULL,
		testcases_stream_set_recv_cb_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, overwrite_stream),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, recv_cb));

/*
C610253 test al_net_stream_recved with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_recved_combi_param[] = {
	&al_stream_invalid_stream,
};
static void testcases_stream_recved_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	if (0 == p_param->overwrite_stream) {
		p_param->stream = al_net_stream_new(p_param->type);
	}
	test_printf("test al_net_stream_recved(%p, %p)", p_param->stream,
	    p_param->len);
	al_net_stream_recved(p_param->stream, p_param->len);

	al_net_stream_close(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_recved_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 16),
		pfm_tcase_make_name("net-stream", "C610253"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_recved_combi_param, NULL,
		testcases_stream_recved_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, overwrite_stream));

/*
C610255 test tcp echo
expect:
1.no assert
2.connect to server success
3.the sent data match the received data
*/
static void testcases_stream_echo_final(struct al_stream_arg *p_param)
{
	if (NULL != p_param) {
		test_printf("count=%d, send_bytes=%zd, recv_bytes=%zd",
		    p_param->count, p_param->send_bytes, p_param->recv_bytes);
		al_hash_sha1_ctxt_free(p_param->send_ctxt);
		al_hash_sha1_ctxt_free(p_param->recv_ctxt);
		al_net_stream_close(p_param->stream);
		p_param->stream = NULL;
	}
}
static int testcases_stream_echo_send(struct al_stream_arg *p_param)
{
	char test_net_stream_buf[1024];
	enum al_err err;

	if (p_param->count >= TEST_STREAM_PACKAGE_NUM) {
		return 0;
	}
	al_random_fill(test_net_stream_buf, sizeof(test_net_stream_buf));
	err = al_net_stream_write(p_param->stream, test_net_stream_buf,
	    sizeof(test_net_stream_buf));
	if (AL_ERR_OK != err) {
		return -1;
	}
	err = al_net_stream_output(p_param->stream);
	if (AL_ERR_OK != err) {
		return -1;
	}
	al_hash_sha1_add(p_param->send_ctxt, test_net_stream_buf,
	    sizeof(test_net_stream_buf));
	p_param->count++;
	p_param->send_bytes += sizeof(test_net_stream_buf);
	return 0;
}

static void testcases_stream_echo_recv(struct al_stream_arg *p_param,
		void *data, size_t size)
{
	char send_sha1[AL_HASH_SHA1_SIZE], recv_sha1[AL_HASH_SHA1_SIZE];
	char *result;
	int rc;

	al_hash_sha1_add(p_param->recv_ctxt, data, size);
	p_param->recv_bytes += size;
	if (p_param->recv_bytes == p_param->send_bytes
	    && TEST_STREAM_PACKAGE_NUM == p_param->count) {
		al_hash_sha1_final(p_param->send_ctxt, send_sha1);
		al_hash_sha1_final(p_param->recv_ctxt, recv_sha1);
		rc = memcmp(send_sha1, recv_sha1, AL_HASH_SHA1_SIZE);
		if (0 != rc) {
			result = "data verify failed";
		} else {
			result = NULL;
		}
		testcases_stream_echo_final(p_param);
		pfm_test_case_async_finished(result);
	} else {
		testcases_stream_echo_send(p_param);
	}
}

static enum al_err testcases_stream_echo_recv_cb(void *arg,
		struct al_net_stream *stream, void *data, size_t size)
{
	struct al_stream_arg *p_param;
	p_param = (struct al_stream_arg *)arg;

	testcases_stream_echo_recv(p_param, data, size);
	return AL_ERR_OK;
}

static void testcases_stream_echo_err_cb(void *arg, enum al_err err)
{
	struct al_stream_arg *p_param;
	test_printf("arg=%p, err=%d", arg, err);
	p_param = (struct al_stream_arg *)arg;
	testcases_stream_echo_final(p_param);
	pfm_test_case_async_finished("stream_err_cb recv error");
}

static int testcases_stream_echo_init(struct al_stream_arg *p_param)
{
	if (NULL == p_param) {
		return -1;
	}

	p_param->stream = al_net_stream_new(p_param->type);
	if (NULL == p_param->stream) {
		return -1;
	}
	p_param->count = 0;
	p_param->send_bytes = 0;
	p_param->recv_bytes = 0;
	p_param->send_ctxt = al_hash_sha1_ctxt_alloc();
	p_param->recv_ctxt = al_hash_sha1_ctxt_alloc();
	if (NULL == p_param->send_ctxt || NULL == p_param->recv_ctxt) {
		return -1;
	}
	al_hash_sha1_ctxt_init(p_param->send_ctxt);
	al_hash_sha1_ctxt_init(p_param->recv_ctxt);

	al_net_stream_set_recv_cb(p_param->stream, p_param->recv_cb);
	al_net_stream_set_sent_cb(p_param->stream, p_param->send_cb);
	al_net_stream_set_err_cb(p_param->stream, p_param->err_cb);

	al_net_stream_set_arg(p_param->stream, p_param->arg);

	return 0;
}
static enum al_err testcases_stream_echo_connected(void *arg,
		struct al_net_stream *stream, enum al_err err)
{
	struct al_stream_arg *p_param;
	char *result;
	int rc;

	p_param = (struct al_stream_arg *)arg;
	if (AL_ERR_OK == err) {
		rc = testcases_stream_echo_send(p_param);
		if (rc < 0) {
			result = "send failed";
			goto on_failed;
		}
	} else {
		result = "connect failed";
		goto on_failed;
	}

	return AL_ERR_OK;
on_failed:
	testcases_stream_echo_final(p_param);
	pfm_test_case_async_finished(result);
	return AL_ERR_OK;
}

static char *testcases_stream_echo_func(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_stream_arg *p_param;
	enum al_err err;
	int rc;

	testcases_stream_get_conf(stream_server_ip);

	p_param = (struct al_stream_arg *)pcase->arg;
	p_param->recv_cb = testcases_stream_echo_recv_cb;
	p_param->send_cb = testcases_stream_null_sent_cb;
	p_param->err_cb = testcases_stream_echo_err_cb;
	p_param->arg = p_param;
	rc = testcases_stream_echo_init(p_param);
	ASSERT(0 == rc);

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, p_param->port,
	    testcases_stream_echo_connected);
	ASSERT(AL_ERR_OK == err);

	pfm_test_case_async_start();
	return NULL;
}

PFM_TEST_DESC_DEF(al_net_stream_echo_tcp_func,
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 17),
	pfm_tcase_make_name("net-stream", "C610255"), EXPECT_SUCCESS,
	NULL, testcases_stream_echo_func, &al_stream_tcp_dft);

/*
C610256 test tls echo
expect:
1.no assert
2.connect to server success
3.the sent data match the received data
*/

PFM_TEST_DESC_DEF(al_net_stream_echo_tls_func,
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 18),
	pfm_tcase_make_name("net-stream", "C610256"), EXPECT_SUCCESS,
	NULL, testcases_stream_echo_func, &al_stream_tls_dft);

/*
C610257 test al_net_stream_write with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_write_combi_param[] = {
	&al_stream_invalid_stream,
	&al_stream_invalid_ptr,
};

static void testcases_stream_write_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	if (0 == p_param->overwrite_stream) {
		p_param->stream = al_net_stream_new(p_param->type);
	}
	test_printf("test al_net_stream_write(%p, %p, %zd)", p_param->stream,
	    p_param->data, p_param->len);
	al_net_stream_write(p_param->stream, p_param->data, p_param->len);

	al_net_stream_close(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_write_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 19),
		pfm_tcase_make_name("net-stream", "C610257"), 2, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_write_combi_param, NULL,
		testcases_stream_write_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, overwrite_stream),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, data));

/*
C610258 test al_net_stream_write tcp in mutil thread
expect:
1.no assert
2.connect to server success
3.sending data match server's receiving data
*/
static enum al_err testcases_stream_write_mthread_recv_cb(void *arg,
		struct al_net_stream *stream, void *data, size_t size)
{
	struct pfm_tf_mthread_info *info;
	struct al_stream_arg *p_param;

	info = (struct pfm_tf_mthread_info *)arg;
	if (NULL != info) {
		p_param = (struct al_stream_arg *)(info->param);

		al_hash_sha1_add(p_param->recv_ctxt, data, size);
		p_param->recv_bytes += size;
	}
	return AL_ERR_OK;
}

static enum al_err testcases_stream_write_mthread_connected(void *arg,
		struct al_net_stream *stream, enum al_err err)
{
	struct pfm_tf_mthread_info *info;
	struct al_stream_arg *p_param;

	info = (struct pfm_tf_mthread_info *)arg;
	if (NULL != info) {
		p_param = (struct al_stream_arg *)(info->param);

		p_param->err = err;
	}
	return AL_ERR_OK;
}

static void testcases_stream_write_mthread_err_cb(void *arg, enum al_err err)
{
	struct pfm_tf_mthread_info *info;
	struct al_stream_arg *p_param;

	info = (struct pfm_tf_mthread_info *)arg;
	if (NULL != info) {
		p_param = (struct al_stream_arg *)(info->param);

		testcases_stream_echo_final(p_param);
	}
	pfm_test_frame_mthread_error(info, "stream_err_cb recv error");
}

static void testcases_stream_write_mthread(
		struct pfm_tf_mthread_info *info)
{
	int i, rc;
	struct al_stream_arg *p_param;
	char send_sha1[AL_HASH_SHA1_SIZE], recv_sha1[AL_HASH_SHA1_SIZE];
	char *result;
	enum al_err err;

	p_param = (struct al_stream_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}
	if ((u32)-1 == ipv4_buf) {
		result = "server_ip is not set";
		goto on_exit;
	}
	p_param->recv_cb = testcases_stream_write_mthread_recv_cb;
	p_param->send_cb = testcases_stream_null_sent_cb;
	p_param->err_cb = testcases_stream_write_mthread_err_cb;
	p_param->arg = info;
	rc = testcases_stream_echo_init(p_param);
	if (0 != rc) {
		result = "testcases_stream_echo_init failed";
		goto on_exit;
	}

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, p_param->port,
	    testcases_stream_write_mthread_connected);
	if (AL_ERR_OK != err) {
		result = "al_net_stream_connect failed";
		goto on_exit;
	}

	for (i = 0; i < TEST_STREAM_WAIT_CIRCLE; i++) {
		al_os_thread_sleep(100);
		rc = al_net_stream_is_established(p_param->stream);
		if (0 != rc || AL_ERR_OK != p_param->err) {
			break;
		}
	}

	if (0 == rc || AL_ERR_OK != p_param->err) {
		result = "al_net_stream_connect failed";
		goto on_exit;
	}

	while (p_param->count < TEST_STREAM_PACKAGE_NUM
	    && !al_os_thread_get_exit_flag(info->tid)) {
		rc = testcases_stream_echo_send(p_param);
		if (0 != rc) {
			result = "testcases_stream_echo_send failed";
			goto on_exit;
		}
		al_os_thread_sleep(10);
	}

	for (i = 0; i < TEST_STREAM_WAIT_CIRCLE; i++) {
		al_os_thread_sleep(100);
		if (p_param->recv_bytes == p_param->send_bytes) {
			break;
		}
	}
	if (p_param->recv_bytes == p_param->send_bytes) {
		al_hash_sha1_final(p_param->send_ctxt, send_sha1);
		al_hash_sha1_final(p_param->recv_ctxt, recv_sha1);
		rc = memcmp(send_sha1, recv_sha1, AL_HASH_SHA1_SIZE);
		if (0 != rc) {
			result = "data verify failed";
		} else {
			result = NULL;
		}
	} else {
		result = "lost packet";
	}

on_exit:
	testcases_stream_echo_final(p_param);
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_net_stream_write_tcp_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 20),
		pfm_tcase_make_name("net-stream", "C610258"),
		TEST_NET_STREAM_TCP_NUM, EXPECT_SUCCESS, NULL,
		&al_stream_tcp_dft, sizeof(struct al_stream_arg),
		testcases_stream_connect_mthread_init,
		testcases_stream_write_mthread, NULL);

/*
C610259 test al_net_stream_write tls in mutil thread
expect:
1.no assert
2.connect to server success
3.sending data match server's receiving data
*/
PFM_MTHREAD_TEST_DESC_DEF(al_net_stream_write_tls_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 21),
		pfm_tcase_make_name("net-stream", "C610259"),
		TEST_NET_STREAM_TLS_NUM, EXPECT_SUCCESS, NULL,
		&al_stream_tls_dft, sizeof(struct al_stream_arg),
		testcases_stream_connect_mthread_init,
		testcases_stream_write_mthread, NULL);

/*
C610260 test al_net_stream_write continuously without calling
al_net_stream_output
expect:
1.no assert
2.net_stream connect to server success
3.al_net_stream_write return AL_ERR_OK in the beginning, return
AL_ERR_BUF when out of memory
*/

static void testcases_stream_connect_mthread_C610260(
		struct pfm_tf_mthread_info *info)
{
	int count = 3000;
	int i, rc;
	struct al_stream_arg *p_param;
	char test_net_stream_buf[4 * 1024];
	char *result;
	enum al_err err;

	p_param = (struct al_stream_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}
	p_param->stream = al_net_stream_new(p_param->type);
	if (NULL == p_param->stream) {
		result = "al_net_stream_new failed";
		goto on_exit;
	}
	al_net_stream_set_arg(p_param->stream, p_param);

	err = al_net_stream_connect(p_param->stream, p_param->hostname,
	    p_param->host_addr, p_param->port,
	    testcases_stream_null_connected);
	if (AL_ERR_OK != err) {
		result = "al_net_stream_connect failed";
		goto on_exit;
	}

	for (i = 0; i < TEST_STREAM_WAIT_CIRCLE; i++) {
		al_os_thread_sleep(100);
		rc = al_net_stream_is_established(p_param->stream);
		if (0 != rc || AL_ERR_OK != p_param->err) {
			break;
		}
	}
	if (0 == rc || AL_ERR_OK != p_param->err) {
		result = "al_net_stream_connect failed";
		goto on_exit;
	}
	while (count-- && !al_os_thread_get_exit_flag(info->tid)) {
		err = al_net_stream_write(p_param->stream, test_net_stream_buf,
		    sizeof(test_net_stream_buf));
		if (AL_ERR_OK != err) {
			if (AL_ERR_BUF == err) {
				result = NULL;
			} else {
				result = "al_net_stream_write failed";
			}
			goto on_exit;
		}
		al_os_thread_sleep(10);
	}

	result = "al_net_stream_write not return AL_ERR_BUF";
on_exit:
	if (NULL != p_param) {
		al_net_stream_close(p_param->stream);
		p_param->stream = NULL;
	}
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_net_stream_write_without_output,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 22),
		pfm_tcase_make_name("net-stream", "C610260"),
		TEST_NET_STREAM_TCP_NUM, EXPECT_SUCCESS, NULL,
		&al_stream_tcp_dft, sizeof(struct al_stream_arg),
		testcases_stream_connect_mthread_init,
		testcases_stream_connect_mthread_C610260, NULL);

/*
C610262 test al_net_stream_output with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_output_combi_param[] = {
	&al_stream_invalid_ptr,
};
static void testcases_net_stream_output_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	al_net_stream_output(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_output_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 23),
		pfm_tcase_make_name("net-stream", "C610262"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_output_combi_param, NULL,
		testcases_net_stream_output_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, stream));

/*
C610263 test al_net_stream_set_sent_cb with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_set_sent_cb_combi_param[] = {
	&al_stream_invalid_stream,
};
static void testcases_net_stream_set_sent_cb_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	if (0 == p_param->overwrite_stream) {
		p_param->stream = al_net_stream_new(p_param->type);
	}
	test_printf("test al_net_stream_set_sent_cb(%p, %p)", p_param->stream,
	    p_param->send_cb);
	al_net_stream_set_sent_cb(p_param->stream, p_param->send_cb);

	al_net_stream_close(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_set_sent_cb_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 24),
		pfm_tcase_make_name("net-stream", "C610263"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_set_sent_cb_combi_param, NULL,
		testcases_net_stream_set_sent_cb_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, overwrite_stream));

/*
C610266 test al_net_stream_set_err_cb with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_set_err_cb_combi_param[] = {
	&al_stream_invalid_stream,
	&al_stream_invalid_ptr,
};
static void testcases_net_stream_set_err_cb_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	if (0 == p_param->overwrite_stream) {
		p_param->stream = al_net_stream_new(p_param->type);
	}
	test_printf("test al_net_stream_set_err_cb(%p, %p)", p_param->stream,
	    p_param->err_cb);
	al_net_stream_set_err_cb(p_param->stream, p_param->err_cb);

	al_net_stream_close(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_set_err_cb_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 25),
		pfm_tcase_make_name("net-stream", "C610266"), 2, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_set_err_cb_combi_param, NULL,
		testcases_net_stream_set_err_cb_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, overwrite_stream),
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, err_cb));

/*
C610268 test al_net_stream_get_tcp_obj with normal parameters
expect:
1.no assert
2.al_net_stream_get_tcp_obj return non-null pointer
*/
static const void *al_net_stream_get_tcp_obj_valid_combi_param[] = {
	&al_stream_valid_type,
};
static void testcases_net_stream_get_tcp_obj_valid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;
	struct pfm_net_tcp *tcp;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	p_param->stream = al_net_stream_new(p_param->type);
	ASSERT(NULL != p_param->stream);

	tcp = al_net_stream_get_tcp_obj(p_param->stream);
	ASSERT(NULL != tcp);

	al_net_stream_close(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_get_tcp_obj_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 26),
		pfm_tcase_make_name("net-stream", "C610268"), 1, EXPECT_SUCCESS,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_get_tcp_obj_valid_combi_param, NULL,
		testcases_net_stream_get_tcp_obj_valid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, type));

/*
C610269 test al_net_stream_get_tcp_obj with invalid parameters
expect:
1.assert
*/
static const void *al_net_stream_get_tcp_obj_invalid_combi_param[] = {
	&al_stream_invalid_ptr,
};
static void testcases_net_stream_get_tcp_obj_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_stream_arg *p_param;

	p_param = (struct al_stream_arg *)info->param;
	ASSERT(NULL != p_param);

	al_net_stream_get_tcp_obj(p_param->stream);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_stream_get_tcp_obj_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_STREAM, 27),
		pfm_tcase_make_name("net-stream", "C610269"), 1, EXPECT_FAILURE,
		NULL, &al_stream_tcp_dft, sizeof(struct al_stream_arg),
		al_net_stream_get_tcp_obj_invalid_combi_param, NULL,
		testcases_net_stream_get_tcp_obj_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_stream_arg, stream));
