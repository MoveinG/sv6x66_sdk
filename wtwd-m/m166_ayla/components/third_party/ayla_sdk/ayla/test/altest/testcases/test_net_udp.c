/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <ayla/ipaddr_fmt.h>
#include <al/al_err.h>
#include <al/al_os_thread.h>
#include <al/al_net_addr.h>
#include <al/al_net_if.h>
#include <al/al_hash_sha1.h>
#include <al/al_net_udp.h>
#include <al/al_random.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_net_udp_arg {
	struct al_net_udp *net_udp;
	struct al_net_addr *laddr; /* local address */
	struct al_net_addr *raddr; /* remote address */
	u16 lport; /* local port */
	u16 rport; /* remote port */
	struct al_net_addr *group;
	const void *data;
	void *buf;
	size_t len;
	void *recv_arg;
	struct al_net_if *net_if;

	void (*recv_cb)(void *arg, struct al_net_udp *udp,
			void *data, size_t len,
			struct al_net_addr *from_ip, unsigned short from_port,
			struct al_net_if *net_if);

	int overwrite_net_udp;
	int overwrite_net_if;
	int use_sendto;
	int use_bind;
	int use_connect;
	int id;

	struct al_hash_sha1_ctxt *send_ctxt;
	struct al_hash_sha1_ctxt *recv_ctxt;
	int count;
	int err_count;
	size_t send_bytes;
	size_t recv_bytes;
};

#define TEST_NET_UDP_THREAD_NUM	(2)

#define TEST_UDP_PACKAGE_NUM		(1000)
#define TEST_UDP_WAIT_CIRCLE		(300)
#define TEST_UDP_PACKAGE_SIZE		(256)

#define TEST_UDP_UNREACH	0x100008f

#define TEST_NET_UDP_LPORT	(9887)
#define TEST_NET_UDP_RPORT	(8887)

static u32 nraddr_buf = TEST_UDP_UNREACH;
static struct al_net_addr laddr_buf;
static struct al_net_addr raddr_buf;
static struct al_net_udp_arg al_net_udp_dft = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = NULL,
};

static char udp_server_lip[32] = {"127.0.0.1"};
static struct pfm_test_var udp_var_lip = {
		.name = "udp_server_lip",
		.buf = udp_server_lip,
		.buf_size = sizeof(udp_server_lip),
};

static char udp_server_rip[32] = {"127.0.0.1"};
static struct pfm_test_var udp_var_rip = {
		.name = "udp_server_rip",
		.buf = udp_server_rip,
		.buf_size = sizeof(udp_server_rip),
};
static const struct pfm_test_var * const udp_vars[] = {
		&udp_var_lip,
		&udp_var_rip,
		NULL,
};

PFM_COMBI_PARAM_VALS(al_net_udp_invalid_ptr, void *, 1, NULL);

PFM_COMBI_PARAM_VALS(al_net_udp_valid_laddr, struct al_net_addr *, 1,
		(struct al_net_addr *)&laddr_buf);
PFM_COMBI_PARAM_VALS(al_net_udp_valid_raddr, struct al_net_addr *, 1,
		(struct al_net_addr *)&raddr_buf);
PFM_COMBI_PARAM_VALS(al_net_udp_valid_nraddr, struct al_net_addr *, 1,
		(struct al_net_addr *)&nraddr_buf);
PFM_COMBI_PARAM_VALS(al_net_udp_invalid_addr, struct al_net_addr *, 1, NULL);
PFM_COMBI_PARAM_VALS(al_net_udp_valid_lport, u16, 1, TEST_NET_UDP_LPORT);
PFM_COMBI_PARAM_VALS(al_net_udp_valid_rport, u16, 1, TEST_NET_UDP_RPORT);
PFM_COMBI_PARAM_VALS(al_net_udp_invalid_port, u16, 1, 0);
PFM_COMBI_PARAM_VALS(al_net_udp_invalid_net_udp, int, 1, 1);
PFM_COMBI_PARAM_VALS(al_net_udp_valid_len, size_t, 2, 1, 256);
PFM_COMBI_PARAM_VALS(al_net_udp_invalid_len, size_t, 2, 0, 257);
PFM_COMBI_PARAM_VALS(al_net_udp_invalid_udp, int, 1, 1);
PFM_COMBI_PARAM_VALS(al_net_udp_invalid_if, int, 1, 1);

