/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jsmn.h>

#include <al/al_utypes.h>
#include <ayla/utypes.h>
#include <al/al_err.h>
#include <al/al_os_thread.h>
#include <al/al_net_dns.h>
#include <ayla/ipaddr_fmt.h>
#include <ayla/assert.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

#define MAX_TEST_NUM	100
#define MAX_IP_ADDRS    6

static const char *gs_hostname[2] = {
	"ads-dev.aylanetworks.com",
	"www.aylanetworks.com",
};

static char ga_addr_set[2][MAX_IP_ADDRS][24];
static u32 ga_addr_got[2];
static int volatile ga_test_cnt[2];
static struct al_net_dns_req ga_dns_req[2];
static char *gp_result[2];
static u8 gb_sync_test;

static char ip0[24 * MAX_IP_ADDRS] = "52.72.209.12,34.195.40.112,42.26.59.7";
static char ip1[24 * MAX_IP_ADDRS] = "52.25.94.247,52.89.99.44,42.26.59.7"
	",34.213.252.117,52.27.42.170";

static struct pfm_test_var dns_var_ip0 = {
		.name = "dns_ads_ayla_com_ip",
		.buf = ip0,
		.buf_size = sizeof(ip0),
};
static struct pfm_test_var dns_var_ip1 = {
		.name = "dns_www_ayla_com_ip",
		.buf = ip1,
		.buf_size = sizeof(ip1),
};
static const struct pfm_test_var * const test_dns_vars[] = {
		&dns_var_ip0,
		&dns_var_ip1,
		NULL,
};
static void net_dns_test_init(int role)
{
	ga_addr_got[role] = 0;
	ga_test_cnt[role] = 0;
	memset(&ga_dns_req[role], 0, sizeof(ga_dns_req[0]));
	gp_result[role] = NULL;
}

static int net_dns_test_find_addr(int role, const char *addr)
{
	int i;
	const char *p_addr_set;

	if (role < 0 || role >= 2) {
		return 0; /* not found */
	}
	for (i = 0; i < MAX_IP_ADDRS; i++) {
		p_addr_set = ga_addr_set[role][i];
		if (p_addr_set == NULL) {
			break;
		}
		if (strcmp(addr, p_addr_set) == 0) {
			return 1; /* found */
		}
	}
	return 0; /* not found */
}

static void net_dns_test_case_req_cb1(struct al_net_dns_req *req)
{
	int i;
	char ip_addr[24];
	if (gb_sync_test) {
		return;
	}
	for (i = 0; i < 2; i++) {
		if (!strcmp(gs_hostname[i], req->hostname))
			break;
	}
	if (i >= 2) {
		gp_result[0] = "DNS: hostname error";
		gp_result[1] = gp_result[0];
		pfm_test_case_async_finished(gp_result[0]);
		return;
	}
	if (req->error != AL_ERR_OK) {
		gp_result[i] = "DNS: req->error";
		goto last;
	}
	ga_addr_got[i] = al_net_addr_get_ipv4(&req->addr);
	ipv4_to_str(ga_addr_got[i], ip_addr, sizeof(ip_addr));
	test_printf("DNS: addr(%s) = %s", gs_hostname[i], ip_addr);
	if (net_dns_test_find_addr(i, ip_addr)) {
		gp_result[i] = NULL;
	} else {
		gp_result[i] = "DNS: addr != addr_set";
	}
last:
	pfm_test_case_async_finished(gp_result[i]);
}

static void net_dns_test_init_addrs(int role, int argc, char **argv)
{
	char *val;
	char *mark;
	char *p;
	int i;
	int n;
	int m;
	val = test_dns_vars[role]->buf;
	ASSERT(val);
	ASSERT(val[0]);
	memset(ga_addr_set[role], 0, sizeof(ga_addr_set[role]));
	m = sizeof(ga_addr_set[0][0]);
	for (i = 0; i < MAX_IP_ADDRS; i++) {
		p = ga_addr_set[role][i];
		mark = strchr(val, ',');
		if (!mark) {
			strncpy(p, val, m);
			p[m-1] = '\0';
			break;
		} else {
			n = mark - val;
			ASSERT(n < m);
			memcpy(p, val, n);
			p[n] = '\0';
			val = mark + 1;
		}
	}
}

/*
C610201	test al_dns_req_ipv4_start and al_dns_req_cancel
1.no assert
2.show the ip address
*/
static char *net_dns_test_case_proc_C610201(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	struct al_net_dns_req *req;
	enum al_err err;

	net_dns_test_init(0);
	net_dns_test_init_addrs(0, argc, argv);
	req = &ga_dns_req[0];
	req->hostname = gs_hostname[0];
	req->callback = net_dns_test_case_req_cb1;
	gb_sync_test = 0;

	pfm_test_case_async_start();
	err = al_dns_req_ipv4_start(req);
	if (err != AL_ERR_OK) {
		return "C610201: al_dns_req_ipv4_start() fails";
	}
	return NULL;
}

