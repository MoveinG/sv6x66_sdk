/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include <ayla/log.h>
#include <ayla/ipaddr_fmt.h>
#include <ayla/parse.h>
#include <al/al_err.h>
#include <al/al_os_thread.h>
#include <al/al_net_if.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_net_if_arg {
	enum al_net_if_type type;
	struct al_net_if *net_if;
	u8 *mac_addr;
};

static u8 test_mac_buffer[6];
static const struct al_net_if_arg al_net_if_dft = {
	.type = AL_NET_IF_DEF,
	.mac_addr = test_mac_buffer,
};
/* should set test_ip, test_mask and test_mac before start testing */
static const char *test_ip[AL_NET_IF_MAX];
static const char *test_mask[AL_NET_IF_MAX];
static const char *test_mac[AL_NET_IF_MAX];

PFM_COMBI_PARAM_VALS(al_net_if_valid_type, enum al_net_if_type, 3,
		AL_NET_IF_DEF, AL_NET_IF_STA, AL_NET_IF_AP);
PFM_COMBI_PARAM_VALS(al_net_if_invalid_type, enum al_net_if_type, 2,
		AL_NET_IF_DEF - 1, AL_NET_IF_AP + 1);

static char if_ip[32] = {"127.0.0.1"};
static struct pfm_test_var if_var_ip = {
		.name = "if_ip",
		.buf = if_ip,
		.buf_size = sizeof(if_ip),
};
static char if_netmask[32] = {"255.255.255.0"};
static struct pfm_test_var if_var_netmask = {
		.name = "if_netmask",
		.buf = if_netmask,
		.buf_size = sizeof(if_netmask),
};

static char if_mac[32] = {"00:00:00:00:00:00"};
static struct pfm_test_var if_var_mac = {
		.name = "if_mac",
		.buf = if_mac,
		.buf_size = sizeof(if_mac),
};
static const struct pfm_test_var * const if_vars[] = {
		&if_var_ip,
		&if_var_netmask,
		&if_var_mac,
		NULL,
};
static void testcases_net_if_check_args(const char *ip,
		const char *netmask, const char *mac)
{
	int i;

	for (i = 0; i < ARRAY_LEN(test_ip); i++) {
		test_ip[i] = ip;
	}

	for (i = 0; i < ARRAY_LEN(test_ip); i++) {
		test_mask[i] = netmask;
	}

	for (i = 0; i < ARRAY_LEN(test_ip); i++) {
		test_mac[i] = mac;
	}
}

static void testcases_net_if_combi_param_init(struct pfm_tf_combi_param_info
		*info, int argc, char **argv)
{
	struct al_net_if_arg *p_param;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);
	testcases_net_if_check_args(if_ip, if_netmask, if_mac);

	test_printf("init al_get_net_if(%d) ...", p_param->type);
	p_param->net_if = al_get_net_if(p_param->type);
	ASSERT(NULL != p_param->net_if);
}

/*
C610213 test al_get_net_if with normal parameters
(both wlan and ethernet are up)
expect:
1.no assert
2.al_get_net_if returns the network interface pointer
*/
static const void *al_net_if_valid_if_combi_param[] = {
	&al_net_if_valid_type,
};

static void testcases_net_if_get_if(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_get_net_if(%d) ...", p_param->type);
	p_param->net_if = al_get_net_if(p_param->type);
	ASSERT(NULL != p_param->net_if);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_valid_if_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 1),
		pfm_tcase_make_name("net_if", "C610213"), 1, EXPECT_SUCCESS,
		if_vars, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_if_combi_param, NULL,
		testcases_net_if_get_if, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610214 test al_get_net_if with invalid parameters
(both wlan and ethernet are up)
expect:
1.assert
*/
static const void *al_net_if_invalid_if_combi_param[] = {
	&al_net_if_invalid_type,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_get_invalid_if_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 2),
		pfm_tcase_make_name("net_if", "C610214"), 1, EXPECT_FAILURE,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_invalid_if_combi_param, NULL,
		testcases_net_if_get_if, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610215 test al_get_net_if when ethernet is down
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net_if-C610215".Please make sure
the ethernet interface is down before running this test case.
expect:
1.no assert
2.al_get_net_if returns the network interface pointer
when the interface is up
3.al_get_net_if return non-null when the interface is down
*/
static char *testcases_net_if_get_ethernet_down(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_net_if *net_if;

	net_if = al_get_net_if(AL_NET_IF_DEF);
	ASSERT(NULL != net_if);

	net_if = al_get_net_if(AL_NET_IF_DEF);
	ASSERT(NULL != net_if);
	return NULL;
}