static void testcases_udp_get_conf(const char *lip, const char *rip)
{
	u32 lipv4;
	u32 ripv4;

	lipv4 = str_to_ipv4(lip);
	al_net_addr_set_ipv4(&laddr_buf, lipv4);
	ASSERT((u32)-1 != lipv4);

	ripv4 = str_to_ipv4(rip);
	al_net_addr_set_ipv4(&raddr_buf, ripv4);
	ASSERT((u32)-1 != ripv4);
}

static void testcases_udp_echo_final(struct al_net_udp_arg *p_param)
{
	if (NULL != p_param) {
		test_printf("[id:%d]count=%d, send_bytes=%zd, recv_bytes=%zd",
		    p_param->id, p_param->count, p_param->send_bytes,
		    p_param->recv_bytes);
		al_net_udp_free(p_param->net_udp);
		p_param->net_udp = NULL;
		al_hash_sha1_ctxt_free(p_param->send_ctxt);
		p_param->send_ctxt = NULL;
		al_hash_sha1_ctxt_free(p_param->recv_ctxt);
		p_param->recv_ctxt = NULL;
	}
}
static int testcases_udp_echo_send(struct al_net_udp_arg *p_param)
{
	enum al_err err;

	if (p_param->count >= TEST_UDP_PACKAGE_NUM) {
		return 0;
	}
	p_param->buf = al_net_udp_buf_alloc(p_param->len);
	if (NULL == p_param->buf) {
		return 0;
	}
	al_random_fill(p_param->buf, p_param->len);
	al_hash_sha1_ctxt_init(p_param->send_ctxt);
	al_hash_sha1_add(p_param->send_ctxt, p_param->buf + AL_HASH_SHA1_SIZE,
	    p_param->len - AL_HASH_SHA1_SIZE);
	al_hash_sha1_final(p_param->send_ctxt, p_param->buf);
	if (p_param->use_sendto) {
		err = al_net_udp_sendto_if(p_param->net_udp,
		    p_param->buf, p_param->len,
		    p_param->raddr, p_param->rport, p_param->net_if);
		if (AL_ERR_OK != err) {
			return -1;
		}
	} else {
		err = al_net_udp_send(p_param->net_udp, p_param->buf,
		    p_param->len);
		if (AL_ERR_OK != err) {
			return -1;
		}
	}
	p_param->count++;
	p_param->send_bytes += p_param->len;
	return 0;
}

static void testcases_udp_echo_recv_cb(void *arg, struct al_net_udp *udp,
		void *data, size_t len, struct al_net_addr *from_ip,
		unsigned short from_port, struct al_net_if *net_if)
{
	struct al_net_udp_arg *p_param;
	p_param = (struct al_net_udp_arg *)arg;

	char recv_hash[AL_HASH_SHA1_SIZE];
	if (NULL != data && len > 0) {
		al_hash_sha1_ctxt_init(p_param->recv_ctxt);
		al_hash_sha1_add(p_param->recv_ctxt, data + AL_HASH_SHA1_SIZE,
		    len - AL_HASH_SHA1_SIZE);
		al_hash_sha1_final(p_param->recv_ctxt, recv_hash);
		if (0 != memcmp(recv_hash, data, sizeof(recv_hash))) {
			p_param->err_count++;
		}
		p_param->recv_bytes += len;
	} else {
		p_param->err_count++;
	}
	al_net_udp_buf_free(data);
}