/*
C610202	test al_dns_req_ipv4_start when req is null:
1.assert
*/
static char *net_dns_test_case_proc_C610202(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;

	gb_sync_test = 1;
	rc = pfm_try_catch_assert();
	if (!rc) {
		err = al_dns_req_ipv4_start(NULL);
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610202: al_dns_req_ipv4_start(NULL) should be failure";
}

/*
C610203	test al_dns_req_ipv4_start when req->hostname is null
1.assert
*/
static char *net_dns_test_case_proc_C610203(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	struct al_net_dns_req *req;

	net_dns_test_init(0);
	net_dns_test_init_addrs(0, argc, argv);
	req = &ga_dns_req[0];
	req->hostname = NULL;
	req->callback = net_dns_test_case_req_cb1;
	gb_sync_test = 1;

	rc = pfm_try_catch_assert();
	if (!rc) {
		err = al_dns_req_ipv4_start(req);
	} else {
		err = AL_ERR_ERR;
	}
	al_dns_req_cancel(req);
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610203: al_dns_req_ipv4_start(req->hostname is null) "
		"should be failure";
}

/*
C610204	test al_dns_req_ipv4_start when req->callback is null
1.assert
*/
static char *net_dns_test_case_proc_C610204(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	struct al_net_dns_req *req;

	net_dns_test_init(0);
	net_dns_test_init_addrs(0, argc, argv);
	req = &ga_dns_req[0];
	req->hostname = gs_hostname[0];
	req->callback = NULL;
	gb_sync_test = 1;

	rc = pfm_try_catch_assert();
	if (!rc) {
		err = al_dns_req_ipv4_start(req);
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610204: al_dns_req_ipv4_start(req->callback is null) "
		"should be failure";
}

/*
C610206	test double call al_dns_req_ipv4_start
1.no assert
2.show the ip address
*/
static char *net_dns_test_case_proc_C610206(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	struct al_net_dns_req *req;
	enum al_err err;

	net_dns_test_init(0);
	net_dns_test_init_addrs(0, argc, argv);
	req = &ga_dns_req[0];
	req->hostname = gs_hostname[0];
	req->callback = net_dns_test_case_req_cb1;
	gb_sync_test = 0;

	pfm_test_case_async_start();
	err = al_dns_req_ipv4_start(req);

	req->hostname = gs_hostname[0];
	err = al_dns_req_ipv4_start(req);
	if (err != AL_ERR_OK) {
		return "C610206: al_dns_req_ipv4_start() twice failed";
	}
	return NULL;
}

/*
C610207	test al_dns_req_ipv4_start calling in mutil-thread
1.no assert
2.show the ip address
Usage: altest net-dns-C610207 -- ip0=52.72.209.12,34.195.40.112 \
	ip1=52.89.99.44,52.25.94.247
*/
static void net_dns_test_case_mt_test_init(struct pfm_tf_mthread_info *info,
	int argc, char **argv)
{
	net_dns_test_init(info->id);
	net_dns_test_init_addrs(info->id, argc, argv);
}

static void net_dns_test_case_req_cb2(struct al_net_dns_req *req)
{
	int i; /* 0 -- thread 0, 1 -- thread 1 */
	int j;
	char ip_addr[24];
	for (i = 0; i < 2; i++) {
		if (!strcmp(gs_hostname[i], req->hostname))
			break;
	}
	if (i >= 2) {
		gp_result[0] = "C610207: hostname error";
		gp_result[1] = gp_result[0];
		ga_test_cnt[0] = MAX_TEST_NUM;
		ga_test_cnt[1] = MAX_TEST_NUM;
		return;
	}
	if (req->error != AL_ERR_OK) {
		gp_result[i] = "C610207: req->error != AL_ERR_OK";
		goto last;
	}
	ga_addr_got[i] = al_net_addr_get_ipv4(&req->addr);
	ipv4_to_str(ga_addr_got[i], ip_addr, sizeof(ip_addr));
	test_printf("C610207[%d]: addr(%s) = %s", i, gs_hostname[i], ip_addr);
	if (net_dns_test_find_addr(i, ip_addr)) {
		gp_result[i] = NULL;
	} else {
		for (j = 0; j < MAX_IP_ADDRS; j++) {
			if (ga_addr_set[i][j] == NULL) {
				break;
			}
		}
#if (MAX_IP_ADDRS == 6)
		test_printf("\nC610207[%d]: %s != (%s, %s, %s, %s, %s, %s)\n",
			i, ip_addr, ga_addr_set[i][0], ga_addr_set[i][1],
			ga_addr_set[i][2], ga_addr_set[i][3],
			ga_addr_set[i][4], ga_addr_set[i][5]);
#else
#error
#endif
		gp_result[i] = "C610207: addr != addr_set";
	}
last:
	if (gp_result[i]) {
		ga_test_cnt[i] = MAX_TEST_NUM;
	} else {
		ga_test_cnt[i]++;
	}
}

static void net_dns_test_case_mt_test_cb(struct pfm_tf_mthread_info *info)
{
	/* struct pfm_tf_mthread_desc *mthread_desc = info->desc; */
	int last_cnt;
	int cnt;
	int role = info->id;
	struct al_net_dns_req *req;
again:
	last_cnt = ga_test_cnt[role];
	req = &ga_dns_req[role];
	req->hostname = gs_hostname[role];
	req->callback = net_dns_test_case_req_cb2;
	al_dns_req_ipv4_start(req);
	while (!al_os_thread_get_exit_flag(info->tid)) {
		cnt = ga_test_cnt[role];
		if (cnt >= MAX_TEST_NUM || gp_result[role])
			break;
		if (cnt != last_cnt) {
			al_os_thread_sleep(10);
			goto again;
		} else {
			al_os_thread_sleep(10);
		}
	}
	pfm_test_frame_mthread_error(info, gp_result[role]);
}

/*
C610208	test al_dns_req_cancel when req is null
1.no assert
*/
static char *net_dns_test_case_proc_C610208(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;

	gb_sync_test = 1;
	rc = pfm_try_catch_assert();
	if (!rc) {
		al_dns_req_cancel(NULL);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610208: al_dns_req_cancel( NULL ) should be no assert";
}

/*
C610209	test al_dns_req_cancel before calling al_dns_req_ipv4_start
1.no assert
*/
static char *net_dns_test_case_proc_C610209(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_err err;
	struct al_net_dns_req *req;

	net_dns_test_init(0);
	net_dns_test_init_addrs(0, argc, argv);
	req = &ga_dns_req[0];
	req->hostname = gs_hostname[0];
	req->callback = net_dns_test_case_req_cb1;
	gb_sync_test = 0;

	al_dns_req_cancel(req);
	pfm_test_case_async_start();
	err = al_dns_req_ipv4_start(req);
	if (err == AL_ERR_OK) {
		return NULL;
	}
	return "C610208: Call al_dns_req cancel() and "
		"al_dns_req_ipv4_start() failure";
}

/*
C610210	test al_net_dns_delete_host with normal parameters
1.no assert
*/
static char *net_dns_test_case_proc_C610210(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	gb_sync_test = 1;
	al_net_dns_delete_host(gs_hostname[0]);
	return NULL;
}


/*
C610211	test al_net_dns_delete_host when hostname is null
1.assert
*/
static char *net_dns_test_case_proc_C610211(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;

	gb_sync_test = 1;
	rc = pfm_try_catch_assert();
	if (!rc) {
		al_net_dns_delete_host(NULL);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610211: al_net_dns_delete_host(NULL) should be failure";
}

/*
C610212	test al_net_dns_servers_rotate with normal parameters
1.no assert
*/
static char *net_dns_test_case_proc_C610212(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	gb_sync_test = 1;
	al_net_dns_servers_rotate();
	return NULL;
}

#define NET_DNS_TEST_CASE_DEF(id, ord) \
	PFM_TEST_DESC_DEF(net_dns_test_case_desc_##id,\
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_DNS, ord),\
	"net-dns-" #id, EXPECT_SUCCESS, test_dns_vars,\
	net_dns_test_case_proc_##id, NULL)

#define TEST_MULT_THREAD_NUM	2

NET_DNS_TEST_CASE_DEF(C610201, 1);
NET_DNS_TEST_CASE_DEF(C610202, 2);
NET_DNS_TEST_CASE_DEF(C610203, 3);
NET_DNS_TEST_CASE_DEF(C610204, 4);
NET_DNS_TEST_CASE_DEF(C610206, 6);
PFM_MTHREAD_TEST_DESC_DEF(net_dns_test_case_desc_C610207,
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_DNS, 7),
	pfm_tcase_make_name("net-dns", "C610207"),
	TEST_MULT_THREAD_NUM, EXPECT_SUCCESS, test_dns_vars, NULL, 0,
	net_dns_test_case_mt_test_init,
	net_dns_test_case_mt_test_cb, NULL);
NET_DNS_TEST_CASE_DEF(C610208, 8);
NET_DNS_TEST_CASE_DEF(C610209, 9);
NET_DNS_TEST_CASE_DEF(C610210, 10);
NET_DNS_TEST_CASE_DEF(C610211, 11);
NET_DNS_TEST_CASE_DEF(C610212, 12);