PFM_TEST_DESC_DEF(al_net_if_get_ethernet_down_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net_if", "C610215"), EXPECT_SUCCESS,
		NULL, testcases_net_if_get_ethernet_down, NULL);

/*
C610216 test al_get_net_if when wlan is down
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net_if-C610216".Please make sure
the wlan interface is down before running this test case.
expect:
1.no assert
2.al_get_net_if returns the network interface pointer
when the interface is up
3.al_get_net_if return null when the interface is down
*/
static char *testcases_net_if_get_wlan_down(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_net_if *net_if;

	net_if = al_get_net_if(AL_NET_IF_STA);
	ASSERT(NULL != net_if);
	net_if = al_get_net_if(AL_NET_IF_AP);
	ASSERT(NULL != net_if);

	net_if = al_get_net_if(AL_NET_IF_STA);
	ASSERT(NULL != net_if);
	net_if = al_get_net_if(AL_NET_IF_AP);
	ASSERT(NULL != net_if);
	return NULL;
}

PFM_TEST_DESC_DEF(al_net_if_get_wlan_down_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net_if", "C610216"), EXPECT_SUCCESS,
		NULL, testcases_net_if_get_wlan_down, NULL);

/*
C610218 test al_net_if_get_ipv4 with normal parameters
expect:
1.no assert
2.the ip addresses match ip showed by the ifconfig
*/
static const void *al_net_if_valid_ipv4_combi_param[] = {
	&al_net_if_valid_type,
};