static int testcases_udp_echo_init(struct al_net_udp_arg *p_param)
{
	int rc;
	enum al_err err;

	if (NULL == p_param) {
		rc = -1;
		goto on_failed;
	}

	testcases_udp_get_conf(udp_server_lip, udp_server_rip);

	p_param->net_udp = al_net_udp_new();
	if (NULL == p_param->net_udp) {
		rc = -2;
		goto on_failed;
	}
	p_param->count = 0;
	p_param->err_count = 0;
	p_param->send_bytes = 0;
	p_param->recv_bytes = 0;
	p_param->send_ctxt = al_hash_sha1_ctxt_alloc();
	p_param->recv_ctxt = al_hash_sha1_ctxt_alloc();
	p_param->len = TEST_UDP_PACKAGE_SIZE;
	if (NULL == p_param->send_ctxt || NULL == p_param->recv_ctxt) {
		rc = -3;
		goto on_failed;
	}

	if (p_param->use_bind) {
		err = al_net_udp_bind(p_param->net_udp, p_param->laddr,
		    p_param->lport + p_param->id);
		if (AL_ERR_OK != err) {
			rc = -4;
			goto on_failed;
		}
	}
	if (p_param->use_connect) {
		err = al_net_udp_connect(p_param->net_udp, p_param->raddr,
		    p_param->rport);
		if (AL_ERR_OK != err) {
			rc = -5;
			goto on_failed;
		}
	}
	al_net_udp_set_recv_cb(p_param->net_udp, p_param->recv_cb, p_param);
	p_param->net_if = al_get_net_if(AL_NET_IF_DEF);

	return 0;
on_failed:
	return rc;
}

static void testcases_net_udp_init_combi_param(struct pfm_tf_combi_param_info
		*info, int argc, char **argv)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	testcases_udp_get_conf(udp_server_lip, udp_server_rip);
	if (0 == p_param->overwrite_net_udp) {
		p_param->net_udp = al_net_udp_new();
		ASSERT(NULL != p_param->net_udp);
	}
	if (0 == p_param->overwrite_net_if) {
		p_param->net_if = al_get_net_if(AL_NET_IF_DEF);
		ASSERT(NULL != p_param->net_if);
	}
}
static void testcases_net_udp_final_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	al_net_udp_free(p_param->net_udp);
}

/*
C610270 test al_net_udp_new and al_net_udp_free
expect:
1.no assert
2.al_net_udp_new return a non-null pointer
*/
static char *testcases_net_udp_open_and_close(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)pcase->arg;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_new() ...");
	p_param->net_udp = al_net_udp_new();
	ASSERT(NULL != p_param->net_udp);

	test_printf("testing al_net_udp_free() ...");
	al_net_udp_free(p_param->net_udp);
	p_param->net_udp = NULL;
	return NULL;
}

PFM_TEST_DESC_DEF(al_net_udp_open_and_close_combi_param,
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 1),
	pfm_tcase_make_name("net-udp", "C610270"), EXPECT_SUCCESS,
	udp_vars, testcases_net_udp_open_and_close, &al_net_udp_dft);

/*
C610271 test al_net_udp_free with invalid parameters
expect:
1.no assert
*/
static const void *al_net_udp_free_invalid_combi_param[] = {
	&al_net_udp_invalid_ptr,
};

static void testcases_net_udp_free_invalid(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_free(%p) ...",
	    p_param->net_udp);

	al_net_udp_free(p_param->net_udp);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_free_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 2),
		pfm_tcase_make_name("net-udp", "C610271"), 1, EXPECT_SUCCESS,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_free_invalid_combi_param, NULL,
		testcases_net_udp_free_invalid, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, net_udp));

/*
C610272 test al_net_udp_buf_alloc and al_net_udp_buf_free
expect:
1.no assert
2.al_net_udp_buf_alloc returns non-null pointer
*/
static const void *al_net_udp_buf_alloc_valid_combi_param[] = {
	&al_net_udp_valid_len,
};
static void testcases_net_udp_buf_alloc(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_buf_alloc(%zd) ...",
	    p_param->len);

	p_param->buf = al_net_udp_buf_alloc(p_param->len);
	ASSERT(NULL != p_param->buf);

	al_net_udp_buf_free(p_param->buf);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_alloc_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 3),
		pfm_tcase_make_name("net-udp", "C610272"), 1, EXPECT_SUCCESS,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_buf_alloc_valid_combi_param, NULL,
		testcases_net_udp_buf_alloc, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, len));

