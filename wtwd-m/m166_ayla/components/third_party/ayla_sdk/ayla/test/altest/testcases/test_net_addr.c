/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <al/al_utypes.h>
#include <al/al_err.h>
#include <al/al_random.h>
#include <al/al_os_thread.h>
#include <al/al_net_addr.h>
#include <platform/pfm_net_addr.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

/*
C610197	test al_net_addr_get_ipv4 before calling al_net_addr_set_ipv4
1.no assert
*/
static char *net_addr_test_case_proc_C610197(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	u32 addr1;
	struct al_net_addr addr;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		addr1 = al_net_addr_get_ipv4(&addr);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	addr1 = addr1; /* eliminate warning */
	/* Expect Success */
	if (err == AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610197: al_net_addr_get_ipv4() fails before calling"
		"al_net_addr_set_ipv4()";
}

/*
C610198	test al_net_addr_get_ipv4 after calling al_net_addr_set_ipv4:
1.no assert
2.the set and get ip addresses is the same
*/
static char *net_addr_test_case_proc_C610198(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	u32 addr1;
	u32 addr2;
	struct al_net_addr addr;
	al_random_fill(&addr1, sizeof(addr1));
	al_net_addr_set_ipv4(&addr, addr1);
	addr2 = al_net_addr_get_ipv4(&addr);
	if (addr2 != addr1) {
		return "C610198: al_net_addr_get_ipv4() value error";
	}
	return NULL;
}

/*
C610199	test al_net_addr_get_ipv4 when addr is null:
1.assert
*/
static char *net_addr_test_case_proc_C610199(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	u32 addr1;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		addr1 = al_net_addr_get_ipv4(NULL);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	addr1 = addr1; /* eliminate warning */
	return "C610199: al_net_addr_get_ipv4(NULL) should be failure";
}

/*
C610200	test al_net_addr_set_ipv4 when addr is null
1.assert
*/
static char *net_addr_test_case_proc_C610200(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		al_net_addr_set_ipv4(NULL, 0x12345678);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610200: al_net_addr_set_ipv4(NULL, addr) should be failure";
}

#define NET_ADDR_TEST_CASE_DEF(id, ord) \
	PFM_TEST_DESC_DEF(net_addr_test_case_desc_##id,\
	pfm_tcase_make_order(PFM_TCASE_ORD_NET_ADDR, ord),\
	"net-addr-" #id, EXPECT_SUCCESS, NULL, \
	net_addr_test_case_proc_##id, NULL)

NET_ADDR_TEST_CASE_DEF(C610197, 1);
NET_ADDR_TEST_CASE_DEF(C610198, 2);
NET_ADDR_TEST_CASE_DEF(C610199, 3);
NET_ADDR_TEST_CASE_DEF(C610200, 4);