static void testcases_net_if_get_ipv4(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	u32 ipv4, ipv4_given;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_ipv4(%p) ...", p_param->net_if);
	ipv4 = al_net_if_get_ipv4(p_param->net_if);
	ipv4_given = str_to_ipv4(test_ip[p_param->type]);
	ASSERT(ipv4_given == ipv4);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_valid_ipv4_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 5),
		pfm_tcase_make_name("net_if", "C610218"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_ipv4_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_ipv4, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610219 test al_net_if_get_ipv4 with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_invalid_ipv4_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 6),
		pfm_tcase_make_name("net_if", "C610219"), 1, EXPECT_FAILURE,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_ipv4_combi_param, NULL,
		testcases_net_if_get_ipv4, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610220 test al_net_if_get_ipv4 when the interface is down
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net_if-C610219".Please make sure
all the interface are down before running this test case.
expect:
1.no assert
2.al_net_if_get_ipv4 return 0x0
*/
static void testcases_net_if_get_ipv4_when_noif(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	u32 ipv4;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_ipv4(%p) ...", p_param->net_if);
	ipv4 = al_net_if_get_ipv4(p_param->net_if);
	ASSERT(0 == ipv4);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		al_net_if_handle_get_ipv4_when_noif_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net_if", "C610220"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_ipv4_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_ipv4_when_noif, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610221 test al_net_if_get_netmask with normal parameters
expect:
1.no assert
2.the netmask match ip showed by the ifconfig
*/
static const void *al_net_if_valid_netmask_combi_param[] = {
	&al_net_if_valid_type,
};

static void testcases_net_if_get_netmask(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	u32 netmask, netmask_given;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_netmask(%p) ...", p_param->net_if);
	netmask = al_net_if_get_netmask(p_param->net_if);
	netmask_given = str_to_ipv4(test_mask[p_param->type]);
	ASSERT(netmask == netmask_given);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_valid_netmask_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 8),
		pfm_tcase_make_name("net_if", "C610221"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_netmask_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_netmask, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610222 test al_net_if_get_netmask with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_invalid_netmask_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 9),
		pfm_tcase_make_name("net_if", "C610222"), 1, EXPECT_FAILURE,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_netmask_combi_param, NULL,
		testcases_net_if_get_netmask, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610223 test al_net_if_get_netmask when the interface is down
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net_if-C610223".Please make sure
all the interface are down before running this test case.
expect:
1.no assert
2.al_net_if_get_netmask return 0x0
*/
static void testcases_net_if_get_netmask_when_noif(
		struct pfm_tf_combi_param_info *info)
{
	struct al_net_if_arg *p_param;
	u32 netmask;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_netmask(%p) ...", p_param->net_if);
	netmask = al_net_if_get_netmask(p_param->net_if);
	ASSERT(0 == netmask);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(
		al_net_if_handle_get_netmask_when_noif_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net_if", "C610223"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_netmask_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_netmask_when_noif, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610224 test al_net_if_get_mac_addr with normal parameters
expect:
1.no assert
2.the mac addresses match mac showed by the ifconfig
*/
static const void *al_net_if_valid_mac_combi_param[] = {
	&al_net_if_valid_type,
};

static void testcases_net_if_get_mac(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	u8 mac[6], mac_given[6];
	int rc;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_mac(%p) ...", p_param->net_if);
	rc = al_net_if_get_mac_addr(p_param->net_if, mac);
	ASSERT(0 == rc);
	rc = parse_mac(mac_given, test_mac[p_param->type]);
	ASSERT(0 == rc);
	rc = memcmp(mac_given, mac, sizeof(mac));
	ASSERT(0 == rc);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_valid_mac_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 11),
		pfm_tcase_make_name("net_if", "C610224"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_mac_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_mac, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610225 test al_net_if_get_mac_addr with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_invalid_mac_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 12),
		pfm_tcase_make_name("net_if", "C610225"), 1, EXPECT_FAILURE,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_mac_combi_param, NULL,
		testcases_net_if_get_mac, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610226 test al_net_if_get_mac_addr when the interface is down
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net_if-C610226".Please make sure
all the interface are down before running this test case.
expect:
1.no assert
2.the mac addresses match mac showed by the ifconfig
*/
static void testcases_net_if_get_mac_when_noif(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	u8 mac[6], mac_given[6];
	int rc;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_mac(%p) ...", p_param->net_if);
	rc = al_net_if_get_mac_addr(p_param->net_if, mac);
	ASSERT(0 == rc);
	rc = parse_mac(mac_given, test_mac[p_param->type]);
	ASSERT(0 == rc);
	rc = memcmp(mac_given, mac, sizeof(mac));
	ASSERT(0 == rc);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(
		al_net_if_handle_get_mac_when_noif_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net_if", "C610226"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_mac_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_mac_when_noif, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610227 test al_net_if_get_addr with normal parameters
expect:
1.no assert
2.the ip addresses match ip showed by the ifconfig
*/
static const void *al_net_if_valid_addr_combi_param[] = {
	&al_net_if_valid_type,
};

static void testcases_net_if_get_addr(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	struct al_net_addr *addr;
	u32 ipv4, ipv4_given;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_addr(%p) ...", p_param->net_if);
	addr = al_net_if_get_addr(p_param->net_if);
	ASSERT(NULL != addr);
	ipv4 = al_net_addr_get_ipv4(addr);
	ipv4_given = str_to_ipv4(test_ip[p_param->type]);
	ASSERT(ipv4_given == ipv4);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_valid_addr_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 14),
		pfm_tcase_make_name("net_if", "C610227"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_addr_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_addr, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610228 test al_net_if_get_addr with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_TEST_DESC_DEF(al_net_if_handle_get_invalid_addr_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_NET_IF, 15),
		pfm_tcase_make_name("net_if", "C610228"), 1, EXPECT_FAILURE,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_addr_combi_param, NULL,
		testcases_net_if_get_addr, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));

/*
C610229 test al_net_if_get_addr when the interface is down
NOTICE:This is a manual test, will not be called by CLI "altest all".
It should be called by CLI "altest net_if-C610229".Please make sure
all the interface are down before running this test case.
expect:
1.no assert
2.al_net_if_get_addr return NULL
*/
static void testcases_net_if_get_addr_when_noif(struct pfm_tf_combi_param_info
		*info)
{
	struct al_net_if_arg *p_param;
	struct al_net_addr *addr;
	u32 ipv4;

	p_param = (struct al_net_if_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("init al_net_if_get_addr(%p) ...", p_param->net_if);
	addr = al_net_if_get_addr(p_param->net_if);
	ASSERT(NULL != addr);
	ipv4 = al_net_addr_get_ipv4(addr);
	ASSERT(0 == ipv4);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(
		al_net_if_handle_get_addr_when_noif_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("net_if", "C610229"), 1, EXPECT_SUCCESS,
		NULL, &al_net_if_dft, sizeof(struct al_net_if_arg),
		al_net_if_valid_addr_combi_param,
		testcases_net_if_combi_param_init,
		testcases_net_if_get_addr_when_noif, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_net_if_arg, type));