/*
C610273 test al_net_udp_buf_alloc with invalid parameters
expect:
1.no assert
*/
static const void *al_net_udp_buf_alloc_invalid_combi_param[] = {
	&al_net_udp_invalid_len,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_buf_alloc_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 4),
		pfm_tcase_make_name("net-udp", "C610273"), 1, EXPECT_FAILURE,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_buf_alloc_invalid_combi_param, NULL,
		testcases_net_udp_buf_alloc, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, len));

/*
C610276 test al_net_udp_buf_free with invalid parameters
expect:
1.no assert
*/
static const void *al_net_udp_buf_free_invalid_combi_param[] = {
	&al_net_udp_invalid_ptr,
};
static void testcases_net_udp_buf_free_invalid(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_buf_free(%p) ...",
	    p_param->buf);

	al_net_udp_buf_free(p_param->buf);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_buf_free_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 5),
		pfm_tcase_make_name("net-udp", "C610276"), 1, EXPECT_SUCCESS,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_buf_free_invalid_combi_param, NULL,
		testcases_net_udp_buf_free_invalid, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, buf));

/*
C610277 test al_net_udp_bind with normal parameters
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_bind returns AL_ERR_OK
*/

static const void *al_net_udp_bind_valid_combi_param[] = {
	&al_net_udp_valid_laddr,
	&al_net_udp_valid_lport,
};

static void testcases_net_udp_bind_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_udp_arg *p_param;
	enum al_err err;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_bind(%p, %p, %d) ...",
	    p_param->net_udp, p_param->laddr, p_param->lport);

	err = al_net_udp_bind(p_param->net_udp, p_param->laddr,
	    p_param->lport);
	ASSERT(AL_ERR_OK == err);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_bind_valid_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-udp", "C610277"), 2, EXPECT_SUCCESS,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_bind_valid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_bind_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, laddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, lport));

/*
C610278 test al_net_udp_bind with invalid parameters
expect:
1.assert
*/
static const void *al_net_udp_bind_invalid_combi_param[] = {
	&al_net_udp_invalid_udp,
	&al_net_udp_invalid_addr,
	&al_net_udp_invalid_port,
};
PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_bind_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 6),
		pfm_tcase_make_name("net-udp", "C610278"), 3, EXPECT_FAILURE,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_bind_invalid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_bind_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, overwrite_net_udp),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, laddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, lport));

/*
C610283 test double call al_net_udp_bind
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_bind returns AL_ERR_OK in the first time,
and return non-AL_ERR_OK in the second time
4.use "netstat -ant" to check if the local port is binded
*/

static void testcases_net_udp_bind_double_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_udp_arg *p_param;
	enum al_err err;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_bind(%p, %p, %d) ...",
	    p_param->net_udp, p_param->laddr, p_param->lport);

	err = al_net_udp_bind(p_param->net_udp, p_param->laddr,
	    p_param->lport);
	ASSERT(AL_ERR_OK == err);

	err = al_net_udp_bind(p_param->net_udp, p_param->laddr,
	    p_param->lport + 1);
	ASSERT(AL_ERR_OK != err);

	/* on Linux platform, you can use netstat to check
	 * if the right port is binded. */
	al_os_thread_sleep(5000);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_bind_double_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-udp", "C610283"), 2, EXPECT_SUCCESS,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_bind_valid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_bind_double_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, laddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, lport));

/*
C610284 test al_net_udp_connect with normal parameters
expect:
1.no assert
2.connected callback return err=AL_ERR_OK
*/
static const void *al_net_udp_connect_valid_combi_param[] = {
	&al_net_udp_valid_raddr,
	&al_net_udp_valid_rport,
};

static void testcases_net_udp_connect_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_udp_arg *p_param;
	enum al_err err;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_connect(%p, %p, %d) ...",
	    p_param->net_udp, p_param->raddr, p_param->rport);

	err = al_net_udp_connect(p_param->net_udp, p_param->raddr,
	    p_param->rport);
	ASSERT(AL_ERR_OK == err);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_connect_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 7),
		pfm_tcase_make_name("net-udp", "C610284"), 2,
		EXPECT_SUCCESS, NULL,
		&al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_connect_valid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_connect_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, raddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, rport));

/*
C610285 test al_net_udp_connect with invalid parameters
expect:
1.assert
*/
static const void *al_net_udp_connect_invalid_combi_param[] = {
	&al_net_udp_invalid_net_udp,
	&al_net_udp_invalid_addr,
	&al_net_udp_invalid_port,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_connect_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 8),
		pfm_tcase_make_name("net-udp", "C610285"), 3, EXPECT_FAILURE,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_connect_invalid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_connect_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, overwrite_net_udp),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, raddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, rport));

/*
C610713 test al_net_udp_connect with normal parameters
expect:
1.no assert
2.connected callback return err=AL_ERR_OK
*/
static const void *al_net_udp_connect_double_combi_param[] = {
	&al_net_udp_valid_nraddr,
	&al_net_udp_valid_rport,
};

static void testcases_net_udp_connect_double_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_udp_arg *p_param;
	enum al_err err;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_net_udp_connect(%p, %p, %d) ...",
	    p_param->net_udp, p_param->raddr, p_param->rport);

	err = al_net_udp_connect(p_param->net_udp, p_param->raddr,
	    p_param->rport);
	ASSERT(AL_ERR_OK == err);

	err = al_net_udp_connect(p_param->net_udp, p_param->raddr,
	    p_param->rport);
	ASSERT(AL_ERR_OK == err);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_connect_double_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 9),
		pfm_tcase_make_name("net-udp", "C610713"), 2,
		EXPECT_SUCCESS, NULL,
		&al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_connect_double_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_connect_double_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, raddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, rport));

/*
C610289 test al_net_udp_connect when host is unreachable
expect:
1.no assert
2.connected callback return err=AL_ERR_OK
*/

static const void *al_net_udp_connect_unreach_combi_param[] = {
	&al_net_udp_valid_raddr,
	&al_net_udp_valid_rport,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_connect_unreach_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-udp", "C610289"), 2,
		EXPECT_FAILURE, NULL,
		&al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_connect_unreach_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_connect_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, raddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, rport));

/*
C610292 test al_net_udp_set_recv_cb with invalid parameters
expect:
1.assert
*/

static const void *al_net_udp_set_recv_cb_combi_param[] = {
	&al_net_udp_invalid_udp,
	&al_net_udp_invalid_ptr,
};
static void testcases_net_udp_set_recv_cb_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("test al_net_udp_set_recv_cb(%p, %p, %p)", p_param->net_udp,
	    p_param->recv_cb, p_param->recv_arg);
	al_net_udp_set_recv_cb(p_param->net_udp, p_param->recv_cb,
	    p_param->recv_arg);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_set_recv_cb_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 10),
		pfm_tcase_make_name("net-udp", "C610292"), 2, EXPECT_FAILURE,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_set_recv_cb_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_set_recv_cb_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, overwrite_net_udp),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, recv_cb));

/*
C610296 test al_net_udp_send with invalid parameters
expect:
1.assert
*/

static const void *al_net_udp_send_invalid_combi_param[] = {
	&al_net_udp_invalid_udp,
	&al_net_udp_invalid_ptr,
	&al_net_udp_invalid_len,
};
static void testcases_net_udp_send_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("test al_net_udp_send(%p, %p, %zd)", p_param->net_udp,
	    p_param->buf, p_param->len);
	al_net_udp_send(p_param->net_udp, p_param->buf,
	    p_param->len);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_send_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 11),
		pfm_tcase_make_name("net-udp", "C610296"), 3, EXPECT_FAILURE,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_send_invalid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_send_invalid_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, overwrite_net_udp),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, buf),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, len));

static void testcases_udp_sendto_mthread(
		struct pfm_tf_mthread_info *info)
{
	int i, rc;
	struct al_net_udp_arg *p_param;
	char *result;

	p_param = (struct al_net_udp_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}
	p_param->id = info->id;
	p_param->recv_arg = p_param;
	rc = testcases_udp_echo_init(p_param);
	if (0 != rc) {
		result = "testcases_udp_echo_init failed";
		test_printf("testcases_udp_echo_init return %d", rc);
		goto on_exit;
	}

	while (p_param->count < TEST_UDP_PACKAGE_NUM
	    && !al_os_thread_get_exit_flag(info->tid)) {
		rc = testcases_udp_echo_send(p_param);
		if (0 != rc) {
			result = "testcases_udp_echo_send failed";
			goto on_exit;
		}
		al_os_thread_sleep(10);
	}

	for (i = 0; i < TEST_UDP_WAIT_CIRCLE; i++) {
		al_os_thread_sleep(100);
		if (p_param->recv_bytes == p_param->send_bytes) {
			break;
		}
	}
	if (p_param->recv_bytes == p_param->send_bytes
	    && TEST_UDP_PACKAGE_NUM == p_param->count) {
		if (0 != p_param->err_count) {
			result = "data verify failed";
		} else {
			result = NULL;
		}
	} else {
		result = "lost packet";
	}
on_exit:
	test_printf("result=%s", result);
	testcases_udp_echo_final(p_param);
	pfm_test_frame_mthread_error(info, result);
}

/*
C610301 test al_net_udp_send without calling al_net_udp_bind
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.The data received match that sent by al_net_udp_send
*/

static struct al_net_udp_arg al_net_udp_send_without_bind_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 0,
	.use_bind = 0,
	.use_connect = 1,
};

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_send_without_bind_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 13),
		pfm_tcase_make_name("net-udp", "C610301"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_send_without_bind_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_mthread, NULL);

/*
C610302 test al_net_udp_send without calling al_net_udp_connect
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.the server will receive nothing
*/

static struct al_net_udp_arg al_net_udp_send_without_connect_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 0,
	.use_bind = 1,
	.use_connect = 0,
};

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_send_without_connect_mthread,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-udp", "C610302"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_send_without_connect_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_mthread, NULL);

/*
C610303 test al_net_udp_send calling in mutil-thread
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.The data received match that sent by al_net_udp_send
*/

static struct al_net_udp_arg al_net_udp_send_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 0,
	.use_bind = 1,
	.use_connect = 1,
};

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_send_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 12),
		pfm_tcase_make_name("net-udp", "C610303"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_send_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_mthread, NULL);

/*
C610305 test al_net_udp_sendto_if with invalid parameters
expect:
1.assert
*/

static const void *al_net_udp_sendto_invalid_combi_param[] = {
	&al_net_udp_invalid_udp,
	&al_net_udp_invalid_ptr,
	&al_net_udp_invalid_len,
	&al_net_udp_invalid_addr,
	&al_net_udp_invalid_port,
	&al_net_udp_invalid_if,
};
static void testcases_net_udp_sendto_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_udp_arg *p_param;

	p_param = (struct al_net_udp_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("test al_net_udp_sendto_if(%p, %p, %zd, %p, %u, %p)",
	    p_param->net_udp, p_param->buf, p_param->len, p_param->raddr,
	    p_param->rport, p_param->net_if);
	al_net_udp_sendto_if(p_param->net_udp, p_param->buf, p_param->len,
	    p_param->raddr, p_param->rport, p_param->net_if);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_udp_sendto_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 13),
		pfm_tcase_make_name("net-udp", "C610305"), 6, EXPECT_FAILURE,
		NULL, &al_net_udp_dft, sizeof(struct al_net_udp_arg),
		al_net_udp_sendto_invalid_combi_param,
		testcases_net_udp_init_combi_param,
		testcases_net_udp_sendto_invalid_combi_param,
		testcases_net_udp_final_combi_param,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, overwrite_net_udp),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, buf),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, len),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, raddr),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, rport),
		PFM_COMBI_PARAM_INFO_ITEM(al_net_udp_arg, overwrite_net_if));

/*
C610312 test al_net_udp_sendto_if without calling al_net_udp_bind
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.The data received match that sent by al_net_udp_sendto_if
*/

static struct al_net_udp_arg al_net_udp_sendto_without_bind_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 1,
	.use_bind = 0,
	.use_connect = 1,
};

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_sendto_without_bind_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 14),
		pfm_tcase_make_name("net-udp", "C610312"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_sendto_without_bind_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_mthread, NULL);

/*
C610313 test al_net_udp_sendto_if without calling al_net_udp_connect
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.the server will receive nothing
*/

static struct al_net_udp_arg al_net_udp_sendto_without_connect_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 1,
	.use_bind = 1,
	.use_connect = 0,
};

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_sendto_without_connect_mthread,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-udp", "C610313"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_sendto_without_connect_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_mthread, NULL);

/*
C610314 test al_net_udp_sendto_if without calling al_net_udp_bind
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.The connected server receive nothing, and the sendto destination
receive data
*/

static struct al_net_udp_arg al_net_udp_sendto_connect_other_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 1,
	.use_bind = 0,
	.use_connect = 1,
};

static void testcases_udp_sendto_connect_other_mthread(
		struct pfm_tf_mthread_info *info)
{
	int i, rc;
	struct al_net_udp_arg *p_param;
	char *result;

	p_param = (struct al_net_udp_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}
	p_param->id = info->id;
	p_param->recv_arg = p_param;
	p_param->rport++;
	rc = testcases_udp_echo_init(p_param);
	if (0 != rc) {
		result = "testcases_udp_echo_init failed";
		goto on_exit;
	}

	p_param->rport--;
	while (p_param->count < TEST_UDP_PACKAGE_NUM
	    && !al_os_thread_get_exit_flag(info->tid)) {
		rc = testcases_udp_echo_send(p_param);
		if (0 != rc) {
			result = "testcases_udp_echo_send failed";
			goto on_exit;
		}
		al_os_thread_sleep(10);
	}

	for (i = 0; i < TEST_UDP_WAIT_CIRCLE; i++) {
		al_os_thread_sleep(100);
		if (p_param->recv_bytes == p_param->send_bytes) {
			break;
		}
	}
	if (p_param->recv_bytes == p_param->send_bytes
	    && TEST_UDP_PACKAGE_NUM == p_param->count) {
		if (0 != p_param->err_count) {
			result = "data verify failed";
		} else {
			result = NULL;
		}
	} else {
		result = "lost packet";
	}
on_exit:
	testcases_udp_echo_final(p_param);
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_sendto_connect_other_mthread,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net-udp", "C610314"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_sendto_connect_other_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_connect_other_mthread, NULL);

/*
C610315 test al_net_udp_sendto_if calling in mutil-thread
expect:
1.no assert
2.al_net_udp_new returns non-null pointer
3.al_net_udp_buf_alloc return non-null pointer
4.The data received match that sent by al_net_udp_sendto_if
*/

static struct al_net_udp_arg al_net_udp_sendto_mthread = {
	.laddr = &laddr_buf,
	.lport = TEST_NET_UDP_LPORT,
	.raddr = &raddr_buf,
	.rport = TEST_NET_UDP_RPORT,
	.data = NULL,
	.len = TEST_UDP_PACKAGE_SIZE,
	.recv_arg = NULL,

	.recv_cb = testcases_udp_echo_recv_cb,
	.use_sendto = 1,
	.use_bind = 0,
	.use_connect = 0,
};

PFM_MTHREAD_TEST_DESC_DEF(al_net_udp_sendto_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_UDP, 15),
		pfm_tcase_make_name("net-udp", "C610315"),
		TEST_NET_UDP_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_net_udp_sendto_mthread,
		sizeof(struct al_net_udp_arg), NULL,
		testcases_udp_sendto_mthread, NULL);
